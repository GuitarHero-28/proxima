#include "Orderbook.h"
#include <algorithm>
#include <chrono>
#include <ctime>


//book keeping helpers
void Orderbook::UpdateLevelData(Price price, Quantity quantity, int countDelta){
    auto& data= data_[price];
    data.count += countDelta;
    data.quantity += quantity;

    //cleanup
    if(data.count == 0){
        data_.erase(price);
    }
}

void Orderbook::OnOrderAdded(OrderPointer order){
    UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(), 1);
}

void Orderbook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled){
    UpdateLevelData(price, -quantity, isFullyFilled ? -1 : 0);
}

//logic: Add Order
Trades Orderbook::AddOrder(OrderPointer order){

    //thread safety for N-API
    std::scoped_lock lock(ordersMutex_);

    //duplicate check
    if(orders_.contains(order->GetOrderId())) return {};

    // If it's a Market Buy, price it at the highest Ask (worst price)
    // so it crosses the spread and matches everything.
    if(order->GetOrderType() == OrderType::Market){
        Price worstAsk = asks_.rbegin()->first;
        order->ToGoodTillCancel(worstAsk);
    } else if (order->GetSide() == Side::Sell && !bids_.empty()){
        Price worstBid = bids_.rbegin()->first;
        order->ToGoodTillCancel(worstBid);
    } else {
        return {}; // rejected ; no liquidity for market order
    }

    //check out our cache (data_) to see if enough quantity exist befor matching
    if(order->GetOrderType() == OrderType::FillOrKill && 
       !CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity())){
         return{}; //reejcted as cannot fill immediately
    }

    OrderPointers::iterator iterator;
    if(order->GetSide() == Side::Buy){
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::next(orders.begin(),orders.size()-1);
    }

    //update fast lookups maps
    orders_.insert({order->GetOrderId(), OrderEntry{order,iterator}});
    OnOrderAdded(order);

    // Trigger Matching Engine
    return MatchOrders();
}

// logic : cancel order
void Orderbook::CancelOrder(OrderId orderId){
    std::scoped_lock lock(ordersMutex_);
    if(!orders_.contains(orderId)) return;

    const auto& [order,iterator]= orders_.at(orderId);
    orders_.erase(orderId);

    if(order->GetSide() == Side::Buy){
        auto& orders = bids_[order->GetPrice()];
        orders.erase(iterator);
        if(orders.empty()) bids_.erase(order->GetPrice());
    } else {
        auto& orders = asks_[order->GetPrice()];
        orders.erase(iterator);
        if(orders.empty()) asks_.erase(order->GetPrice());
    }

    OnOrderCancelled(order);
}

// logic:: matching engine
Trades Orderbook::MatchOrders(){
    Trades trades;
    trades.reserve(orders_.size()); // optimization : reserve mem

    while(true){
        //exit if either side is empty
        if(bids_.empty() || asks_.empty()) break;

        auto& [bidPrice, bidOrders] = *bids_.begin();
        auto& [askPrice, askOrders] = *asks_.begin();

        // Exit if Spread exists (Bid < Ask means no overlap)
        if(bidPrice < askPrice) break;

        //Match logic : While we have orders at this overlapping price level
        while(!bidOrders.empty() && !askOrders.empty()){
            auto bid = bidOrders.front();
            auto ask = askOrders.front();

            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            //update Objects
            bid->Fill(quantity);
            ask->Fill(quantity);

            //Check 'IsFilled' before erasing to avoid dangling pointers
            if(bid->IsFilled()){
                bidOrders.pop_front();
                orders_.erase(bid->GetOrderId());
                OnOrderMatched(bidPrice, quantity, true);
            }else{
                OnOrderMatched(bidPrice,quantity,false);
            }

            if (ask->IsFilled()) {
                askOrders.pop_front();
                orders_.erase(ask->GetOrderId());
                OnOrderMatched(askPrice, quantity, true);
            } else {
                OnOrderMatched(askPrice, quantity, false);
            }

            //record the table
            trades.push_back(Trade{
                TradeInfo{bid->GetOrderId(), bid->GetPrice(), quantity},
                TradeInfo{ask->GetOrderId(), ask->GetPrice(), quantity}
            });
        }

        // clean up empty levels *after* the inner loop 
        // to avoid invalidating the iterators used in the loop.
        if (bids_.begin()->second.empty()) bids_.erase(bidPrice);
        if (asks_.begin()->second.empty()) asks_.erase(askPrice);
    }

    // FillAndKill Logic: If the incoming order was FAK, cancel whatever is left
    // (Simplified for this scope: Standard matching loop handles immediate fills)
    
    return trades;
}

//CanFullyFill (for FillOrKill)
bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const{
    if(!CanMatch(side,price)) return false;

    std::optional<Price> threshold;
    if(side == Side::Buy){
        const auto& [askPrice, _] = *asks_.begin();
        threshold = askPrice;
    }else {
        const auto& [bidPrice, _] = *bids_.begin();
        threshold = bidPrice;
    }

    for(const auto& [levelPrice, levelData] : data_){
        //filters: ignore prices that are worse than our limits
        if(threshold.has_value() &&
           ((side == Side::Buy && levelPrice > price ) || 
             (side == Side::Sell && levelPrice <price))){
                continue;
        }

        // Optimistic matching: Check if cached quantity is enough
        if (quantity <= levelData.quantity) return true;
        quantity -= levelData.quantity;
    }

    return false;
}

bool Orderbook::CanMatch(Side side, Price price) const {
    if (side == Side::Buy) {
        return !asks_.empty() && price >= asks_.begin()->first;
    } else {
        return !bids_.empty() && price <= bids_.begin()->first;
    }
}

//public utilities (websockets data souce)
std::size_t Orderbook::Size() const{
    std::scoped_lock lock(ordersMutex_);
    return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const { // Fixed Typo: GetOrderInfoa -> GetOrderInfos
    std::scoped_lock lock(ordersMutex_);
    OrderbookLevelInfos infos;
    infos.bids.reserve(bids_.size());
    infos.asks.reserve(asks_.size());

    // Serialize Bids
    for (const auto& [price, orders] : bids_) {
        Quantity q = 0;
        for (const auto& o : orders) q += o->GetRemainingQuantity();
        infos.bids.push_back({price, q}); 
    }

    // Serialize asks
    for (const auto& [price, orders] : asks_) {
        Quantity q = 0;
        for (const auto& o : orders) q += o->GetRemainingQuantity();
        infos.asks.push_back({price, q});
    }

    return infos;
}
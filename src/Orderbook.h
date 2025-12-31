#pragma once 
#include "Order.h"
#include "Data.h"

#include <map>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <optional>

struct OrderbookLevelInfos {
    LevelInfos bids;
    LevelInfos asks;
};

struct TradeInfo {
   OrderId orderId;
   Price price;
   Quantity quantity;
};

struct Trade {
    TradeInfo bidTrade;
    TradeInfo askTrade;
};

using Trades =std::vector<Trade>;

class Orderbook{
    private:
    struct OrderEntry {
        OrderPointer order;
        OrderPointers::iterator location;
    };

    struct LevelData {
         Quantity quantity=0;
         uint32_t count=0;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;

    std::map<Price, OrderPointers, std::less<Price>> asks_;

    std::unordered_map<OrderId, OrderEntry> orders_;

    std::unordered_map<Price, LevelData> data_;

    mutable std::mutex ordersMutex_;

    bool CanMatch(Side side, Price price) const;
    bool CanFullyFill(Side side, Price price, Quantity quantity) const;
    Trades MatchOrders();

    void OnOrderAdded(OrderPointer order);
    void OnOrderCancelled(OrderPointer order);
    void OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled);
    void UpdateLevelData(Price price, Quantity quantity, int countDelta);

    public:
    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId orderId);

    OrderbookLevelInfos GetOrderInfos() const;

    std::size_t Size() const;

};
#pragma once
#include "Data.h"
#include <memory>
#include <vector>
#include <list>
#include <exception>
#include <format>
#include <string>

struct Order{
    private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;

    public:
    // Constructor
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_(orderType)
        , orderId_(orderId)
        , side_(side)
        , price_(price)
        , initialQuantity_(quantity)
        , remainingQuantity_(quantity)
    {
    }

    //getters
    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return initialQuantity_ - remainingQuantity_; }

    bool IsFilled() const 
    {
        return remainingQuantity_ == 0; 
    }

    void Fill(Quantity quantity)
    {
        if (quantity > remainingQuantity_) 
             throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", orderId_));

        remainingQuantity_ -= quantity;
    }

    void ToGoodTillCancel(Price price)
    {
        if(orderType_ != OrderType::Market) {
             throw std::logic_error(std::format("Order ({}) cannot have price adjusted, only Market orders can.", orderId_));
        }
        price_=price;
        orderType_ =OrderType::GoodTillCancel;
    }
};

using OrderPointer = std::shared_ptr<Order>; 
using OrderPointers = std::list<OrderPointer>;
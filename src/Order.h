#pragma once
#include "Data.h"
#include <memory>
#include <vector>
#include <list>
#include <exception>
#include <format>
#include <string>

struct Order{
    OrderType orderType;
    OrderId orderId;
    Side side;
    Price price;
    Quantity initialQuantity;
    Quantity remainingQuantity;

    // Constructor
    Order(OrderType _orderType, OrderId _orderId, Side _side, Price _price, Quantity _quantity)
        : orderType(_orderType)
        , orderId(_orderId)
        , side(_side)
        , price(_price)
        , initialQuantity(_quantity)
        , remainingQuantity(_quantity)
    {
    }

    bool IsFilled() const 
    {
        return remainingQuantity == 0; 
    }

    void Fill(Quantity quantity)
    {
        if (quantity > remainingQuantity) 
             throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", orderId));

        remainingQuantity -= quantity;
    }
};

using OrderPointer = std::shared_ptr<Order>; 
using OrderPointers = std::list<OrderPointer>;
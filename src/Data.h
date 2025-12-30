#pragma once
#include <cstdint>
#include <vector>
#include <limits>

// types of order 
enum class OrderType {
    GoodTillCancel, 
    FillAndKill
};

//side i.e buy or sell
enum class Side{
    Buy,
    Sell
};

using Price =std::int32_t;
using Quantity =std::uint32_t;
using OrderId =std::uint64_t;

constexpr Price InvalidPrice =std::numeric_limits<Price>::min();

//what the frontend sees
struct LevelInfo {
    Price price;
    Quantity quantity;
};

using LevelInfos = std::vector<LevelInfo>;




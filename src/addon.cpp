#include <napi.h> 
#include "Orderbook.h"
#include "Order.h"
#include <string>

//wrapper class which exsposes orderbook class to JavaScript
class OrderbookWrapper : public Napi::ObjectWrap<OrderbookWrapper> {
    public:
    static Napi::Object Init(Napi::Env env,Napi::Object exports);
    OrderbookWrapper(const Napi::CallbackInfo& info);

    private:
    static Napi::FunctionReference constructor;
    Orderbook* orderbook_;//pointer to the actual c++ engine

    //methods exposed to JavaScript
    Napi::Value AddOrder(const Napi::CallbackInfo& info);
    Napi::Value GetOrderInfos(const Napi::CallbackInfo& info);
};

Napi::FunctionReference OrderbookWrapper::constructor;

//implementation

//initializing the class for Node.js
Napi::Object OrderbookWrapper::Init(Napi::Env env, Napi::Object exports){
    Napi::Function func = DefineClass(env, "Orderbook",{
    InstanceMethod("addOrder", &OrderbookWrapper::AddOrder),
    InstanceMethod("getOrderInfos", &OrderbookWrapper::GetOrderInfos),
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("Orderbook", func);
    return exports;
}

// constructor (called when you do 'new Orderbook()' in JS)
OrderbookWrapper::OrderbookWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<OrderbookWrapper>(info) {
    this->orderbook_ = new Orderbook();
}

// Method: addOrder(type, side, price, quantity, id)
// This is the critical "Low Latency" function.
Napi::Value OrderbookWrapper::AddOrder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    //validation: ensure JS sent correct arguments
    if(info.Length() < 5){
        Napi::TypeError::New(env, "Expected 5 arguments: type, side, price, qty, id").ThrowAsJavaScriptException();
        return env.Null();
    }

    //we read raw c++ integers directly from V8 memory
    int typeRaw = info[0].As<Napi::Number>().Int32Value();
    int sideRaw = info[1].As<Napi::Number>().Int32Value();
    Price price = info[2].As<Napi::Number>().Int32Value();
    Quantity quantity = info[3].As<Napi::Number>().Int32Value();

    //use Int64 for OrderId, but cast carefully
    OrderId id = static_cast<OrderId>(info[4].As<Napi::Number>().Int64Value());

    //cast integers to enums
    OrderType type = static_cast<OrderType>(typeRaw);
    Side side = static_cast<Side>(sideRaw);

    //create order and send to engine
    auto order = std::make_shared<Order>(type, id, side, price, quantity);

    //execute machine engine
    Trades trades = this->orderbook_->AddOrder(order);

    //return trades to JS (array of Objects)
    Napi::Array result = Napi::Array::New(env, trades.size());
    for(size_t i=0; i<trades.size() ; ++i) {
        Napi::Object trade = Napi::Object::New(env);
        trade.Set("price", trades[i].bidTrade.price);
        trade.Set("quantity", trades[i].bidTrade.quantity);

        // Convert uint64 to string for JS safety (JS loses precision > 2^53)
        trade.Set("bidId", std::to_string(trades[i].bidTrade.orderId)); 
        trade.Set("askId", std::to_string(trades[i].askTrade.orderId));
        
        result[i] = trade;
    }

    return result;
}

// method : getOrderInfos() -> {bids: [], asks: []}
//this powers the React UI via websockets
Napi::Value OrderbookWrapper::GetOrderInfos(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    //get snapshot from engine
    OrderbookLevelInfos infos = this->orderbook_->GetOrderInfos();

    //create JS Response Object
    Napi::Object result = Napi::Object::New(env);
    Napi::Array bids = Napi::Array::New(env, infos.bids.size());
    Napi::Array asks = Napi::Array::New(env, infos.asks.size());

    //serialize bids
    for(size_t i=0; i< infos.bids.size(); ++i){
        Napi::Object level = Napi::Object::New(env);
        level.Set("price", infos.bids[i].price);
        level.Set("quantity", infos.bids[i].quantity);
        bids[i] = level;
    }

    //serialize Asks
    for (size_t i = 0; i < infos.asks.size(); i++) {
        Napi::Object level = Napi::Object::New(env);
        level.Set("price", infos.asks[i].price);
        level.Set("quantity", infos.asks[i].quantity);
        asks[i] = level;
    }

    result.Set("bids", bids);
    result.Set("asks", asks);
    return result;
}

//module export
Napi::Object InitAll(Napi::Env env, Napi::Object exports){
    return OrderbookWrapper::Init(env, exports);
}

NODE_API_MODULE(proxima, InitAll)
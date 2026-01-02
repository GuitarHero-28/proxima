const express = require('express');
const { WebSocketServer } = require('ws');
// This magic line loads your C++ Engine!
const addon = require('bindings')('proxima'); 

const app = express();
const port = 3000;

// 1. Initialize the C++ Engine
const engine = new addon.Orderbook();
console.log("✅ C++ Trading Engine Loaded Successfully!");

// TEST RUN: Let's simulate a trade right now to verify it works
console.log("--- Running Initial Simulation ---");

// Order Types (from C++ Enum): 0 = GoodTillCancel, 3 = Market
// Sides (from C++ Enum): 0 = Buy, 1 = Sell

// Step A: Place a Buy Order (Buy 100 shares @ $150)
console.log("➕ Sending BUY Order...");
const buyResult = engine.addOrder(0, 0, 150, 100, 1001); 
console.log("   Engine Response:", buyResult); // Should be empty (no match yet)

// Step B: Place a Matching Sell Order (Sell 50 shares @ $150)
console.log("kB➕ Sending SELL Order (Match)...");
const sellResult = engine.addOrder(0, 1, 150, 50, 2001); 
console.log("   Engine Response (Trade Executed!):");
console.log(sellResult); // Should show the trade details!

// Step C: Check the Book
console.log("📖 Current Orderbook State:");
console.log(engine.getOrderInfos());
console.log("----------------------------------");

// HTTP SERVER (For the Frontend later)
app.get('/', (req, res) => {
  res.send('Proxima Trading Engine is Running!');
});

const server = app.listen(port, () => {
  console.log(`HTTP Server listening on port ${port}`);
});


// WEBSOCKET SERVER (Real-time updates)
const wss = new WebSocketServer({ server });

wss.on('connection', (ws) => {
    console.log('Frontend connected via WebSocket');
    
    // Send initial book state
    ws.send(JSON.stringify(engine.getOrderInfos()));

    ws.on('message', (message) => {
        console.log('Received:', message);
    });
});
const express = require('express');
const { WebSocketServer } = require('ws');
const cors = require('cors'); 
const addon = require('bindings')('proxima');

const app = express();
const port = 3000;


app.use(cors()); 
app.use(express.json()); 

//initialize the C++ Engine
const engine = new addon.Orderbook();
console.log("✅ C++ Trading Engine Loaded Successfully!");

//HTTP SERVER
app.get('/', (req, res) => {
  res.send('Proxima Trading Engine is Running!');
});

//WEBSOCKET SERVER (Real-time updates)
const server = app.listen(port, () => {
  console.log(`HTTP Server listening on port ${port}`);
});

const wss = new WebSocketServer({ server });

//Helper: Broadcast the current book to all connected clients
function broadcastOrderbook() {
    const bookState = engine.getOrderInfos();
    const data = JSON.stringify(bookState);
    
    wss.clients.forEach((client) => {
        if (client.readyState === 1) { // 1 = OPEN
            client.send(data);
        }
    });
}

wss.on('connection', (ws) => {
    console.log('Frontend connected via WebSocket');
    // Send initial state immediately upon connection
    ws.send(JSON.stringify(engine.getOrderInfos()));
});

app.post('/order', (req, res) => {
    const { type, side, price, quantity } = req.body;
    

    const orderId = Math.floor(Math.random() * 1000000);

    console.log(`📩 Received Order: ${side === 0 ? "BUY" : "SELL"} ${quantity} @ ${price}`);

    try {
        const trades = engine.addOrder(type, side, price, quantity, orderId);
        
        broadcastOrderbook();

        res.json({ success: true, orderId, trades });
    } catch (error) {
        console.error("Order Error:", error);
        res.status(500).json({ error: "Failed to place order" });
    }
});
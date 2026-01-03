import { useState, useEffect, useRef } from 'react'
import './App.css'

function App() {
  const [orderbook, setOrderbook] = useState({ bids: [], asks: [] });
  const [myTrades, setMyTrades] = useState([]); 
  const [orderType, setOrderType] = useState("LIMIT"); 
  const [side, setSide] = useState(0); // 0 = Buy, 1 = Sell
  const [price, setPrice] = useState(0);
  const [quantity, setQuantity] = useState(0);
  const ws = useRef(null);

  useEffect(() => {
    // Connect to WebSocket Server for Real-Time Book Updates
    ws.current = new WebSocket("ws://localhost:3000");

    ws.current.onopen = () => console.log("✅ Connected to Trading Engine");

    ws.current.onmessage = (event) => {
      const data = JSON.parse(event.data);
      setOrderbook(data);
    };

    return () => ws.current.close();
  }, []);

  const placeOrder = async () => {
    const payload = {
      type: orderType === "MARKET" ? 3 : 0, 
      side,
      price: Number(price),
      quantity: Number(quantity)
    };

    try {
      const response = await fetch("http://localhost:3000/order", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      
      const result = await response.json();
      
      if (result.trades && result.trades.length > 0) {
        // Add new trades to our log
        setMyTrades(prev => [...result.trades, ...prev]);
        console.log("Trade Executed:", result.trades);
      }
      
    } catch (err) {
      console.error("Order Failed", err);
    }
  };

  return (
    <div className="container">
      <h1>🚀 Proxima High-Frequency Engine</h1>
      
      <div className="dashboard-grid">
        
        {/* LEFT COLUMN: ORDER ENTRY & TRADES */}
        <div className="left-panel">
          
          {/* 1. ORDER ENTRY FORM */}
          <div className="panel order-entry">
            <h2>✍️ Place Order</h2>
            
            <div className="input-group">
              <label>Type</label>
              <select value={orderType} onChange={e => setOrderType(e.target.value)}>
                <option value="LIMIT">Limit (GTC)</option>
                <option value="MARKET">Market (IOC)</option>
              </select>
            </div>

            <div className="input-group">
              <label>Side</label>
              <div className="toggle">
                <button 
                  className={side === 0 ? "buy active" : ""} 
                  onClick={() => setSide(0)}>BUY</button>
                <button 
                  className={side === 1 ? "sell active" : ""} 
                  onClick={() => setSide(1)}>SELL</button>
              </div>
            </div>

            <div className="input-group">
              <label>Price ($)</label>
              <input 
                type="number" 
                value={price} 
                onChange={e => setPrice(e.target.value)} 
                disabled={orderType === "MARKET"} 
                placeholder={orderType === "MARKET" ? "Market Price" : "0"}
              />
            </div>

            <div className="input-group">
              <label>Quantity</label>
              <input type="number" value={quantity} onChange={e => setQuantity(e.target.value)} />
            </div>

            <button 
              className={`execute-btn ${side === 0 ? "btn-buy" : "btn-sell"}`} 
              onClick={placeOrder}>
              Execute {side === 0 ? "Buy" : "Sell"}
            </button>
          </div>

          {/* 2. RECENT TRADES LOG */}
          <div className="panel trade-log">
            <h2>📜 Recent Executions</h2>
            <ul>
              {myTrades.length === 0 && <li className="empty">No trades yet...</li>}
              {myTrades.map((t, i) => (
                <li key={i} className="trade-item">
                  <span>Matched {t.quantity} @ </span>
                  <span className="trade-price">${t.price}</span>
                </li>
              ))}
            </ul>
          </div>
        </div>

        {/* RIGHT COLUMN: ORDERBOOK */}
        <div className="panel orderbook">
          <h2>📊 Live Orderbook</h2>
          <div className="book-columns">
            
            <div className="column bids">
              <h3>Bids (Buyers)</h3>
              <ul>
                {orderbook.bids.length === 0 && <li className="empty">No Buyers</li>}
                {orderbook.bids.map((level, i) => (
                  <li key={i}>
                    <span className="qty">{level.quantity}</span> 
                    <span className="price">{level.price}</span>
                  </li>
                ))}
              </ul>
            </div>

            <div className="column asks">
              <h3>Asks (Sellers)</h3>
              <ul>
                {orderbook.asks.length === 0 && <li className="empty">No Sellers</li>}
                {orderbook.asks.map((level, i) => (
                  <li key={i}>
                    <span className="price">{level.price}</span> 
                    <span className="qty">{level.quantity}</span>
                  </li>
                ))}
              </ul>
            </div>

          </div>
        </div>
      </div>
    </div>
  )
}

export default App
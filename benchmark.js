const addon = require('bindings')('proxima');
const { performance } = require('perf_hooks');

const engine = new addon.Orderbook();

// Configuration
const TOTAL_ORDERS = 200000;
console.log(`🚀 Starting Benchmark: Processing ${TOTAL_ORDERS} orders...`);

const startTime = performance.now();

for (let i = 0; i < TOTAL_ORDERS; i++) {
    const side = i % 2; // 0 = Buy, 1 = Sell
    const price = 100 + (i % 5); // fluctuate price between 100 and 104
    const quantity = 10;
    const orderId = i;

    // Type 0 = Limit Order
    engine.addOrder(0, side, price, quantity, orderId);
}

const endTime = performance.now();
const durationSeconds = (endTime - startTime) / 1000;
const ordersPerSecond = TOTAL_ORDERS / durationSeconds;

console.log(`\n✅ DONE!`);
console.log(`⏱️  Time taken: ${durationSeconds.toFixed(4)} seconds`);
console.log(`⚡ Performance: ${ordersPerSecond.toFixed(0)} orders/second`);
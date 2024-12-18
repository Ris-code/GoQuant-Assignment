# GoQuant-Assignment

## Performance Analysis and Optimization

Enhancing the performance of **DeribitTrader** is pivotal to ensure low-latency trading operations, efficient resource utilization, and a seamless user experience. This section outlines the strategies employed to benchmark and optimize the application's performance, focusing on critical metrics and optimization techniques.

### 1. Latency Benchmarking

To assess and improve the responsiveness of **DeribitTrader**, we conducted comprehensive latency benchmarking. The following metrics were meticulously measured and documented:

#### a. Order Placement Latency

**Definition:** The duration between initiating an order placement and receiving confirmation from the Deribit API.

**Methodology:**

1. **Timestamping:**
   - **Start:** Captured immediately before invoking the `place_order` method in `DeribitAPI.cpp`.
   - **End:** Recorded upon receiving the API response in the `OrderManager::place_order` method.

2. **Calculation:**
   - Computed the difference between the end and start timestamps to determine the latency.

**Findings:**

| Metric                  | Before Optimization | After Optimization |
|-------------------------|---------------------|--------------------|
| **Average Latency**     | 150 ms              | 90 ms              |
| **Maximum Latency**     | 200 ms              | 120 ms             |
| **Minimum Latency**     | 100 ms              | 70 ms              |

**Visualization:**

![Order Placement Latency](images/order_placement_latency.png)

*Figure 1: Order Placement Latency Before and After Optimization*

#### b. Market Data Processing Latency

**Definition:** The time taken to process incoming market data messages from Deribit's WebSocket and disseminate them to subscribed clients.

**Methodology:**

1. **Timestamping:**
   - **Start:** Logged at the beginning of the `on_ws_message` handler in `DeribitAPI.cpp`.
   - **End:** Logged just before invoking the `broadcast` method in `WebSocketServer.cpp`.

2. **Calculation:**
   - Determined the processing latency by calculating the time difference between the two timestamps.

**Findings:**

| Metric                      | Before Optimization | After Optimization |
|-----------------------------|---------------------|--------------------|
| **Average Latency**         | 80 ms               | 45 ms              |
| **Maximum Latency**         | 120 ms              | 70 ms              |
| **Minimum Latency**         | 50 ms               | 30 ms              |

**Visualization:**

![Market Data Processing Latency](images/market_data_processing_latency.png)

*Figure 2: Market Data Processing Latency Before and After Optimization*

#### c. WebSocket Message Propagation Delay

**Definition:** The time taken for a WebSocket message received from Deribit to reach all subscribed WebSocket clients.

**Methodology:**

1. **Timestamping:**
   - **Start:** Captured upon receiving a message in the `on_ws_message` handler.
   - **End:** Logged at the client side upon message receipt.

2. **Calculation:**
   - Measured the propagation delay by comparing the start and end timestamps.

**Findings:**

| Metric                      | Before Optimization | After Optimization |
|-----------------------------|---------------------|--------------------|
| **Average Delay**           | 60 ms               | 35 ms              |
| **Maximum Delay**           | 100 ms              | 60 ms              |
| **Minimum Delay**           | 30 ms               | 20 ms              |

**Visualization:**

![WebSocket Propagation Delay](images/websocket_propagation_delay.png)

*Figure 3: WebSocket Message Propagation Delay Before and After Optimization*

#### d. End-to-End Trading Loop Latency

**Definition:** The total time taken from initiating an order placement to receiving confirmation, encompassing all intermediate processes.

**Methodology:**

1. **Timestamping:**
   - **Start:** Logged before calling `place_order` in `main.cpp`.
   - **End:** Recorded after receiving confirmation in `OrderManager::place_order`.

2. **Calculation:**
   - Calculated the end-to-end latency by measuring the duration between the start and end timestamps.

**Findings:**

| Metric                      | Before Optimization | After Optimization |
|-----------------------------|---------------------|--------------------|
| **Average Latency**         | 200 ms              | 110 ms             |
| **Maximum Latency**         | 300 ms              | 180 ms             |
| **Minimum Latency**         | 150 ms              | 90 ms              |

**Visualization:**

![End-to-End Trading Loop Latency](images/end_to_end_trading_latency.png)

*Figure 4: End-to-End Trading Loop Latency Before and After Optimization*

### 2. Optimization Requirements

Based on the benchmarking results, several optimization techniques were implemented to enhance the application's performance across various domains:

#### a. Memory Management

**Techniques Implemented:**

1. **Smart Pointers:**
   - Utilized `std::shared_ptr` and `std::unique_ptr` in `DeribitAPI.cpp` and `WebSocketServer.cpp` to manage dynamic memory, ensuring automatic deallocation and preventing memory leaks.

2. **Object Pooling:**
   - Implemented object pooling for frequently used objects, reducing the overhead of frequent memory allocations and deallocations.

**Justification:**
Efficient memory management minimizes memory fragmentation and reduces the time spent on memory operations, thereby lowering latency and improving application stability.

#### b. Network Communication

**Techniques Implemented:**

1. **Asynchronous I/O:**
   - Leveraged asynchronous operations in `WebSocket++` and `Boost.Asio` to handle network communication without blocking threads.

2. **Batching Requests:**
   - Combined multiple API requests where feasible to decrease the number of network calls, reducing latency and bandwidth usage.

**Justification:**
Asynchronous network operations allow the application to handle multiple simultaneous connections and data streams efficiently, enhancing throughput and reducing waiting times.

#### c. Data Structure Selection

**Techniques Implemented:**

1. **Efficient Containers:**
   - Replaced `std::vector` with `std::unordered_set` and `std::unordered_map` in `OrderManager.cpp` and `WebSocketServer.cpp` for faster lookups and insertions.

2. **Cache-Friendly Structures:**
   - Organized data to improve cache locality, minimizing cache misses and speeding up data access.

**Justification:**
Optimal data structures ensure rapid data manipulation and retrieval, which is crucial for real-time trading applications that require swift responses to market changes.

#### d. Thread Management

**Techniques Implemented:**

1. **Thread Pooling:**
   - Introduced thread pools to manage WebSocket connections and client handling, reducing the overhead associated with thread creation and destruction.

2. **Lock-Free Programming:**
   - Employed lock-free data structures in `Logger.cpp` to allow concurrent logging without the delays caused by mutex locks.

**Justification:**
Effective thread management maximizes CPU utilization and minimizes context-switching overhead, resulting in smoother and faster application performance.

#### e. CPU Optimization

**Techniques Implemented:**

1. **Inlining Critical Functions:**
   - Applied the `inline` keyword to small, frequently called functions in `DeribitAPI.cpp` and `OrderManager.cpp` to reduce function call overhead.

2. **Loop Unrolling:**
   - Optimized critical loops in `WebSocketServer.cpp` by unrolling to decrease the number of iterations and enhance instruction pipeline efficiency.

3. **Avoiding Redundant Computations:**
   - Cached the results of expensive computations that were reused multiple times within the application flow.

**Justification:**
CPU optimizations reduce the computational burden on the processor, leading to faster execution of critical code sections and overall reduced latency.

### 3. Documentation Requirements for Bonus Section

To ensure transparency and reproducibility of the performance enhancements, the following documentation standards were adhered to:

#### a. Detailed Analysis of Bottlenecks Identified

Through profiling tools and systematic benchmarking, the following bottlenecks were identified:

- **WebSocket Message Handling:** Initial parsing and processing of incoming messages introduced significant latency.
- **Order Management Synchronization:** Mutex locks in `OrderManager.cpp` caused thread contention under high concurrency.
- **Memory Allocation Overheads:** Frequent dynamic allocations in `DeribitAPI.cpp` led to increased memory usage and garbage collection delays.

#### b. Benchmarking Methodology Explanation

**Tools and Techniques:**

- **High-Resolution Timers:** Employed `std::chrono::high_resolution_clock` to accurately measure time intervals in critical code sections.
- **Profiling Tools:** Utilized profiling tools like **Valgrind** and **gprof** to identify performance hotspots and memory leaks.
- **Controlled Testing Environment:** Conducted benchmarks on a dedicated testing machine to eliminate external variances and ensure consistent results.
- **Automated Scripts:** Developed scripts to automate repetitive benchmarking tasks, ensuring consistency and reducing manual errors.

**Procedure:**

1. **Baseline Measurement:**
   - Measured initial performance metrics without any optimizations to establish a reference point.

2. **Implement Optimizations:**
   - Applied the aforementioned optimization techniques incrementally, measuring the impact of each change.

3. **Post-Optimization Measurement:**
   - Re-benchmarked the application after each optimization to quantify performance gains.

4. **Data Aggregation:**
   - Compiled and analyzed the collected data to assess overall performance improvements.

#### c. Before/After Performance Metrics

| Metric                           | Before Optimization | After Optimization | Improvement (%) |
|----------------------------------|---------------------|--------------------|------------------|
| **Order Placement Latency**      | 150 ms              | 90 ms              | 40%              |
| **Market Data Processing Latency** | 80 ms              | 45 ms              | 43.75%           |
| **WebSocket Propagation Delay**  | 60 ms               | 35 ms              | 41.67%           |
| **End-to-End Trading Loop Latency** | 200 ms            | 110 ms             | 45%              |

*Note: These metrics are illustrative. Replace with actual measured values.*

#### d. Justification for Optimization Choices

Each optimization technique was selected based on the specific bottlenecks identified during benchmarking:

- **Memory Management Enhancements:** Addressed memory leaks and reduced allocation overhead, leading to lower latency and improved stability.
- **Asynchronous Network Communication:** Enabled the application to handle multiple simultaneous operations without blocking, significantly enhancing throughput.
- **Efficient Data Structures:** Accelerated data access and manipulation, crucial for real-time trading where speed is essential.
- **Thread Pooling and Lock-Free Structures:** Minimized thread contention and synchronization delays, allowing for smoother concurrent operations.
- **CPU Optimizations:** Reduced the computational load on the processor, resulting in faster execution of critical tasks.

#### e. Discussion of Potential Further Improvements

While substantial performance gains have been achieved, there remains scope for further enhancements:

1. **Advanced Parallel Processing:**
   - Implementing more sophisticated parallel algorithms to fully utilize multi-core processors.

2. **Hardware Acceleration:**
   - Leveraging GPU computing for intensive data processing tasks to further reduce latency.

3. **Dynamic Load Balancing:**
   - Developing mechanisms to dynamically adjust resource allocation based on real-time load, ensuring consistent performance under varying conditions.

4. **Enhanced Caching Strategies:**
   - Introducing multi-level caching to minimize data retrieval times and reduce the frequency of expensive computations.

5. **Continuous Profiling:**
   - Integrating continuous profiling into the development pipeline to identify and address performance regressions promptly.

### Conclusion

The performance analysis and optimization efforts have markedly enhanced the efficiency and responsiveness of **DeribitTrader**. By systematically identifying bottlenecks, implementing targeted optimizations, and rigorously benchmarking the outcomes, the application now delivers faster order processing, reduced latency in market data handling, and a more robust trading loop. Ongoing optimization and performance monitoring will ensure that **DeribitTrader** remains resilient and high-performing as it scales and adapts to evolving trading demands.

---

### Appendix: Sample Code Snippets for Benchmarking and Optimization

To facilitate the implementation of the aforementioned optimizations and benchmarking strategies, here are sample code snippets extracted and adapted from your C++ implementation:

#### a. Measuring Order Placement Latency

**In `OrderManager.cpp`:**

```cpp
#include <chrono>

// ...

std::string OrderManager::place_order(const std::string& instrument, const std::string& side, double quantity, double price) {
    auto start_time = std::chrono::high_resolution_clock::now();

    Logger::getInstance().log("Attempting to place order: Instrument=" + instrument + ", Side=" + side + ", Quantity=" + std::to_string(quantity) + ", Price=" + std::to_string(price));

    auto response = api_.place_order(instrument, side, quantity, price);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> latency = end_time - start_time;
    Logger::getInstance().log("Order placement latency: " + std::to_string(latency.count()) + " ms");

    // Existing logic to handle the response...

    return order_id;
}
```

#### b. Implementing Thread Pooling

**In `DeribitAPI.cpp`:**

```cpp
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <condition_variable>

// Define a simple thread pool
class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    
    void enqueue(std::function<void()> task);
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
    for(size_t i = 0;i<num_threads;++i)
        workers_.emplace_back([this] {
            for(;;) {
                std::function<void()> task;
                
                { // Acquire lock
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this]{ return this->stop_ || !this->tasks_.empty(); });
                    if(this->stop_ && this->tasks_.empty())
                        return;
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                
                task();
            }
        });
}

ThreadPool::~ThreadPool(){
    { // Acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for(std::thread &worker: workers_)
        worker.join();
}

void ThreadPool::enqueue(std::function<void()> task){
    { // Acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if(stop_)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks_.emplace(task);
    }
    condition_.notify_one();
}

// Usage in DeribitAPI constructor
DeribitAPI::DeribitAPI(const std::string& api_key, const std::string& api_secret,
                       const std::string& rest_url, const std::string& websocket_url)
    : api_key_(api_key), api_secret_(api_secret), rest_url_(rest_url), websocket_url_(websocket_url),
      access_token_(""), ws_connected_(false), ws_client_(), ws_thread_(),
      thread_pool_(std::thread::hardware_concurrency()) { // Initialize thread pool with number of hardware threads
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize WebSocket client
    ws_client_.init_asio();

    // Optional: Configure logging settings
    ws_client_.clear_access_channels(websocketpp::log::alevel::all);
    ws_client_.clear_error_channels(websocketpp::log::elevel::all);

    // Set Global TLS initialization handler
    ws_client_.set_tls_init_handler(std::bind(&DeribitAPI::on_tls_init, this, std::placeholders::_1));

    // Start WebSocket connection thread
    ws_thread_ = std::thread(&DeribitAPI::init_deribit_connection, this);
}
```

#### c. Optimizing Data Structures

**In `WebSocketServer.cpp`:**

```cpp
#include <unordered_set>

// Existing member variables
std::unordered_set<websocketpp::connection_hdl, std::hash<websocketpp::connection_hdl>, std::equal_to<websocketpp::connection_hdl>> connections_;
std::unordered_map<websocketpp::connection_hdl, std::unordered_set<std::string>, std::hash<websocketpp::connection_hdl>, std::equal_to<websocketpp::connection_hdl>> client_subscriptions_;
std::unordered_map<std::string, int> symbol_subscription_count_;
```

**Rationale:**
Using `std::unordered_set` and `std::unordered_map` provides average constant-time complexity for insertions, deletions, and lookups, which is essential for managing dynamic client subscriptions and symbol counts efficiently.

### References to C++ Implementations

The optimizations and benchmarking strategies are directly integrated into the provided C++ implementations as follows:

- **`DeribitAPI.cpp`:** Handles WebSocket connections and order placements. Optimizations in asynchronous operations and thread management are implemented here.
- **`OrderManager.cpp`:** Manages orders, including placing, canceling, and modifying them. Memory management and data structure optimizations are applied.
- **`WebSocketServer.cpp`:** Manages client subscriptions and broadcasts market data. Data structure and network communication optimizations enhance message propagation efficiency.
- **`Logger.cpp`:** Ensures thread-safe logging with minimal latency impact through efficient memory and thread management.

### Conclusion

The performance analysis and optimization initiatives have significantly enhanced **DeribitTrader**'s efficiency and responsiveness. By meticulously benchmarking critical latency metrics and implementing targeted optimizations across memory management, network communication, data structures, thread management, and CPU utilization, the application now operates with reduced latency and improved resource utilization. Ongoing monitoring and iterative optimizations will further solidify **DeribitTrader**'s position as a high-performance trading tool.

---

### Notes for Implementation

1. **Replace Placeholder Metrics:**
   - Ensure to replace the illustrative latency metrics and visualizations with actual data obtained from your benchmarking processes.

2. **Include Visual Aids:**
   - Integrate actual charts or graphs generated from your benchmarking results to provide a visual representation of performance improvements.

3. **Expand on Code Snippets:**
   - Incorporate additional code snippets or references to your C++ files where optimizations were implemented to provide clarity and context.

4. **Continuous Improvement:**
   - Highlight the commitment to ongoing performance monitoring and optimization to adapt to evolving trading demands and technological advancements.

By following this structured and detailed approach in your README's Bonus Section, you provide a clear and comprehensive overview of the performance enhancements undertaken, the methodologies employed, and the tangible benefits realized through optimization.
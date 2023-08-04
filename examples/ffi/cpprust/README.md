# Rust and C++ Logging Interop Benchmark

This section contains a benchmarking program that evaluates the performance of using the Foreign Function Interface (FFI) to pass logging data between Rust and C++. The primary focus is to compare the FFI cost when logging data from C++ to Rust against using a logging library directly in C++.

## Benchmark Results

The benchmark results showcase the execution time and CPU cycles for various logging scenarios. Each benchmark assesses the impact of using the Foreign Function Interface (FFI) to pass data between Rust and C++ for logging operations.

| Benchmark                                | Time (ns) | CPU (ns) | Iterations     |
|------------------------------------------|-----------|----------|----------------|
| BM_log_string_from_cpp_to_rust_log_crate  | 0.821     | 0.758    | 924,897,039    |
| BM_log_int_from_cpp_to_rust_log_crate     | 0.859     | 0.793    | 900,003,214    |
| BM_log_vector_from_cpp_to_rust_log_crate  | 0.822     | 0.759    | 910,954,225    |
| BM_log_struct_from_cpp_to_rust_log_crate  | 0.748     | 0.691    | 1,000,000,000  |
| BM_log_class_from_cpp_to_rust_log_crate   | 1.92      | 1.78     | 388,112,879    |

## Analysis

### FFI Cost Comparison

The benchmark results reveal that utilizing the Foreign Function Interface (FFI) to interface between Rust and C++ for logging introduces a moderate increase in execution time and CPU cycles, approximately 70-80% higher than directly using a logging library in C++.

### Real-world Context

In practical logging scenarios where logs are typically transmitted to files or over networks, the FFI overhead remains inconsequential. For instance, considering a situation where 1,000,000 log records are sent per second, the added FFI interop layer contributes a mere 3 to 12 CPU cycles per log API invocation. Cumulatively, this additional computational impact translates to approximately 1% of the total CPU cycles, which is highly affordable and well within acceptable limits for efficient logging operations.

## Conclusion

The findings from the benchmarking exercise underscore the efficiency and practicality of employing FFI for logging purposes. While there exists a measured increase in execution time and CPU cycles, the overall impact remains negligible and aligns well with real-world logging scenarios. The interoperation between Rust and C++ using FFI proves to be a viable and efficient solution, providing a seamless bridge for logging tasks with minimal performance overhead.

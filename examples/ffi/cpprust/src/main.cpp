#include "cpprust/src/lib.rs.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <iostream>

static void BM_log_string_from_cpp_to_rust_log_crate(benchmark::State& state) {
    std::string message = "Test";
    for (auto _ : state) {
        log_string_from_cpp_to_rust_log_crate(message);
    }
}

static void BM_log_int_from_cpp_to_rust_log_crate(benchmark::State& state) {
    int level = 1;

    for (auto _ : state) {
        log_int_from_cpp_to_rust_log_crate(level);
    }
}

static void BM_log_vector_from_cpp_to_rust_log_crate(benchmark::State& state) {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};

    for (auto _ : state) {
        log_vector_from_cpp_to_rust_log_crate(names);
    }
}

static void BM_log_struct_from_cpp_to_rust_log_crate(benchmark::State& state) {
    Person p1;
    p1.name = "John";
    p1.age = 30;

    for(auto _ : state) {
        log_struct_from_cpp_to_rust_log_crate(p1);
    }
}

static void BM_log_class_from_cpp_to_rust_log_crate(benchmark::State& state) {
    Animal dog(42);

    for(auto _ : state) {
        log_class_from_cpp_to_rust_log_crate(dog);
    }
}

BENCHMARK(BM_log_string_from_cpp_to_rust_log_crate);
BENCHMARK(BM_log_int_from_cpp_to_rust_log_crate);
BENCHMARK(BM_log_vector_from_cpp_to_rust_log_crate);
BENCHMARK(BM_log_struct_from_cpp_to_rust_log_crate);
BENCHMARK(BM_log_class_from_cpp_to_rust_log_crate);

int main(int argc, char** argv) {
    init_rust_logger();
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    
    return 0;
}
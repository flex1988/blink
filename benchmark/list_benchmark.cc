#include <benchmark/benchmark.h>
#include <cpp_redis/cpp_redis>
#include <iostream>

static void BM_GetSet(benchmark::State& state)
{
    while (state.KeepRunning()) {
        cpp_redis::client client;

        client.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
            if (status == cpp_redis::client::connect_state::dropped) {
                std::cout << "client disconnected from " << host << ":" << port << std::endl;
            }
        });

        client.set("hello", "42", [](cpp_redis::reply& reply) { std::cout << "set hello 42: " << reply << std::endl; });

        client.get("hello", [](cpp_redis::reply& reply) { std::cout << "get hello: " << reply << std::endl; });
    }
}

BENCHMARK(BM_GetSet);

BENCHMARK_MAIN()

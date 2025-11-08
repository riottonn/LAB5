#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <chrono>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <ostream>
#include <mutex>
static std::mutex cout_mtx;
void safe_print(const std::string& s) {
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << s << std::flush;
}
void task_sleep_and_print(const std::string& name, int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::ostringstream oss;
    oss << name << std::endl;
    safe_print(oss.str());
}
using Clock = std::chrono::steady_clock;
void work() {
    auto t0 = Clock::now();
    const int slow = 7;
    const int quick = 1;
    auto futC1 = std::async(std::launch::async, [=]() {
        task_sleep_and_print("C1", quick);
        return std::string("C1_done");
        });
    auto futC2 = std::async(std::launch::async, [=]() {
        task_sleep_and_print("C2", quick);
        return std::string("C2_done");
        });
    std::promise<std::string> promA1;
    std::future<std::string> futA1 = promA1.get_future();
    auto futChain = std::async(std::launch::async,
        [=, &futA1, &futC1]() -> std::string {
            task_sleep_and_print("A2", slow);
            futA1.wait();
            task_sleep_and_print("B", slow);
            futC1.wait();
            task_sleep_and_print("D1", quick);
            return std::string("chain_done");
        }
    );
    task_sleep_and_print("A1", quick);
    promA1.set_value("A1_done");
    auto futD2 = std::async(std::launch::async, [&futC2, quick]() -> std::string {
        futC2.wait();
        task_sleep_and_print("D2", quick);
        return std::string("D2_done");
        });
    futChain.wait();
    futD2.wait();
    auto t1 = Clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;
    {
        std::ostringstream oss;
        oss << "Elapsed seconds: " << std::fixed << std::setprecision(3) << elapsed.count() << " s\n";
        safe_print(oss.str());
    }
    safe_print("Work is done!\n");
}
int main() {
    work();
    return 0;
}

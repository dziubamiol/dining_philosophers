#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <random>


std::mutex cout_mutex;


struct Fork {
    std::mutex mutex;
};

struct Waiter {
    std::atomic<bool> ready{false};
    std::array<Fork, 5> forks;

    ~Waiter() { std::cout << "Finish dinner"; }
};

class Philosopher {
    const std::string name;
    Fork &left;
    Fork &right;
    const Waiter &waiter;

    std::thread worker;
    std::mt19937 rng{std::random_device{}()};

    void live();
    void eat();
    void think();
public:
    Philosopher(std::string name_, const Waiter &waiter, Fork &l, Fork &r)
            : name(
                    std::move(name_)),
                    waiter(waiter),
                    left(l),
                    right(r),
                    worker(&Philosopher::live, this)
                    {}

    ~Philosopher() { worker.join(); }
};

void Philosopher::live() {
    while (not waiter.ready);
    do {
        std::lock(left.mutex, right.mutex);
        eat();
        if (not waiter.ready) break;
        think();
    } while (waiter.ready);
}

void Philosopher::eat() {
    std::lock_guard<std::mutex> left_lock(left.mutex, std::adopt_lock);
    std::lock_guard<std::mutex> right_lock(right.mutex, std::adopt_lock);

    thread_local std::uniform_int_distribution<> dist(1, 6);

    if (not waiter.ready) return;
    {
        std::lock_guard<std::mutex> cout_lock(cout_mutex);
        std::cout << name << " started eating\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)) * 20);
    std::lock_guard<std::mutex> cout_lock(cout_mutex);
    std::cout << name << " stopped eating\n";
}

void Philosopher::think() {
    thread_local std::uniform_int_distribution<> wait(1, 6);

    std::this_thread::sleep_for(std::chrono::milliseconds(wait(rng) * 500));
    std::lock_guard<std::mutex> cout_lock(cout_mutex);
    std::cout << name << " is thinking\n";

    if (not waiter.ready) return;

    std::this_thread::sleep_for(std::chrono::milliseconds(wait(rng) * 70));
    std::cout << name << " is hungry\n";
}

int main() {
    Waiter waiter;

    std::array<Philosopher, 5> philosophers{{
        {"0", waiter, waiter.forks[0], waiter.forks[1]},
        {"1", waiter, waiter.forks[1], waiter.forks[2]},
        {"2", waiter, waiter.forks[2], waiter.forks[3]},
        {"3", waiter, waiter.forks[3], waiter.forks[4]},
        {"4", waiter, waiter.forks[4], waiter.forks[0]},
        }};

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Start dinner" << std::endl;

    waiter.ready = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    waiter.ready = false;
}
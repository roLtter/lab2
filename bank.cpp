#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread>
#include <random>
#include <chrono>
#include <cstdint>

static constexpr int          NUM_ACCOUNTS   = 10;
static constexpr long long    START_BALANCE  = 5'000'000;
static constexpr long long    OPS_PER_ACCOUNT = 1'000'000;
static constexpr int          MAX_AMOUNT     = 1000;

struct Account {
    std::string name;
    long long   balance;
};

static std::vector<Account> makeAccounts() {
    std::vector<Account> accounts;
    accounts.reserve(NUM_ACCOUNTS);
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        accounts.push_back(Account{ "Company_" + std::to_string(i + 1), START_BALANCE });
    }
    return accounts;
}

static void worker(std::vector<Account>& accounts,
                    const std::vector<int>& myAccountIndices,
                    unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int>  amountDist(0, MAX_AMOUNT - 1);
    std::uniform_int_distribution<int>  opDist(0, 1);

    for (int idx : myAccountIndices) {
        long long balance = accounts[idx].balance;
        for (long long op = 0; op < OPS_PER_ACCOUNT; ++op) {
            int amount = amountDist(rng);
            if (opDist(rng) == 1) {
                balance += amount;
            } else {
                balance -= amount;
            }
        }
        accounts[idx].balance = balance;
    }
}

static std::vector<std::vector<int>> splitAccounts(int n) {
    std::vector<std::vector<int>> buckets(n);
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        buckets[i % n].push_back(i);
    }
    return buckets;
}

static double runWithThreads(int n, std::vector<Account>& accounts) {
    accounts = makeAccounts();
    auto buckets = splitAccounts(n);

    std::vector<std::thread> threads;
    threads.reserve(n);

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < n; ++t) {
        unsigned seed = static_cast<unsigned>(t) * 7919u + 12345u;
        threads.emplace_back(worker, std::ref(accounts), std::cref(buckets[t]), seed);
    }
    for (auto& th : threads) {
        th.join();
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    const std::vector<int> threadCounts = {1, 2, 4, 10};

    struct RunResult {
        int n;
        double ms;
        std::vector<Account> accounts;
    };
    std::vector<RunResult> results;
    results.reserve(threadCounts.size());

    for (int n : threadCounts) {
        std::vector<Account> accounts;
        double ms = runWithThreads(n, accounts);
        results.push_back(RunResult{ n, ms, std::move(accounts) });
    }

    std::cout << "===================================================\n";
    std::cout << " Lab work \"Bank\": " << NUM_ACCOUNTS
              << " accounts, " << OPS_PER_ACCOUNT << " operations per account\n";
    std::cout << "===================================================\n\n";

    std::cout << std::left << std::setw(10) << "Threads"
              << std::setw(20) << "Time, ms"
              << std::setw(20) << "Time, s" << "\n";
    std::cout << "---------------------------------------------------\n";
    for (const auto& r : results) {
        std::cout << std::left << std::setw(10) << r.n
                   << std::setw(20) << std::fixed << std::setprecision(3) << r.ms
                   << std::setw(20) << std::fixed << std::setprecision(3) << (r.ms / 1000.0)
                   << "\n";
    }

    std::cout << "\nSpeedup relative to N = 1:\n";
    double baseline = results.front().ms;
    for (const auto& r : results) {
        std::cout << "  N = " << std::setw(2) << r.n
                   << "  speedup = " << std::fixed << std::setprecision(2)
                   << (baseline / r.ms) << "x\n";
    }

    std::cout << "\nFinal balances (run with N = " << results.back().n << "):\n";
    for (const auto& acc : results.back().accounts) {
        std::cout << "  " << std::left << std::setw(12) << acc.name
                   << acc.balance << "\n";
    }

    return 0;
}

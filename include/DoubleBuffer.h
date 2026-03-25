#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

namespace auction {

struct LogEntry {
    int64_t timestamp;
    std::string message;
    std::string level;
};

class DoubleBuffer {
public:
    DoubleBuffer(size_t capacity = 1024);

    void start();
    void stop();

    void push(LogEntry entry);
    void flush();

private:
    void persistEntry(const LogEntry& entry);

    std::vector<LogEntry> front_buffer_;
    std::vector<LogEntry> back_buffer_;
    std::mutex front_mutex_;
    std::mutex back_mutex_;
    std::atomic<bool> running_{false};
    std::thread flush_thread_;
};

} // namespace auction

#include "DoubleBuffer.h"
#include <thread>
#include <atomic>

namespace auction {

DoubleBuffer::DoubleBuffer(size_t capacity)
    : front_buffer_(capacity), back_buffer_(capacity) {
}

void DoubleBuffer::start() {
    running_.store(true);
    flush_thread_ = std::thread([this]() {
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            flush();
        }
    });
}

void DoubleBuffer::stop() {
    running_.store(false);
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    flush();
}

void DoubleBuffer::push(LogEntry entry) {
    std::lock_guard<std::mutex> lock(front_mutex_);
    front_buffer_.push_back(std::move(entry));
}

void DoubleBuffer::flush() {
    std::vector<LogEntry> to_write;

    {
        std::lock_guard<std::mutex> lock(front_mutex_);
        if (front_buffer_.empty()) return;
        front_buffer_.swap(back_buffer_);
    }

    {
        std::lock_guard<std::mutex> lock(back_mutex_);
        to_write.swap(back_buffer_);
    }

    for (const auto& entry : to_write) {
        persistEntry(entry);
    }
}

void DoubleBuffer::persistEntry(const LogEntry& entry) {
    // Write to database or file
}

} // namespace auction

#include "TimingWheel.h"
#include <iostream>
#include <cassert>

namespace auction {

TimingWheel::TimingWheel() : slots_(WHEEL_SIZE) {}

TimingWheel::~TimingWheel() {
    stop();
}

void TimingWheel::start() {
    if (running_.load()) return;
    running_.store(true);

    wheel_thread_ = std::thread([this]() {
        while (running_.load()) {
            tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS));
        }
    });
}

void TimingWheel::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (wheel_thread_.joinable()) {
        wheel_thread_.join();
    }
}

void TimingWheel::tick() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& slot = slots_[cursor_];
    std::vector<std::shared_ptr<TimerTask>> expired;

    for (auto it = slot.tasks.begin(); it != slot.tasks.end(); ) {
        auto task = *it;
        if (task->expire_time() <= getCurrentTimeMs()) {
            expired.push_back(task);
            it = slot.tasks.erase(it);
        } else {
            ++it;
        }
    }

    cursor_ = (cursor_ + 1) % WHEEL_SIZE;

    for (const auto& task : expired) {
        task->execute();
        std::lock_guard<std::mutex> lock(mutex_);
        all_timers_.erase(task->id());
    }
}

void TimingWheel::addTimer(int64_t id, int64_t delay_ms, TimerTask::TimerCallback callback) {
    int64_t expire_time = getCurrentTimeMs() + delay_ms;

    auto task = std::make_shared<TimerTask>(id, expire_time, std::move(callback));

    std::lock_guard<std::mutex> lock(mutex_);
    all_timers_[id] = task;

    int slot_index = (cursor_ + static_cast<int>(delay_ms / TICK_MS)) % WHEEL_SIZE;
    slots_[slot_index].tasks.push_back(task);
}

void TimingWheel::removeTimer(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    all_timers_.erase(id);
}

void TimingWheel::onTimer(int64_t id) {
    // Timer expiration handler - to be implemented
}

int64_t TimingWheel::getCurrentTimeMs() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace auction

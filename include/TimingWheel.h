#pragma once

#include <vector>
#include <list>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

namespace auction {

class TimerTask {
public:
    using TimerCallback = std::function<void(int64_t)>;

    TimerTask(int64_t id, int64_t expire_time, TimerCallback callback)
        : id_(id), expire_time_(expire_time), callback_(std::move(callback)) {}

    int64_t id() const { return id_; }
    int64_t expire_time() const { return expire_time_; }
    void execute() { if (callback_) callback_(id_); }

private:
    int64_t id_;
    int64_t expire_time_;
    TimerCallback callback_;
};

class TimingWheel {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    TimingWheel();
    ~TimingWheel();

    void addTimer(int64_t id, int64_t delay_ms, TimerTask::TimerCallback callback);
    void removeTimer(int64_t id);
    void onTimer(int64_t id);

    void start();
    void stop();

private:
    struct WheelSlot {
        std::list<std::shared_ptr<TimerTask>> tasks;
    };

    void tick();
    int64_t getCurrentTimeMs();

    static constexpr int TICK_MS = 100;
    static constexpr int WHEEL_SIZE = 60;
    static constexpr int ACCURACY_BUCKETS = 10;

    std::vector<WheelSlot> slots_;
    std::unordered_map<int64_t, std::shared_ptr<TimerTask>> all_timers_;

    std::atomic<bool> running_{false};
    std::thread wheel_thread_;
    std::mutex mutex_;
    int cursor_{0};
};

} // namespace auction

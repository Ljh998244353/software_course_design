#include <gtest/gtest.h>
#include "TimingWheel.h"
#include <atomic>
#include <chrono>

using namespace auction;

class TimingWheelTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override { wheel_.stop(); }
    TimingWheel wheel_;
};

TEST_F(TimingWheelTest, AddTimer) {
    std::atomic<int> counter{0};

    wheel_.start();
    wheel_.addTimer(1, 200, [&counter](int64_t id) {
        EXPECT_EQ(id, 1);
        counter++;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(TimingWheelTest, RemoveTimer) {
    std::atomic<int> counter{0};

    wheel_.start();
    wheel_.addTimer(1, 200, [&counter](int64_t id) {
        (void)id;
        counter++;
    });

    wheel_.removeTimer(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_EQ(counter.load(), 0);
}

TEST_F(TimingWheelTest, MultipleTimers) {
    std::atomic<int> counter{0};

    wheel_.start();
    wheel_.addTimer(1, 100, [&counter](int64_t id) { (void)id; counter++; });
    wheel_.addTimer(2, 200, [&counter](int64_t id) { (void)id; counter++; });
    wheel_.addTimer(3, 300, [&counter](int64_t id) { (void)id; counter++; });

    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    EXPECT_EQ(counter.load(), 3);
}

TEST_F(TimingWheelTest, TimerTaskGetId) {
    TimerTask task(42, 1000, [](int64_t) {});
    EXPECT_EQ(task.id(), 42);
    EXPECT_EQ(task.expire_time(), 1000);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

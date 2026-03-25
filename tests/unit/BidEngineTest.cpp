#include <gtest/gtest.h>
#include "BidEngine.h"
#include "AntiSnipe.h"
#include "ConfigManager.h"

using namespace auction;

class BidEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigManager::instance().load("../../config/server.yaml");
    }
};

TEST_F(BidEngineTest, AntiSnipeShouldExtend) {
    AntiSnipe anti_snipe;

    EXPECT_TRUE(anti_snipe.shouldExtend(30000, 0));
    EXPECT_TRUE(anti_snipe.shouldExtend(60000, 0));
    EXPECT_FALSE(anti_snipe.shouldExtend(120000, 0));
    EXPECT_FALSE(anti_snipe.shouldExtend(30000, 10));
}

TEST_F(BidEngineTest, AntiSnipeMaxExtensions) {
    AntiSnipe anti_snipe;

    EXPECT_FALSE(anti_snipe.shouldExtend(30000, 9));
    EXPECT_FALSE(anti_snipe.shouldExtend(30000, 10));
}

TEST_F(BidEngineTest, CalculateNewEndTime) {
    AntiSnipe anti_snipe;
    int64_t current_end = 1000000;

    int64_t new_end = anti_snipe.calculateNewEndTime(current_end, 0);
    EXPECT_EQ(new_end, current_end + 120000);

    new_end = anti_snipe.calculateNewEndTime(current_end, 9);
    EXPECT_EQ(new_end, current_end + 120000);

    new_end = anti_snipe.calculateNewEndTime(current_end, 10);
    EXPECT_EQ(new_end, current_end);
}

TEST_F(BidEngineTest, BidResultDefault) {
    BidResult result;
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_code.empty());
    EXPECT_EQ(result.current_price, 0.0);
    EXPECT_EQ(result.highest_bidder_id, 0);
    EXPECT_FALSE(result.is_extended);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

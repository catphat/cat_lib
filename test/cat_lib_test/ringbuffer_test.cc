
#include <gtest/gtest.h>
#include "ringbuffer.hpp"


// Demonstrate some basic assertions.
TEST(RingBufferTest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}


TEST(RingBufferTest_1, BasicAssertions) {
    cat_lib::Ringbuffer<const char*, 256> messageQueue;
    std::string msg;

    for (int i = 0; i < 255; ++i) {
         msg = std::to_string(i);
        messageQueue.insert(msg.c_str());
    }
    EXPECT_FALSE(messageQueue.isFull());
    msg = "256";
    bool result = messageQueue.insert(msg.c_str());
    EXPECT_TRUE(result);
    EXPECT_TRUE(messageQueue.isFull());

    msg = "257";
    result = messageQueue.insert(msg.c_str());
    EXPECT_FALSE(result);
}
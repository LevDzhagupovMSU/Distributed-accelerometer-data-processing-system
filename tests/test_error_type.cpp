#include <gtest/gtest.h>
#include "error_type.hpp"

TEST(ErrorTypeTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(ErrorType::CONNECTION), 0);
    EXPECT_EQ(static_cast<int>(ErrorType::PROTOCOL), 1);
    EXPECT_EQ(static_cast<int>(ErrorType::INTERNAL), 2);
}
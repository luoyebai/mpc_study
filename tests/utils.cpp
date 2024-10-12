
// std
#include <climits>
// gtest
#include <gtest/gtest.h>
// headers
#include "controller/matrix.hpp"

// matrix test
using controller::Matrix;
TEST(ControllerMatrix, CheckMatrixDim) {
    EXPECT_TRUE(controller::CheckMatrixDim({1, 1, 1, 1}, 4, 1));
    EXPECT_TRUE(controller::CheckMatrixDim(Matrix::zeros(13, 17), 13, 17));
    EXPECT_TRUE(controller::CheckMatrixDim(Matrix::zeros(128, 64), 128, 64));
    EXPECT_FALSE(controller::CheckMatrixDim({1, 1, 1, 1}, 4, 2));
    EXPECT_FALSE(controller::CheckMatrixDim(Matrix::zeros(13, 17), 17, 13));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

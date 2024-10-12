
// std
#include <climits>
// gtest
#include <gtest/gtest.h>
#include <yaml-cpp/node/node.h>
// yaml
#include "utils/yaml.hpp"
// headers
#include "controller/matrix.hpp"

// matrix test
using controller::Matrix;
TEST(ControllerMatrix, CheckMatrixDim) {

    // matrix_true_tests:
    //   test_cases: 10
    //   size_test:
    //    size_inputs: [100,20,20,100]
    //    res: [100,20,20,100]
    auto yaml =
        utils::Yaml(std::string(__TESTS_DIR__) + "config/controller.yaml");
    auto true_cases = yaml.readValue<int>("matrix_true_tests.test_cases");
    auto true_case = yaml.readValue<int>("matrix_true_tests.test_cases");

    // auto true_datas =
    //     yaml.readValue<YAML::Node>("matrix_true_tests.test_datas");
    // auto true_inputs =
    //     yaml.readValue<int>("matrix_true_tests.test_datas", 2, true_cases);
    // auto true_res =
    //     yaml.readValue<int>("matrix_true_tests.test_datas.res", 2,
    //     true_cases);

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

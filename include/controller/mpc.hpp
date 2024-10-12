/**
 * \file mpc.hpp
 * @author luoyebai (luoyebai4444@gmail.com)
 * \brief
 * @version 0.1
 * \date 2024-10-01
 *
 * @copyright luoyebai Copyright (c) 2024
 *
 */

#ifndef CONTROLLER_MPC_HPP
#define CONTROLLER_MPC_HPP

// matrix
#include "controller/matrix.hpp"
#include <map>

namespace controller {

constexpr int KONEDIM = 1;

/**
 * \brief This is a class for model predictive control,use casadi to solve the
 * optimization problem.<br>
 *
 * Example code:
 * \code
 * const auto mpc_controller = controller::Mpc(mpc_config_path);
 * mpc_controller.display();
 * \endcode
 *
 * \note mpc_config_path is a yaml config file, example:
 * - state_dim: 2
 * - input_dim: 2
 * - prediction_horizon: 10
 * - incremental: true
 * - A: [1, 0.1, -1, -2]
 * - B: [0.2, 1, 0.5, 2]
 * - Q: [100, 0, 0, 100]
 * - F: [100, 0, 0, 100]
 * - R: [1, 0, 0, 3]
 * - UBG: [1e10, 1e10]
 * - LBG: [1e-10, 1e-10]
 * \author luoyebai(luoyebai4444@gmail.com)
 * \date 2024-09-30
 *
 */

class Mpc {
  public:
    using DimMap =
        std::map<const std::string, const std::pair<const int &, const int &>>;
    using MatrixMap = std::map<std::string, const Matrix &>;

    const DimMap dim_map = {
        {"A", {this->dimensions_.state_dim, this->dimensions_.state_dim}},
        {"B", {this->dimensions_.state_dim, this->dimensions_.input_dim}},
        {"Q", {this->dimensions_.state_dim, this->dimensions_.state_dim}},
        {"F", {this->dimensions_.state_dim, this->dimensions_.state_dim}},
        {"R", {this->dimensions_.input_dim, this->dimensions_.input_dim}},
        {"UBG", {KONEDIM, this->dimensions_.input_dim}},
        {"LBG", {KONEDIM, this->dimensions_.input_dim}},
    };

    const MatrixMap config_map = {
        {"A", this->trans_mat_.A},       {"B", this->trans_mat_.B},
        {"Q", this->adjust_mat_.Q},      {"F", this->adjust_mat_.F},
        {"R", this->adjust_mat_.R},      {"UBG", this->constraints_.UBG},
        {"LBG", this->constraints_.LBG},
    };

    /**
     * \brief Includes the dimensions of the state vector and input vector.<br>
     * Generally expressed by n and p.
     *
     */
    struct Dimensions {
        /// \brief  State dimension.
        int state_dim;
        /// \brief  Input dimension.
        int input_dim;
    };

    /**
     * \brief This is state transition matrix for mpc.<br>
     * Transition matrix is a matrix which can be used to calculate the next
     * state.<br>
     * \details The state transition equation is as follows:
     * <br>
     * \f$
     * x(k+1)=\underbrace{A}_{n \times n}
     * \underbrace{x(k)}_{n \times 1}
     * +\underbrace{B}_{n \times p}
     * \underbrace{u(k)}_{p \times 1}
     * \f$
     *
     */
    struct TransitionMatrix {
        Matrix A;
        Matrix B;
    };

    /**
     * \brief This is all cost weight matrices in MPC.<br>
     * They are all diagonal matrices.
     *
     */
    struct AdjustmentMatrix {
        /// \brief This is the weight matrix of the state error cost in MPC.
        Matrix Q;
        /// \brief This is the weight matrix of the final state cost in MPC.
        Matrix F;
        /// \brief This is the weight matrix of the input cost in MPC.
        Matrix R;
    };

    struct Constraints {
        Matrix UBG;
        Matrix LBG;
    };

    Mpc() = delete;
    Mpc(Mpc &) = default;
    Mpc(Mpc &&) = default;
    ~Mpc() = default;

    /**
     * \brief Construct a new Mpc controller using the configuration file.
     *
     * \param config_path The config file path.
     */
    Mpc(const std::string &config_path);

    /**
     * \brief Construct a new Mpc controller.
     *
     * \param dimensions State and input dimension.
     * \param trans_mat Transition matrix, including A and B.
     * \param adjust_mat  Adjustment matrix, including Q, F and R.
     * \param prediction_horizon Prediction horizon, default is 10.
     * \param incremental If true, the input cost in the optimizer will use
     * the input delta as a reference.
     */
    explicit Mpc(const Dimensions &dimensions,
                 const TransitionMatrix &trans_mat,
                 const AdjustmentMatrix &adjust_mat,
                 const Constraints &constraints, int prediction_horizon = 10,
                 bool incremental = true) noexcept;

    /**
     * \brief Set the prediction horizon.
     *
     * \param N Set value.
     */
    void setPredictionHorizon(int N) noexcept;

    /**
     * \brief Check the dimensions of all the matrices.
     *
     * \return true All dimensions are correct.
     * \return false Exist incorrect dimensions.
     */
    bool checkAllMatrix() const;

    void prepareAllVars();

    MDict traceTarget(const Matrix &target, const Matrix &state_now) const;

    /**
     * \brief Print the dimensions and matrices.
     *
     */
    void display() const;

  private:
    /// \brief Basic variables
    Dimensions dimensions_;
    TransitionMatrix trans_mat_;
    AdjustmentMatrix adjust_mat_;
    Constraints constraints_;
    int prediction_horizon_;
    bool incremental_;

    /// \brief Intermediate variables
    Matrix M_;
    Matrix C_;
    Matrix Q_BAR_;
    Matrix R_BAR_;
    Matrix UBG_BAR_;
    Matrix LBG_BAR_;

    const MatrixMap intermediate_map_ = {
        {"M", this->M_},
        {"C", this->C_},
        {"Q_BAR", this->Q_BAR_},
        {"R_BAR", this->R_BAR_},
        {"UBG_BAR", this->UBG_BAR_},
        {"LBG_BAR", this->LBG_BAR_},
    };

    void computeM();
    void computeC();
    void computeQBAR();
    void computeRBAR();
    void computeConstraintsBAR();

    auto computeCostFunction(const MatrixSym &XK, const MatrixSym &input,
                             const MatrixSym &last_input_frame) const;

    /// The function needs to pass in parameters of type int
    template <typename Function,
              typename = std::void_t<
                  decltype(std::declval<Function>()(std::declval<int>()))>>
    void forEachPrediction(Function &&f) {
        for (int i = 0; i < this->prediction_horizon_; ++i)
            f(i);
        return;
    }
};

} // namespace controller

#endif // !CONTROLLER_MPC_HPP

// std
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string_view>
// yaml
#include "casadi/core/nlpsol.hpp"
#include "utils/yaml.hpp"
// matrix
#include "controller/matrix.hpp"
// header
#include "controller/mpc.hpp"

namespace controller {

/// public
Mpc::Mpc(const Dimensions &dimensions, const TransitionMatrix &trans_mat,
         const AdjustmentMatrix &adjust_mat, const Constraints &constraints,
         int prediction_horizon, bool incremental) noexcept
    : dimensions_(dimensions), trans_mat_(trans_mat), adjust_mat_(adjust_mat),
      constraints_(constraints), prediction_horizon_(prediction_horizon),
      incremental_(incremental) {}

Mpc::Mpc(const std::string &config_path) {
    auto config = utils::Yaml(config_path);
    auto state_dim = config.readValue<int>("state_dim");
    auto input_dim = config.readValue<int>("input_dim");
    // init
    this->dimensions_ = {state_dim, input_dim};
    config.readValue("incremental", this->incremental_);
    config.readValue("prediction_horizon", this->prediction_horizon_);
    // read config assignment
    for (auto [key, dim] : this->dim_map)
        const_cast<Matrix &>(this->config_map.at(key)) =
            Matrix(config.readValue<double>(key, dim.first, dim.second));
}

void Mpc::setPredictionHorizon(int N) noexcept {
    this->prediction_horizon_ = N;
    return;
}

bool Mpc::checkAllMatrix() const {
    bool check_success = true;
    auto check_f = [&check_success](const std::string_view &name,
                                    const Matrix &mat, int size1, int size2) {
        if (CheckMatrixDim(mat, size1, size2))
            return;
        std::stringstream ss;
        ss << name << " dim is " << mat.size() << " != [" << size1 << ","
           << size2 << "]";
        throw std::runtime_error(ss.str());
        check_success = false;
    };
    for (auto [key, dim] : dim_map)
        check_f(key, this->config_map.at(key), dim.first, dim.second);
    return check_success;
}

void Mpc::prepareAllVars() {
    this->computeM();
    this->computeC();
    this->computeQBAR();
    this->computeRBAR();
    this->computeConstraintsBAR();
    return;
}

// private
auto Mpc::computeCostFunction(const MatrixSym &XK, const MatrixSym &input,
                              const MatrixSym &last_input_frame) const {
    static const auto &[_, p] = this->dimensions_;
    static const auto &N = this->prediction_horizon_;
    auto cost_function = MatrixSym();
    if (!this->incremental_) {
        cost_function = MatrixSym::bilin(this->Q_BAR_, XK, XK) +
                        MatrixSym::bilin(this->R_BAR_, input, input);
    } else {
        BlockMatrixSyms last_input;
        last_input.push_back(last_input_frame);
        for (int i = 0; i < N - 1; ++i) {
            BlockMatrixSyms blocks;
            for (int j = 0; j < p; ++j)
                blocks.push_back(input(i * p + j));
            last_input.push_back(MatrixSym::vertcat(blocks));
        }
        auto input_inc = input - MatrixSym::vertcat(last_input);
        cost_function = MatrixSym::bilin(this->Q_BAR_, XK, XK) +
                        MatrixSym::bilin(this->R_BAR_, input_inc, input_inc);
    }
    return cost_function;
}

// public
MDict Mpc::traceTarget(const Matrix &target, const Matrix &state_now) const {
    static const auto &[_, p] = this->dimensions_;
    static const auto &N = this->prediction_horizon_;
    static auto last_input_frame = MatrixSym::zeros(p, 1);
    auto input = MatrixSym::sym("U", N * p);
    auto target_block = BlockMatrixs();

    for (int i = 0; i <= N; ++i)
        target_block.push_back(target);
    auto target_n = Matrix::vertcat(target_block);

    auto XK = MatrixSym::mtimes(this->M_, state_now) - MatrixSym(target_n) +
              MatrixSym::mtimes(this->C_, input);

    auto cost_function = this->computeCostFunction(XK, input, last_input_frame);

    auto solver = casadi::nlpsol(
        "mpc_solver", "ipopt",
        MSymDict{{"x", input}, {"f", cost_function}, {"g", input}},
        Dict{{"ipopt.print_level", 0},
             {"print_time", false},
             {"ipopt.sb", "yes"}});

    auto res = solver(MDict{{"x0", Matrix::zeros(N * p)},
                            {"ubg", this->UBG_BAR_},
                            {"lbg", this->LBG_BAR_}});

    BlockMatrixs input_now_blocks;
    BlockMatrixs inputs_blocks;

    for (int i = 0; i < p; ++i)
        input_now_blocks.push_back(res["x"](i));
    auto input_now = Matrix(input_now_blocks);
    last_input_frame = input_now;

    auto predict_states = Matrix::mtimes(this->M_, state_now) +
                          Matrix::mtimes(this->C_, res["x"]);

    return MDict{{"input_now", input_now},
                 {"inputs", res["x"]},
                 {"predict_states", predict_states}};
}

void Mpc::display() const {
    const auto [n, p] = this->dimensions_;
    const auto N = this->prediction_horizon_;
    std::cout << "incremental=" << std::boolalpha << this->incremental_
              << "\t(n,p,N)=(" << n << "," << p << "," << N << ")\n";
    for (const auto &[key, value] : this->config_map)
        std::cout << key << "=" << value << "\n";
    for (const auto &[key, value] : this->intermediate_map_)
        std::cout << key << "=" << value << "\n";
}

/// private
void Mpc::computeM() {
    BlockMatrixs blocks;
    this->forEachPrediction([this, &blocks](int i) {
        blocks.push_back(Matrix::mpower(this->trans_mat_.A, i));
    });
    blocks.push_back(
        Matrix::mpower(this->trans_mat_.A, this->prediction_horizon_));

    this->M_ = Matrix::vertcat(blocks);
    return;
}

void Mpc::computeC() {
    const auto [n, p] = this->dimensions_;
    const auto N = this->prediction_horizon_;
    BlockMatrixs res_down_blocks;
    for (int i = 0; i < N; ++i) {
        BlockMatrixs row_blocks;
        for (int j = 0; j < N; ++j) {
            if (i < j) {
                row_blocks.push_back(Matrix::zeros(n, p));
                continue;
            }
            const auto A_pow_N_i = Matrix::mpower(this->trans_mat_.A, N - i);
            const auto A_pow_N_j = Matrix::mpower(this->trans_mat_.A, N - j);
            const auto A_inv_N_i = Matrix::inv(A_pow_N_i);
            const auto block = Matrix::mtimes(
                Matrix::mtimes(A_pow_N_j, A_inv_N_i), this->trans_mat_.B);
            row_blocks.push_back(block);
        }
        res_down_blocks.push_back(Matrix::horzcat(row_blocks));
    }
    const auto res_up = Matrix::zeros(p, n * N);
    const auto res_down = Matrix::vertcat(res_down_blocks);
    this->C_ = Matrix::vertcat({res_up, res_down});
    return;
}

void Mpc::computeQBAR() {
    BlockMatrixs blocks;
    this->forEachPrediction(
        [this, &blocks](int i) { blocks.push_back(this->adjust_mat_.Q); });
    blocks.push_back(this->adjust_mat_.F);
    this->Q_BAR_ = Matrix::diagcat(blocks);
    return;
}

void Mpc::computeRBAR() {
    BlockMatrixs blocks;
    this->forEachPrediction(
        [this, &blocks](int i) { blocks.push_back(this->adjust_mat_.R); });
    this->R_BAR_ = Matrix::diagcat(blocks);
    return;
}

void Mpc::computeConstraintsBAR() {
    BlockMatrixs ubg_blocks;
    BlockMatrixs lbg_blocks;
    this->forEachPrediction([this, &ubg_blocks](int i) {
        ubg_blocks.push_back(this->constraints_.UBG);
    });
    this->forEachPrediction([this, &lbg_blocks](int i) {
        lbg_blocks.push_back(this->constraints_.LBG);
    });

    this->UBG_BAR_ = Matrix::reshape(
        Matrix::vertcat(ubg_blocks),
        this->dimensions_.input_dim * this->prediction_horizon_, 1);

    this->LBG_BAR_ = Matrix::reshape(
        Matrix::vertcat(lbg_blocks),
        this->dimensions_.input_dim * this->prediction_horizon_, 1);
    return;
}

} // namespace controller

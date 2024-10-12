#ifndef CONTROLLER_MATRIX_HPP
#define CONTROLLER_MATRIX_HPP

// casadi
#include <casadi/casadi.hpp>

namespace controller {
/**
 * \brief  Casadi matrix type.
 * \details Please check the following website for specific details:<br>
 * https://web.casadi.org/api/html/da/d45/classcasadi_1_1Matrix.html
 */
using Matrix = casadi::DM;
using MatrixSym = casadi::MX;
using Dict = casadi::Dict;
using MDict = casadi::DMDict;
using MSymDict = casadi::MXDict;
using BlockMatrixs = std::vector<Matrix>;
using BlockMatrixSyms = std::vector<MatrixSym>;

/**
 * \brief Check the dimensions of the matrix.
 *
 * \param mat Checked matrix.
 * \param size1 Columns.
 * \param size2 Rows.
 * \return true
 * \return false
 */
bool CheckMatrixDim(const Matrix &mat, int size1, int size2) noexcept;

} // namespace controller

#endif // !CONTROLLER_MATRIX_HPP

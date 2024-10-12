
// header
#include "controller/matrix.hpp"

namespace controller {

bool CheckMatrixDim(const Matrix &mat, int size1, int size2) noexcept {
    if (mat.size1() != size1 || mat.size2() != size2)
        return false;
    else
        return true;
}

} // namespace controller

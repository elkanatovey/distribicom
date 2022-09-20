#include "matrix.h"

namespace multiplication_utils {
    template<typename T>
    void matrix<T>::resize(std::uint64_t _rows, std::uint64_t _cols) {
        rows = _rows;
        cols = _cols;
        data.resize(rows * cols);
    }

    template<typename T>
    void matrix<T>::assert_pos(std::uint64_t row, std::uint64_t col) const {
        assert(row >= 0);
        assert(col >= 0);
        assert(row + col * rows < data.size());
    }
}
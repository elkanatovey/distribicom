
#pragma once

#include "vector"
#include <cstdint>
#include <cassert>


namespace multiplication_utils {
    template<typename T>
    class matrix {
    public:
        std::vector<T> data;
        std::uint64_t rows;
        std::uint64_t cols;

        matrix() : rows(0), cols(0), data() {}

        matrix(std::uint64_t rows, std::uint64_t cols) : rows(rows), cols(cols), data(rows * cols) {};

        matrix(std::uint64_t rows, std::uint64_t cols, std::vector<T> data) : rows(rows), cols(cols), data(data) {}

        matrix(std::uint64_t rows, std::uint64_t cols, T cpy) : rows(rows), cols(cols), data(rows * cols, cpy) {}

        inline void resize(std::uint64_t _rows, std::uint64_t _cols);

        [[nodiscard]] inline std::uint64_t pos(std::uint64_t row, std::uint64_t col) const { return row + col * rows; }

        inline const T &operator()(std::uint64_t row, std::uint64_t col) const {
            #ifdef DISTRIBICOM_DEBUG
            assert_pos(row, col);
            #endif
            return data[pos(row, col)];
        }


        inline T &operator()(std::uint64_t row, std::uint64_t col) {
            #ifdef DISTRIBICOM_DEBUG
            assert(k < data.size());
            #endif
            return data[pos(row, col)];
        }

        void assert_pos(std::uint64_t row, std::uint64_t col) const;
    };
}
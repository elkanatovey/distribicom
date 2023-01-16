
#pragma once

#include <vector>
#include <cstdint>
#include <cassert>


#ifdef DISTRIBICOM_DEBUG

#include <iostream>

#endif // DISTRIBICOM_DEBUG


namespace math_utils {
    template<typename T>
    class matrix {
    public:
        std::uint64_t rows;
        std::uint64_t cols;
        std::vector<T> data;

        matrix() : rows(0), cols(0), data() {}

        matrix(std::uint64_t rows, std::uint64_t cols) : rows(rows), cols(cols), data(rows * cols) {};

        matrix(std::uint64_t rows, std::uint64_t cols, std::vector<T> data) : rows(rows), cols(cols), data(data) {}

        matrix(std::uint64_t rows, std::uint64_t cols, T cpy) : rows(rows), cols(cols), data(rows * cols, cpy) {}

        inline void resize(std::uint64_t _rows, std::uint64_t _cols) {
            rows = _rows;
            cols = _cols;
            data.resize(rows * cols);

        };

        [[nodiscard]] inline std::uint64_t pos(std::uint64_t row, std::uint64_t col) const { return row + col * rows; }

        inline const T &operator()(std::uint64_t row, std::uint64_t col) const {
#ifdef DISTRIBICOM_DEBUG
            assert_pos(row, col);
#endif
            return data[pos(row, col)];
        }


        inline T &operator()(std::uint64_t row, std::uint64_t col) {
#ifdef DISTRIBICOM_DEBUG
            assert_pos(row, col);
#endif
            return data[pos(row, col)];
        }

        inline void assert_pos(std::uint64_t row, std::uint64_t col) const {
#ifdef DISTRIBICOM_DEBUG

            if (row + col * rows >= data.size()) {
                std::stringstream o;
                o << "matrix::assert_pos: row + col * rows >= data.size()"
                  << " row: " << row << ", col: " << col << ", rows: " << rows << ", cols: " << cols
                    << ", data.size(): " << data.size();
                throw std::out_of_range(o.str());
                std::cout << std::endl;
            }
#endif
        };
    };
}
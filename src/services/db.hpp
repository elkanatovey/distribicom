#pragma once

#include "../math_utils/matrix.h"
#include "seal/seal.h"
#include <mutex>


// This class is a wrapper for a matrix that ensures the access to the matrix is thread safe.
template<typename T>
class DB {

    // a proper way to receive a matrix from the DB for multiple reads. upon scope exit the DB's lock will be released.
    struct shared_mat {
        explicit shared_mat(const multiplication_utils::matrix<T> &mat, std::mutex &mtx) : mat(mat), lck(mtx) {}

        const multiplication_utils::matrix<T> &mat;
    private:
        std::lock_guard<std::mutex> lck;
    };

    multiplication_utils::matrix<T> memory_matrix;
    std::mutex mtx;

public:
    // TODO: add a proper way to create PIR DB.
    explicit DB(const int a, const int b) : memory_matrix(), mtx() {}

    // used mainly for testing.
    explicit DB(multiplication_utils::matrix<T> &mat) : memory_matrix(mat), mtx() {}

    // gives access to a locked reference of the matrix.
    // as long as the shared_mat instance is not destroyed the DB remains locked.
    // useful for distributing the DB, or going over multiple rows.
    shared_mat many_reads() {
        return shared_mat(memory_matrix, mtx);
    }

    void write(const T &t, const int row, const int col) {
        mtx.lock();
        memory_matrix(row, col) = t;
        mtx.unlock();
    }

    // todo: add special write function - for the DB<seal::Plaintext> case - where we can write to specific part
    //  of a plaintext.
    void write() {

    }

    // mainly used for testing.
    bool try_write() {
        auto success = mtx.try_lock();
        if (!success) {
            return false;
        }
        mtx.unlock();
        return true;
    };
};



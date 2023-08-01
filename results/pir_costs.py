# times taken from our distribicom/test/math_utils/benchmarks using SEAL 4.0.0
import numpy as np
from matplotlib import pyplot as plt, ticker

from results import utils

addition_cipher_time = 13138
mult_cipher_time = 792613
mult_plain_time = 61054


def num_ops_in_vec_to_vec_mult(n):
    return {"mult": n, "add": n - 1}


def matrix_mult_ops(n, m, k):
    vec_mult_sizes = m
    num_vec_mults = n * k

    ops = num_ops_in_vec_to_vec_mult(vec_mult_sizes)
    for op in ops:
        ops[op] *= num_vec_mults

    return ops


def pir_query_lvl1_time(db_rows, db_cols):
    # PIR lvl1 is where we take the db as a ptx matrix and multiply it by a query (ctx) vector from the right.
    num_ops = matrix_mult_ops(db_rows, db_cols, 1)
    return num_ops["mult"] * mult_plain_time + num_ops["add"] * addition_cipher_time


def pir_query_lvl2_time(db_rows, db_cols):
    # PIR lvl2 is after PIR lvl1, where we have two (ctx) vectors and multiply them.
    num_ops = matrix_mult_ops(1, db_cols, 1)
    return num_ops["mult"] * mult_cipher_time + num_ops["add"] * addition_cipher_time


def ns2ms(ns):
    return ns * 1e-6


if __name__ == '__main__':
    # db sizes to PIR costs
    for rows, cols in [(42, 41), (82, 82), (164, 164), (232, 232)]:
        pir1 = pir_query_lvl1_time(rows, cols)
        pir2 = pir_query_lvl2_time(rows, cols)
        db_size = rows * cols * 39

        print(f"{db_size} & {ns2ms(pir1)} & {ns2ms(pir2)} \\\\")
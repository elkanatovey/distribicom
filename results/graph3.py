import numpy as np
from matplotlib import pyplot as plt

import utils, constants

# x db size to times.
dpir_values = {
    1 << 13: utils.GenericDataPoint(60, [470, 453, 544, 551, 517, 523, 488, 470, 468]),
    1 << 14: utils.GenericDataPoint(105, [1286, 1273, 1267, 1263, 1176, 1180, 1030, 1291, 1255]),
    1 << 15: utils.GenericDataPoint(210, [3535, 3405, 3401, 3477, 3300, 3406, 3329, 3348]),
    1 << 16: utils.GenericDataPoint(420, [6397, 6410, 6430, 6426, 6426, 6438, 6430, 6424, 6462])
}

fpir_values = {
    1 << 13: utils.GenericDataPoint(60, [182, 181, 170, 173, 171, 172]),
    1 << 14: utils.GenericDataPoint(105, [382, 381, 382, 381, 383]),
    1 << 15: utils.GenericDataPoint(210, [1071, 1069, 1071, 1065, 1068]),
    1 << 16: utils.GenericDataPoint(420, [3289, 3297, 3294, 3299, 3280])
}
sealpir_values = sealpir_res = utils.grab_sealpir_results_from_file(
    "/Users/jonathanweiss/CLionProjects/distribicom/results/evals/3rd-graph/run_sealpir_log.txt")


def num_queries_to_x_axis(num_queries):
    if num_queries == 60:
        return 1 << 13
    elif num_queries == 105:
        return 1 << 14
    elif num_queries == 210:
        return 1 << 15
    elif num_queries == 420:
        return 1 << 16

    raise Exception("Unknown num queries: " + str(num_queries))


def set_color(prms, clr):
    prms["ecolor"] = clr
    prms["color"] = clr


if __name__ == '__main__':
    sealpir_dict = {}
    for res in sealpir_res:
        sealpir_dict[num_queries_to_x_axis(res.queries)] = res

    fig, ax = plt.subplots()
    xs = sorted(sealpir_dict.keys())

    prms = {
        "markersize": 4,
        "barsabove": True,
        "capsize": 2,
        "elinewidth": constants.line_size - 1
    }

    dpir_ys = [*map(lambda x: dpir_values[x].avg, xs)]
    dpir_errs = [*map(lambda x: dpir_values[x].std, xs)]
    set_color(prms, constants.dpir_clr)
    plt.errorbar(xs, dpir_ys, dpir_errs, **prms)

    sealpir_ys = [*map(lambda x: sealpir_dict[x].avg, xs)]
    sealpir_errs = [*map(lambda x: sealpir_dict[x].std, xs)]
    set_color(prms, constants.sealpir_clr)
    plt.errorbar(xs, sealpir_ys, sealpir_errs, **prms)

    fpir_ys = [*map(lambda x: fpir_values[x].avg, xs)]
    fpir_errs = [*map(lambda x: fpir_values[x].std, xs)]
    set_color(prms, constants.addra_clr)
    plt.errorbar(xs, fpir_ys, fpir_errs, **prms)

    utils.add_y_format(ax)

    ax.set_xticks(xs)
    ax.set_xticklabels(["$2^{13}$", "$2^{14}$", "$2^{15}$", "$2^{16}$"])

    plt.show()

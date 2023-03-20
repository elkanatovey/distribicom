import numpy as np
from matplotlib import pyplot as plt

import utils, constants

# x db size to times.
dpir_values = {
    1 << 13: utils.GenericDataPoint(60, [470, 453, 544, 551, 517, 523, 488, 470, 468]),
    1 << 14: utils.GenericDataPoint(105, [1286, 1273, 1267, 1263, 1176, 1180, 1030, 1291, 1255]),
    1 << 15: utils.GenericDataPoint(210, [4380, 4594, 4383, 4319, 4377, 3974, 4088, 3732, 3557]),
    1 << 16: utils.GenericDataPoint(420, [6397, 6410, 6430, 6426, 6426, 6438, 6430, 6424, 6462])
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
    dpir_ys = [*map(lambda x: dpir_values[x].avg, xs)]
    dpir_errs = [*map(lambda x: dpir_values[x].std, xs)]

    sealpir_ys = [*map(lambda x: sealpir_dict[x].avg, xs)]
    sealpir_errs = [*map(lambda x: sealpir_dict[x].std, xs)]

    prms = {
        "markersize": 4,
        "barsabove": True,
        "capsize": 2,
        "elinewidth": constants.line_size - 1
    }

    set_color(prms, constants.dpir_clr)
    plt.errorbar(xs, dpir_ys, dpir_errs, **prms)

    set_color(prms, constants.sealpir_clr)
    plt.errorbar(xs, sealpir_ys, sealpir_errs, **prms)

    utils.add_y_format(ax)
    ax.set_xticks(xs)
    ax.set_xticklabels(["$2^{13}$", "$2^{14}$", "$2^{15}$", "$2^{16}$"])
    plt.show()

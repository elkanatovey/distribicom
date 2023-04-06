import numpy as np
import matplotlib
from matplotlib import pyplot as plt

import utils, constants

# x db size to times.
dpir_values = {
    1 << 16: utils.GenericDataPoint(84, [1407, 1393, 1395, 1394, 1399, 1402, 1406, 1411, 1398]),
    1 << 18: utils.GenericDataPoint(164, [5365, 5612, 5229, 5241, 5237, 5238, 5229, 5244, 5219]),
    1 << 19: utils.GenericDataPoint(232, [11445, 11018, 10842, 10776, 10759, 10676, 10769, 10765, 10707]),
    1 << 20: utils.GenericDataPoint(328, [23786, 21804, 21853, 21774, 21816, 21815, 21800, 21761, 21746]),
}

fpir_values = {
    1 << 16: utils.GenericDataPoint(84, [664, 667, 670, 683, 670]),
    1 << 18: utils.GenericDataPoint(164, [4463, 4467, 4483, 4478, 4476]),
    1 << 19: utils.GenericDataPoint(232, [12708,12524,12606,12649,12534]),
    1 << 20: utils.GenericDataPoint(328, [33838, 33837, 33852]),
}

sealpir_values = {
    1 << 16: utils.GenericDataPoint(84, [664, 677, 670, 683, 670]),
    1 << 18: utils.GenericDataPoint(164, [15146, 15341, 15305, 15262, 15271]),
    1 << 19: utils.GenericDataPoint(232, [0]),
    1 << 20: utils.GenericDataPoint(328, [85106, 84490, 84662, 84755, 84576]),
}


def set_color(prms, clr):
    prms["ecolor"] = clr
    prms["color"] = clr


matplotlib.rcParams['font.size'] = constants.font_size

if __name__ == '__main__':
    fig, ax = plt.subplots()
    xs = sorted(sealpir_values.keys())

    prms = {
        "marker": 'o',
        "linewidth": constants.line_size,
        "markersize": constants.line_size + 1,

        "barsabove": True,
        # "capsize": constants.line_size + 1,
        # "linewidth": constants.line_size,
        # "elinewidth": constants.line_size + 1
    }
    sealpir_ys = [*map(lambda x: sealpir_values[x].avg, xs)]
    sealpir_errs = [*map(lambda x: sealpir_values[x].std, xs)]
    set_color(prms, constants.sealpir_clr)
    plt.errorbar(xs, sealpir_ys, sealpir_errs, **prms)

    fpir_ys = [*map(lambda x: fpir_values[x].avg, xs)]
    fpir_errs = [*map(lambda x: fpir_values[x].std, xs)]
    set_color(prms, constants.addra_clr)
    plt.errorbar(xs, fpir_ys, fpir_errs, **prms)

    dpir_ys = [*map(lambda x: dpir_values[x].avg, xs)]
    dpir_errs = [*map(lambda x: dpir_values[x].std, xs)]
    set_color(prms, constants.dpir_clr)
    plt.errorbar(xs, dpir_ys, dpir_errs, **prms)

    utils.add_y_format(ax)

    ax.set_xticks(xs)
    ax.set_xticklabels(["$2^{16}$", "$2^{18}$", "$s^{19}$", "$2^{20}$"])

    ax.set_ylabel('round latency')
    ax.legend(["SealPir (pung's engine)", "FPIR (Addra's engine)", "DPIR"])
    plt.show()

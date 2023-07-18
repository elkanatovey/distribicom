import numpy as np
import matplotlib
from matplotlib import pyplot as plt

import utils, constants

dpir_values = {
    # 1 << 16: utils.GenericDataPoint(82, [1407, 1393, 1395, 1394, 1399, 1402, 1406, 1411, 1398]),
    # 1 << 17: utils.GenericDataPoint(116, [2933, 2782, 2745, 2706, 2690, 2689, 2693, 2691, 2782]),
    # 1 << 18: utils.GenericDataPoint(164, [5365, 5612, 5229, 5241, 5237, 5238, 5229, 5244, 5219]),
    # 1 << 19: utils.GenericDataPoint(232, [11445, 11018, 10842, 10776, 10759, 10676, 10769, 10765, 10707]),

    1 << 16: utils.GenericDataPoint(21, [647, 648, 649, 650, 647, 642, 648, 642, 646, ]),  # 42
    1 << 17: utils.GenericDataPoint(58, [1503, 1486, 1473, 1478, 1480, 1476, 1478, 1473, 1458, ]),
    1 << 18: utils.GenericDataPoint(82, [3158, 2900, 2908, 2902, 2907, 2902, 2898, 2912, 2909, ]),  # 82x82
    1 << 19: utils.GenericDataPoint(116, [6530, 6442, 6479, 6530, 6373, 6523, 6443, 6407, 6594, ]),
    1 << 20: utils.GenericDataPoint(328, [23786, 21804, 21853, 21774, 21816, 21815, 21800, 21761, 21746]),  # 164x 164
    1 << 21: utils.GenericDataPoint(464, [66234, 71135, 70590, 71612, 72395, 71140, 70148, 68856, 68037, ])  # 232*232
}

fpir_values = {
    # 1 << 16: utils.GenericDataPoint(84, [664, 667, 670, 683, 670]),
    # 1 << 17: utils.GenericDataPoint(116, [1720, 1759, 1664, 1741, 1712]),
    # 1 << 18: utils.GenericDataPoint(164, [4463, 4467, 4483, 4478, 4476]),
    # 1 << 19: utils.GenericDataPoint(232, [12708, 12524, 12606, 12649, 12534]),
    # 1 << 20: utils.GenericDataPoint(328, [33838, 33837, 33852]),

    1 << 16: utils.GenericDataPoint(21, [221, 220, 224, 220, 203]),
    1 << 17: utils.GenericDataPoint(58, [1172, 1019, 973, 989, 988]),
    1 << 18: utils.GenericDataPoint(82, [2503, 2505, 2419, 2502, 2548]),
    1 << 19: utils.GenericDataPoint(116, [7131, 7305, 7083, 7056, 7111]),
    1 << 20: utils.GenericDataPoint(328, [33838, 33837, 33852, 37303, 37589, 37765, 37481, 37578]),
    1 << 21: utils.GenericDataPoint(464, [96595, 96212, 96266, 96114, 96681])  # 232*232
}

sealpir_values = {
    # 1 << 16: utils.GenericDataPoint(84, [664, 677, 670, 683, 670]),
    # 1 << 17: utils.GenericDataPoint(116, [5426, 5476, 5373, 5379, 5446]),
    # 1 << 18: utils.GenericDataPoint(164, [15146, 15341, 15305, 15262, 15271]),
    # 1 << 19: utils.GenericDataPoint(232, [33509, 33543, 33058, 33220, 33585]),

    1 << 16: utils.GenericDataPoint(21, [780, 764, 758, 781, 740]),
    1 << 17: utils.GenericDataPoint(58, [3140, 3265, 3119, 3074, 3137]),
    1 << 18: utils.GenericDataPoint(82, [7316, 7308, 7338, 7406, 7472]),
    1 << 19: utils.GenericDataPoint(116, [19153, 18936, 19200, 19089, 19073]),
    1 << 20: utils.GenericDataPoint(328, [85106, 84490, 84662, 84755, 84576]),
    1 << 21: utils.GenericDataPoint(464, [85106, 84490, 84662, 84755, 84576])
}


def set_color(prms, clr):
    prms["ecolor"] = clr
    prms["color"] = clr


matplotlib.rcParams['font.size'] = constants.font_size

if __name__ == '__main__':
    fig, ax = plt.subplots()
    xs = sorted(sealpir_values.keys())

    sealpir_ys = [*map(lambda x: sealpir_values[x].avg, xs)]
    sealpir_errs = [*map(lambda x: sealpir_values[x].std, xs)]

    utils.plot_errbars(ax, xs, sealpir_ys, sealpir_errs, "", constants.sealpir_clr)

    fpir_ys = [*map(lambda x: fpir_values[x].avg, xs)]
    fpir_errs = [*map(lambda x: fpir_values[x].std, xs)]

    utils.plot_errbars(ax, xs, fpir_ys, fpir_errs, "", constants.addra_clr)

    dpir_ys = [*map(lambda x: dpir_values[x].avg, xs)]
    dpir_errs = [*map(lambda x: dpir_values[x].std, xs)]

    utils.plot_errbars(ax, xs, dpir_ys, dpir_errs, "", constants.dpir_clr)

    utils.add_y_format(ax)

    ax.set_xticks(xs)
    ax.set_xticklabels(["$2^{16}$", "$2^{17}$", "$2^{18}$", "$2^{19}$", "$2^{20}$", "$2^{21}$"])

    ax.set_ylabel('round latency')
    ax.set_xlabel('|messages|')
    ax.legend(["SealPIR (pung's engine)", "FastPIR (Addra's engine)", "DPIR"])
    ax.set_ylim(0)
    fig.tight_layout()

    plt.show()

    print(xs)
    print(np.array(fpir_ys) / np.array(dpir_ys))
    print(np.array(sealpir_ys) / np.array(dpir_ys))

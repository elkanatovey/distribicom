import numpy as np
import matplotlib
from matplotlib import pyplot as plt

import utils, constants
import worker_runtimes

dpir_values = {
    # 1 << 16: utils.GenericDataPoint(82, [1407, 1393, 1395, 1394, 1399, 1402, 1406, 1411, 1398]),
    # 1 << 16: utils.GenericDataPoint(21, [647, 648, 649, 650, 647, 642, 648, 642, 646, ]),  # 42

    # 1 << 17: utils.GenericDataPoint(58, [1503, 1486, 1473, 1478, 1480, 1476, 1478, 1473, 1458, ]),
    # 1 << 17: utils.GenericDataPoint(116, [2933, 2782, 2745, 2706, 2690, 2689, 2693, 2691, 2782]),

    # 1 << 18: utils.GenericDataPoint(82, [3158, 2900, 2908, 2902, 2907, 2902, 2898, 2912, 2909, ]),  # 82x82
    # 1 << 18: utils.GenericDataPoint(164, [5365, 5612, 5229, 5241, 5237, 5238, 5229, 5244, 5219]),

    1 << 19: utils.GenericDataPoint(58, [4633, 4564, 4545, 4570, 4574, 4613, 4717, 4550, 4539]),
    # 1 << 19: utils.GenericDataPoint(116, [6530, 6442, 6479, 6530, 6373, 6523, 6443, 6407, 6594, ]),
    # 1 << 19: utils.GenericDataPoint(232, [11445, 11018, 10842, 10776, 10759, 10676, 10769, 10765, 10707]),

    1 << 20: utils.GenericDataPoint(164, [12611, 12074, 11866, 11936, 11924, 11852, 11917, 11891, 11862]),  # 164x 164
    # 1 << 20: utils.GenericDataPoint(328, [23786, 21804, 21853, 21774, 21816, 21815, 21800, 21761, 21746]),  # 164x 164
    # # 1 << 21: utils.GenericDataPoint(464, [66234, 71135, 70590, 71612, 72395, 71140, 70148, 68856, 68037, ])  # 232*232

    1 << 21: utils.GenericDataPoint(232, [28917, 28015, 29080, 28608, 27981, 28625, 29051, 28477, 28955, ])
    # # 1 << 21: utils.GenericDataPoint(464, [66234, 64309,66729,66707,67325,66569, 68856, 68037]) , # 232*232
    # 1 << 21: utils.GenericDataPoint(696, [111518, 115011, 117747, 119678, ])  # 232*232
}

fpir_values = {
    # fast-pfir
    #    1 << 16: utils.GenericDataPoint(21, [221, 220, 224, 220, 203]),
    #    1 << 17: utils.GenericDataPoint(58, [1172, 1019, 973, 989, 988]),
    #    1 << 18: utils.GenericDataPoint(82, [2503, 2505, 2419, 2502, 2548]),
    #    1 << 19: utils.GenericDataPoint(116, [7131, 7305, 7083, 7056, 7111]),
    #    1 << 20: utils.GenericDataPoint(164, [16989, 17009, 16991, 16977, 16980]),
    #    1 << 21: utils.GenericDataPoint(232, [48105, 48206, 48204, 48168])  # 232*232

    # slow-fpir:
    # 1 << 16: utils.GenericDataPoint(21, [417, 411, 418, 453, 426]),
    # 1 << 17: utils.GenericDataPoint(58, [2249, 2331, 1888, 2127, 2348]),
    # 1 << 18: utils.GenericDataPoint(82, [6794, 5730, 5660, 5789, 5652]),
    # 1 << 19: utils.GenericDataPoint(116, [14020, 14012, 11629, 11922, 31235]),

    1 << 19: utils.GenericDataPoint(58, [5677, 5436, 5432, 5650, 6436]),
    1 << 20: utils.GenericDataPoint(164, [35164, 31964, 32659, 32732, 16980]),
    1 << 21: utils.GenericDataPoint(232, [96085, 90539, 100484, 89251, 89600])  # 232*232
}

sealpir_values = {
    # 1 << 16: utils.GenericDataPoint(84, [664, 677, 670, 683, 670]),
    # 1 << 17: utils.GenericDataPoint(116, [5426, 5476, 5373, 5379, 5446]),
    # 1 << 18: utils.GenericDataPoint(164, [15146, 15341, 15305, 15262, 15271]),
    # 1 << 19: utils.GenericDataPoint(232, [33509, 33543, 33058, 33220, 33585]),

    # 1 << 16: utils.GenericDataPoint(21, [780, 764, 758, 781, 740]),
    # 1 << 17: utils.GenericDataPoint(58, [3140, 3265, 3119, 3074, 3137]),
    # 1 << 18: utils.GenericDataPoint(82, [7316, 7308, 7338, 7406, 7472]),
    # 1 << 19: utils.GenericDataPoint(116, [19153, 18936, 19200, 19089, 19073]),
    # 1 << 20: utils.GenericDataPoint(328, [85106, 84490, 84662, 84755, 84576]),

    1 << 19: utils.GenericDataPoint(58, [8344, 8303, 8309, 8349, 8312]),
    1 << 20: utils.GenericDataPoint(164, [42799, 42422, 42423, 42608, 42467]),
    1 << 21: utils.GenericDataPoint(232, [115399, 123351, 121437, 125887, 131057])
}

workers_multiplication_times = {
    1 << 19: utils.GenericDataPoint(58,
                                    worker_runtimes.extract_time("evals/3rd-graph/slurm_58_workers_116x116.out")[0]),
    1 << 20: utils.GenericDataPoint(164, worker_runtimes.extract_time("evals/scripts_mil_size/slurm-q328-c24.log")[0]),
    1 << 21: utils.GenericDataPoint(232, worker_runtimes.extract_time("evals/2mil-slurm.out")[0])
}


def set_color(prms, clr):
    prms["ecolor"] = clr
    prms["color"] = clr


matplotlib.rcParams['font.size'] = constants.font_size


def main():
    fig, ax = plt.subplots()
    xs = sorted(sealpir_values.keys())
    sealpir_ys = [*map(lambda x: sealpir_values[x].avg, xs)]
    sealpir_errs = [*map(lambda x: sealpir_values[x].std, xs)]

    fpir_ys = [*map(lambda x: fpir_values[x].avg, xs)]
    fpir_errs = [*map(lambda x: fpir_values[x].std, xs)]

    dpir_ys = [*map(lambda x: dpir_values[x].avg, xs)]
    dpir_errs = [*map(lambda x: dpir_values[x].std, xs)]

    dpir_workers_ys = [*map(lambda x: workers_multiplication_times[x].avg, xs)]
    dpir_workers_errs = [*map(lambda x: workers_multiplication_times[x].std, xs)]

    xs = [19, 20, 21]
    utils.plot_errbars(ax, xs, sealpir_ys, sealpir_errs, "", constants.sealpir_clr)
    utils.plot_errbars(ax, xs, fpir_ys, fpir_errs, "", constants.addra_clr)
    utils.plot_errbars(ax, xs, dpir_ys, dpir_errs, "", constants.dpir_clr)
    utils.plot_errbars(ax, xs, dpir_workers_ys, dpir_workers_errs, "", constants.dpir_clr, is_dashed=True)

    utils.add_y_format(ax)
    ax.set_xticks(xs)

    ax.set_xticklabels(["$2^{19}$", "$2^{20}$", "$2^{21}$"])
    ax.set_ylabel('round latency')
    ax.set_xlabel('|messages|')
    ax.legend(["SealPIR (Pung's engine)", "FastPIR (Addra's engine)", "DPIR", "DPIR workers"])
    ax.set_ylim(0)
    fig.tight_layout()
    plt.show()
    print(xs)
    print(np.array(fpir_ys) / np.array(dpir_ys))
    print(np.array(sealpir_ys) / np.array(dpir_ys))


if __name__ == '__main__':
    main()

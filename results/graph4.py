import numpy as np
import matplotlib
from matplotlib import pyplot as plt

import utils, constants

# x db size to times.
dpir_values = {
    10: [28967, 28860, 28715, 28456, 28427, 29059, 27729, 28211, 28438],
    20: [19962, 18139, 19332, 18147, 17787, 18450, 18698, 17859, 17848],
    40: [14503, 14201, 14288, 14208, 14250, 14129],
    60: [13793, 13806, 13760, 13585, 13783, 13650, 13922, 14133, 13772],
    80: [14158, 13935, 13912, 14026, 14101, 13750, 14152, 14073, 13944],
    100: [13989, 13855, 13735, 13882, 13867, 13951, 13826, 13741, 13815]
}
matplotlib.rcParams['font.size'] = constants.font_size

if __name__ == '__main__':
    fig, ax = plt.subplots()

    xs = sorted(dpir_values.keys())
    ys = [np.mean(dpir_values[x]) for x in xs]
    yerr = [np.std(dpir_values[x]) for x in xs]

    utils.add_y_format(ax)

    clr = constants.dpir_clr
    pr = {
        "markersize": 4,
        "barsabove": True,
        "capsize": 2,
        "linewidth": constants.line_size - 1,
        "elinewidth": constants.line_size,
        "ecolor": clr,
        "color": clr,
    }

    ax.errorbar(xs, ys, yerr=yerr, **pr)
    ax.set_xlabel("mbit/s")
    ax.set_ylabel("round latency")
    ax.set_xticks([10, 20, 40, 60, 80, 100])
    # ax.set_ylim(0, max(ys)*1.05)  # Use `max(y)` to automatically set the upper limit to the maximum value in y
    # ax. = "Number of queries"
    ax.set_ylim(0)
    plt.show()

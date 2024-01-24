import copy

import numpy as np
import matplotlib
from matplotlib import pyplot as plt

import utils, constants

# x db size to times.
dpir_values_1m_db = {
    10: [28967, 28860, 28715, 28456, 28427, 29059, 27729, 28211, 28438],
    20: [19962, 18139, 19332, 18147, 17787, 18450, 18698, 17859, 17848],
    40: [14503, 14201, 14288, 14208, 14250, 14129],
    60: [13793, 13806, 13760, 13585, 13783, 13650, 13922, 14133, 13772],
    80: [14158, 13935, 13912, 14026, 14101, 13750, 14152, 14073, 13944],
    100: [13989, 13855, 13735, 13882, 13867, 13951, 13826, 13741, 13815]
}

dpir_values_65k_db = {
    10: [4591, 4589, 4592, 4589, 4592, 4590, 4592, 4592, 4590],
    20: [2289, 2297, 2295, 2297, 2295, 2298, 2291, 2297, 2295],
    40: [1144, 1148, 1146, 1151, 1145, 1148, 1147, 1148, 1146],
    60: [801, 794, 792, 856, 820, 809, 804, 801, 801],
    80: [811, 805, 810, 813, 801, 800, 793, 796, 789],
    100: [844, 809, 801, 821, 830, 801, 801, 809, 783],
}

matplotlib.rcParams['font.size'] = constants.font_size - 8

plot_params = prms = {
    'fmt': '-o',
    'barsabove': True,
    'capsize': constants.marker_size,

    'linewidth': constants.line_size,
    'markersize': constants.marker_size,
}


def plot_line(ax, values_dict, clr, nm):
    xs = sorted(values_dict.keys())
    ys = [np.mean(values_dict[x]) for x in xs]
    yerr = [np.std(values_dict[x]) for x in xs]
    utils.add_y_format(ax)

    ax.errorbar(
        xs, ys,
        yerr=yerr,
        **edit_plot_params(plot_params, clr, nm)
    )
    # utils.plot_errbars(ax, xs, ys, yerr, nm, clr)


def edit_plot_params(dict, clr, nm, fmt='-'):
    plt_prms = copy.deepcopy(dict)
    plt_prms['color'] = clr
    plt_prms['label'] = nm
    plt_prms['ecolor'] = clr
    plt_prms['fmt'] = fmt

    return plt_prms


def sealpir_plot(ax, xs):
    sealpir_164 = [42799, 42422, 42423, 42608, 42467]
    sealpir_line = [np.mean(sealpir_164)] * len(xs)

    sealpir_err = [np.std(sealpir_164)] * len(xs)
    ax.errorbar(xs, sealpir_line, yerr=sealpir_err,
                **edit_plot_params(plot_params, '#E9D985', 'Sealpir $2^{20}$ messages', fmt='--')
                )


def fast_pir_plot(ax, xs):
    addra_results = [29435, 30146, 29437, 29426, 29443]
    addras_164_points_unchained = [np.mean(addra_results)] * len(xs)
    addra_err = [np.std(addra_results)] * len(xs)

    ax.errorbar(xs, addras_164_points_unchained, yerr=addra_err,
                **edit_plot_params(plot_params, '#6E7579', 'Fastpir $2^{20}$ messages', fmt='--')
                )


def main():
    fig, ax = plt.subplots()

    xs = [10, 20, 40, 60, 80, 100]
    # sealpir_plot(ax, xs)
    # fast_pir_plot(ax, xs)

    # constants.change_sizes(2)
    plot_line(ax, dpir_values_1m_db, constants.dpir_clr, "DPIR $2^{20}$ messages")
    plot_line(ax, dpir_values_65k_db, constants.addra_clr, "DPIR $2^{16}$ messages")
    ax.set_xlabel("mbit/s")
    ax.set_ylabel("round latency")
    ax.set_xticks(xs)
    # ax.set_ylim(0)

    ax.legend(loc='upper right', framealpha=0.3)
    fig.tight_layout()
    plt.show()


if __name__ == '__main__':
    main()

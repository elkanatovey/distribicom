import os
from typing import List

import graph1
import constants

import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['font.size'] = constants.font_size


def plot_dpir_line(ax, test_results: List[graph1.TestResult]):
    test_results = sorted(test_results, key=lambda x: x.num_queries)

    xs = [0, *(test_result.num_queries for test_result in test_results)]
    ys = [0, *(test_result.data[0] for test_result in test_results)]
    ax.plot(
        xs,
        ys,
        marker='o',
        color=constants.dpir_clr,
        linewidth=constants.line_size,
        markersize=constants.line_size + 1
    )


if __name__ == '__main__':
    test_results = graph1.collect_test_results("./1thread")

    fig, ax = plt.subplots()
    plot_dpir_line(ax, test_results)
    ax.legend(["dPIR first round"])

    ax.set_xticks([0, 42, 84, 126, 168])
    ax.set_yticklabels(map(lambda x: str(x) + "s", [0, 0, 1, 2, 3, 4, 5]))

    ax.set_xlabel('number of queries')
    ax.set_ylabel('round latency')

    plt.show()

import os
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import constants

import graph1
from utils import *

matplotlib.rcParams['font.size'] = constants.font_size


def grab_sealpir_results_from_file(fname):
    lines = []
    with open(fname) as f:
        lines = f.readlines()

    resmap = {}
    for l in lines:
        l = l.strip()
        l = l.split(" ")
        n_queries = int(l[8])
        if n_queries not in resmap:
            resmap[n_queries] = []
        time = int(l[5])
        resmap[n_queries].append(time)
    return [*map(lambda k: graph1.GenericDataPoint(k, resmap[k]), resmap)]


sealpir = [
    graph1.GenericDataPoint(164, [42799, 42422, 42423, 42608, 42467]),
    graph1.GenericDataPoint(328, [86106, 84490, 84662, 84755, 84576]),
    graph1.GenericDataPoint(492, [124656, 123813, 123998, 124460, 123978]),
    graph1.GenericDataPoint(656, [180675, 175734, 176022, 180259, 184216])
]

step_two_times = [

]

# colour-pallet: https://coolors.co/443d4a-55434e-ba6567-fe5f55-e3a792
if __name__ == '__main__':
    main_folder = "evals/scripts_mil_size/64_workers_per_node"
    dpir_test_results = collect_dpir_test_results(main_folder)

    sealpir = grab_sealpir_results_from_file("evals/scripts_mil_size/sealpir")

    fig, ax = plt.subplots()

    graph1.plot_dpir_line(ax, dpir_test_results)
    # graph1.plot_dpir_step_2_times(ax, step_two_times)

    graph1.plot_other_sys_results(ax, sealpir)

    ax.legend()

    ax.set_xticks(get_from_dpir_results_x_axis(dpir_test_results))

    ax.set_yticks([i * 20000 for i in range(10)])
    ax.set_yticklabels(map(lambda x: str(int(x * 20)) + "s", range(0, 10, 1)))

    ax.set_xlabel('number of clients')
    ax.set_ylabel('round latency')

    plt.show()
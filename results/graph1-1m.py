import os
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import constants

import graph1
from utils import *

matplotlib.rcParams['font.size'] = constants.font_size

# colour-pallet: https://coolors.co/443d4a-55434e-ba6567-fe5f55-e3a792
if __name__ == '__main__':
    main_folder = "evals/scripts_mil_size/64_workers_per_node"
    dpir_test_results = collect_dpir_test_results(main_folder)

    fig, ax = plt.subplots()

    graph1.plot_dpir_line(ax, dpir_test_results)

    ax.legend()

    ax.set_xticks(get_from_dpir_results_x_axis(dpir_test_results))

    ax.set_yticks([i * 5000 for i in range(8)])
    ax.set_yticklabels(map(lambda x: str(int(x / 2 * 10)) + "s", range(0, 8, 1)))

    ax.set_xlabel('number of clients')
    ax.set_ylabel('round latency')

    plt.show()

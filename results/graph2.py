import os
from typing import List

import numpy as np

import graph1
import utils
import constants

import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['font.size'] = constants.font_size


class EpochSetupTime:
    @staticmethod
    def is_test_result(file_name: str) -> bool:
        return not any(word in file_name for word in ["server", "ignore", "addra", "singleserverresults"])

    def __init__(self, file_name):

        self.file_name = file_name
        self.n_queries = 0
        self.server_setup = 0
        self.gal_key_send_time = 0
        self.worker_expansion_times = []

        self.parse_file()

        self.expansion_time = max(self.worker_expansion_times)
        self.total = self.setup_time()

    def setup_time(self):
        return self.server_setup + self.gal_key_send_time + self.expansion_time

    def parse_file(self):
        with open(self.file_name, "r") as f:
            self.collect_data(f)

    def collect_data(self, f):
        for line in f:
            if "Manager::new_epoch:" in line:
                self.collect_server_setup_time(line)
            elif "worker::expansion time" in line:
                self.collect_expansion_time(line)
            elif "server set up with" in line and "queries" in line:
                self.set_n_queries(line)
            elif "Manager::send_galois_keys" in line:
                self.collect_galois_key_time(line)

    def collect_server_setup_time(self, line):
        """Manager::new_epoch: 1152 ms"""
        self.server_setup = int(line.split(" ")[1])

    def collect_expansion_time(self, line):
        self.worker_expansion_times.append(int(line.split(" ")[2]))

    def set_n_queries(self, line):
        self.n_queries = int(line.split(" ")[-2])

    def collect_galois_key_time(self, line):
        "Manager::send_galois_keys: 239ms"
        self.gal_key_send_time = int(line.split(" ")[1][:-3])


def plot_dpir_line(ax, test_results: List[utils.TestResult], clrr=constants.epoch_setup):
    test_results = sorted(test_results, key=lambda x: x.num_queries)

    xs = [*(test_result.num_queries for test_result in test_results)]
    ys = [*(test_result.avg for test_result in test_results)]
    yerrs = [*(np.std(test_result.data) for test_result in test_results)]
    utils.plot_errbars(ax, xs, ys, yerrs, "", clrr)


if __name__ == '__main__':
    fldrs = [
        "evals/2nd-graph/65k",
        "evals/2nd-graph/262k",
        "evals/2nd-graph/1m",
    ]
    clrs = [
        constants.epoch_setup,
        constants.dpir_clr,
        constants.sealpir_clr,
    ]

    fig, ax = plt.subplots()
    for i, fldr in enumerate(fldrs):
        test_results = utils.collect_dpir_test_results(fldr)

        plot_dpir_line(ax, test_results, clrs[i])

        ax.set_xlabel('clients per server')
        ax.set_ylabel('setup time')

    ax.legend([
        '$2^{16}$ messages',
        '$2^{18}$ messages',
        '$2^{20}$ messages',
    ])
    utils.add_y_format(ax)
    ax.set_ylim(0)
    plt.show()

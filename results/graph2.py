import os
from typing import List

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


def plot_dpir_line(ax, test_results: List[utils.TestResult]):
    test_results = sorted(test_results, key=lambda x: x.num_queries)

    xs = [*(test_result.num_queries for test_result in test_results)]
    ys = [*(test_result.data[0] for test_result in test_results)]

    ax.plot(
        xs,
        ys,
        marker='o',
        color=constants.epoch_setup,
        linewidth=constants.line_size,
        markersize=constants.line_size + 1
    )


# def collect_test_results(folder_path):
#     filtered = filter(lambda name: .is_test_result(name), graph1.get_all_fnames(folder_path))
#     test_results = [*map(lambda fname: EpochSetupTime(os.path.join(folder_path, fname)), filtered)]
#
#     if len(test_results) == 0:
#         raise Exception("No test results found.")
#     return test_results


if __name__ == '__main__':
    test_results = utils.collect_dpir_test_results("evals/65k_size/64_workers_per_node/combined")

    fig, ax = plt.subplots()
    plot_dpir_line(ax, test_results)
    # ax.legend([])

    ax.set_xticks([*utils.get_from_dpir_results_x_axis(test_results)])

    ax.set_yticks([i * 2000 for i in range(8)])
    ax.set_yticklabels(map(lambda x: str(2 * x) + "s", range(8)))

    ax.set_xlabel('number of clients')
    ax.set_ylabel('setup time')

    plt.show()

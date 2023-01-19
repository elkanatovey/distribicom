import os
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import constants

matplotlib.rcParams['font.size'] = constants.font_size


class SealPirResults:
    def __init__(self, queries, times):
        # self.name = name
        self.queries = queries
        self.avg = np.average(times)
        self.std = np.std(times)


# according to Elkana's collected results.
sealpir_results_42_queries = SealPirResults(42, [1519, 1476, 1492, 1577, 1517])
sealpir_results_84_queries = SealPirResults(84, [2737, 2626, 2575, 2643, 2619])
sealpir_results_126_queries = SealPirResults(126, [3341, 3345, 3354, 3359, 3371])
sealpir_results_168_queries = SealPirResults(168, [5114, 5053, 4941, 4767, 5041])


class TestResult:
    @staticmethod
    def is_test_result(file_name: str) -> bool:
        return not any(word in file_name for word in ["slurm", "ignore"])

    def __init__(self, file_name: str):

        self.file_name = file_name
        # general info.
        self.num_threads_per_worker = 1
        self.num_cores = 12
        self.num_vcpus = 24
        self.server_num_threads = 12

        self.num_workers = int(file_name.split("_")[2])
        self.num_queries = self.num_workers

        self.data = []  # initialise
        self.parse_file()

    def parse_file(self):
        with open(self.file_name, "r") as f:
            self.move_seeker_to_results(f)
            self.collect_data(f)

    def move_seeker_to_results(self, f):
        for line in f:
            if "results:" not in line:
                continue
            break

    def collect_data(self, f):
        f.readline()  # skip the first line ( "[" ).
        self.data = []
        for line in f:
            if "]" in line:
                break
            self.data.append(line[:-2].strip())

        self.data = [int(x[:-3]) for x in self.data]  # remove "ms," from each data point


def plot_dpir_line(ax, test_results: List[TestResult]):
    test_results = sorted(test_results, key=lambda x: x.num_queries)

    xs = [0, *(test_result.num_queries for test_result in test_results)]
    ys = [0, *(np.average(test_result.data[1:]) for test_result in test_results)]
    errbars = [0, *(np.std(test_result.data[1:]) for test_result in test_results)]
    print("dpir", ys)
    ax.plot(
        xs,
        ys,
        marker='o',
        color=constants.dpir_clr,
        linewidth=constants.line_size,
        markersize=constants.line_size + 1
    )
    ax.errorbar(
        xs,
        ys,
        yerr=errbars,
        fmt='.',
        markersize=4,
        barsabove=True,
        capsize=2,
        ecolor=constants.dpir_clr,
        color=constants.dpir_clr,
        elinewidth=constants.line_size - 1
    )


def plot_sealpir_line(ax):
    ys = sorted(
        [
            sealpir_results_42_queries,
            sealpir_results_84_queries,
            sealpir_results_126_queries,
            sealpir_results_168_queries
        ],
        key=lambda x: x.queries,
    )

    xs = [0, *(result.queries for result in ys)]
    errbars = [0, *map(lambda y: y.std, ys)]
    print(errbars)
    ys = [0, *map(lambda y: y.avg, ys)]
    print("sealpir", ys)
    ax.plot(
        xs,
        ys,
        color=constants.sealpir_clr,
        marker='o',
        linewidth=constants.line_size,
        markersize=constants.line_size + 1
    )
    ax.errorbar(
        xs,
        ys,
        yerr=errbars,
        fmt='.',
        barsabove=True,
        capsize=2,
        ecolor=constants.sealpir_clr,
        color=constants.sealpir_clr
    )


def collect_test_results(folder_path):
    filtered = filter(lambda name: TestResult.is_test_result(name), get_all_fnames(folder_path))
    test_results = [*map(lambda fname: TestResult(os.path.join(folder_path, fname)), filtered)]

    if len(test_results) == 0:
        raise Exception("No test results found.")
    return test_results


def get_all_fnames(folder_path):
    file_names = []
    for filename in os.listdir(folder_path):
        file_names.append(filename)
    return file_names


# def plot_epoch_line(ax, test_results: List[TestResult]):
#     test_results = sorted(test_results, key=lambda x: x.num_queries)
#
#     xs = [0, *(test_result.num_queries for test_result in test_results)]
#     ys = [0, *(test_result.data[0] for test_result in test_results)]
#     ax.plot(
#         xs,
#         ys,
#         marker='o',
#         color=constants.epoch_setup,
#         linewidth=constants.line_size,
#         markersize=constants.line_size + 1
#     )


# colour-pallet: https://coolors.co/443d4a-55434e-ba6567-fe5f55-e3a792
if __name__ == '__main__':
    test_results = collect_test_results("./1thread")

    fig, ax = plt.subplots()
    plot_dpir_line(ax, test_results)
    plot_sealpir_line(ax)

    ax.legend(["dPIR", "sealPIR"])

    ax.set_xticks([0, 42, 84, 126, 168])
    # ax.set_yticklabels(map(lambda x: str(x) + "s", [0, 0, 1, 2, 3, 4, 5]))

    ax.set_xlabel('number of clients')
    ax.set_ylabel('round latency')

    plt.show()

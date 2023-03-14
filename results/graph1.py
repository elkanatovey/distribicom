import os
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import constants

from utils import *

matplotlib.rcParams['font.size'] = constants.font_size


# according to Elkana's collected results.
# GenericDataPoint(, []),


def plot_dpir_line(ax, test_results: List[TestResult]):
    test_results = sorted(test_results, key=lambda x: x.num_queries)

    xs = [*(test_result.num_queries for test_result in test_results)]
    ys = [*(np.average(test_result.data[1:]) for test_result in test_results)]
    errbars = [*(np.std(test_result.data[1:]) for test_result in test_results)]
    print("dpir", ys)
    ax.plot(
        xs,
        ys,
        marker='o',
        color=constants.dpir_clr,
        linewidth=constants.line_size,
        markersize=constants.line_size + 1,
        label="DPIR"
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


def plot_other_sys_results(ax, sealpir_results: List[GenericDataPoint], clr=constants.sealpir_clr, label="SealPIR"):
    ys = sorted(
        sealpir_results,
        key=lambda x: x.queries,
    )

    xs = [*(result.queries for result in ys)]
    errbars = [*map(lambda y: y.std, ys)]
    print(errbars)
    ys = [*map(lambda y: y.avg, ys)]
    print("sealpir", ys)
    ax.plot(
        xs,
        ys,
        color=clr,
        marker='o',
        linewidth=constants.line_size,
        markersize=constants.line_size + 1,
        label=label
    )
    ax.errorbar(
        xs,
        ys,
        yerr=errbars,
        fmt='.',
        barsabove=True,
        capsize=2,
        ecolor=clr,
        color=clr
    )


class SingleServerParsing:
    @staticmethod
    def is_test_result(file_name: str) -> bool:
        return file_name == "singleserverresults"

    def __init__(self, filename):
        self.filename = filename
        self.data_points = {}  # {num_queries, [data_points]}
        self.parse_file()

    def parse_file(self):
        "Main: pool query processing time: 1981 ms on 95 queries and 95 threads"
        with open(self.filename, "r") as f:
            for line in f:
                self.collect_line(line)

    def collect_line(self, line):
        time = int(line.split(" ")[5])
        n_queries = int(line.split(" ")[8])
        if n_queries not in self.data_points:
            self.data_points[n_queries] = []
        self.data_points[n_queries].append(time)

    def into_sealpir_result_list(self):
        return [*map(lambda x: GenericDataPoint(x[0], x[1]), self.data_points.items())]


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


class AddraResult:
    @staticmethod
    def is_test_result(file_name: str) -> bool:
        return file_name.startswith("addra")

    def __init__(self, file_name: str):
        self.file_name = file_name
        self.data = []  # initialise
        self.queries = int(os.path.basename(file_name).split("_")[1])
        self.avg = 0
        self.std = 0
        self.parse_file()

    def parse_file(self):
        with open(self.file_name, "r") as f:
            self.move_seeker_to_results(f)
            self.collect_data(f)

    def move_seeker_to_results(self, f):
        f.readline()

    def collect_data(self, f):
        for line in f:
            while len(line.split(" ")) != 3:
                line = line.replace("  ", " ")
            total_time = line.strip().split(" ")[2]
            self.data.append(int(total_time))

        # convert us to ms:
        self.data = [*map(lambda x: x // 1000, self.data)]

    def into_generic_data_point(self):
        return GenericDataPoint(self.queries, self.data)


def addra_plot(ax, main_folder):
    fs = [*filter(AddraResult.is_test_result, get_all_fnames(main_folder))]
    addra_results = [*map(lambda x: AddraResult(os.path.join(main_folder, x)).into_generic_data_point(), fs)]
    plot_other_sys_results(ax, addra_results, clr=constants.addra_clr, label="Addra")


# these times are for 42 queries, 84 queries, ...
step_two_times = [
    [106, 106, 107, 104, 105, 106],
    [223, 208, 216, 205, 206, 207, 209],
    [311, 309, 311, 310, 309, 318, 314],
    [413, 411, 412, 414, 413, 411, 412],
    [514, 516, 516, 517, 515, 516, 515],
    [674, 626, 632, 625, 620, 621, 619],
    [720, 727, 722, 719, 723, 726, 723],
    [866],
    [955, 953, 958, 962, 956, 960, 970],
    [1066, 1065, 1055, 1044, 1058, 1044, 1051],
    [1170, 1151, 1132, 1130, 1135, 1134, 1160],
    [1272, 1279, 1278, 1279, 1287],
]


def plot_dpir_step_2_times(ax, step_two_times):
    xs = np.arange(1, len(step_two_times) + 1) * 42
    ys, errbars = zip(*map(lambda lst: [np.average(lst), np.std(lst)], step_two_times))
    ax.errorbar(
        xs,
        ys,
        yerr=errbars,
        fmt='--.',
        markersize=4,
        barsabove=True,
        capsize=2,
        ecolor=constants.dpir_clr,
        color=constants.dpir_clr,
        elinewidth=constants.line_size - 1
    )


# colour-pallet: https://coolors.co/443d4a-55434e-ba6567-fe5f55-e3a792
# https://coolors.co/e9d985-b2bd7e-749c75-6a5d7b-5d4a66
if __name__ == '__main__':
    data = [
        ("evals/65k_size/64_workers_per_node/combined", "evals/65k_size/sealpir"),
        ("evals/256k", "evals/256k/sealpir"),
        ("evals/scripts_mil_size/64_workers_per_node", "evals/scripts_mil_size/sealpir"),
    ]
    # main_folder = "evals/65k_size/64_workers_per_node/combined"
    # sealpir_res = grab_sealpir_results_from_file("evals/65k_size/sealpir")

    for dpir_data_folder, sealpir_data_folder in data:
        fig, ax = plt.subplots()
        dpir_test_results = collect_dpir_test_results(dpir_data_folder)
        sealpir_res = grab_sealpir_results_from_file(sealpir_data_folder)
        max_x = max([*get_from_dpir_results_x_axis(dpir_test_results)])

        sealpir_res = [*filter(lambda x: x.queries <= max_x, sealpir_res)]
        plot_other_sys_results(ax, sealpir_res)

        plot_dpir_line(ax, dpir_test_results)
        # plot_dpir_step_2_times(ax, step_two_times) # TODO: find a way to add this too.
        # addra_plot(ax, main_folder)

        ax.legend()

        # ax.set_xticks([*get_from_dpir_results_x_axis(dpir_test_results)])

        # ax.set_yticks([i * 2000 for i in range(7)])
        # ax.set_yticklabels(map(lambda x: str(2 * x) + "s", range(7)))

        ax.set_xlabel('number of clients')
        ax.set_ylabel('round latency')

    plt.show()

from matplotlib import ticker
import numpy as np
import os
import constants
import matplotlib

matplotlib.rcParams['font.size'] = constants.font_size


def plot_errbars(ax, xs, ys, errbars, label, clr, is_dashed=False):
    fmt = "-o"
    if is_dashed:
        fmt = "--o"
    ax.errorbar(
        xs,
        ys,
        label=label,
        yerr=errbars,
        fmt=fmt,
        barsabove=True,
        capsize=constants.marker_size,
        ecolor=clr,
        color=clr,

        linewidth=constants.line_size,
        markersize=constants.marker_size,
    )


class TestResult:
    @staticmethod
    def is_test_result(file_name: str) -> bool:
        return not any(word in file_name for word in ["slurm", "ignore", "addra", "singleserverresults"])

    def __init__(self, file_name: str):

        self.file_name = file_name
        # general info.
        self.num_threads_per_worker = 1
        self.num_cores = 12
        self.num_vcpus = 24
        self.server_num_threads = 12

        self.num_workers = int(os.path.basename(file_name).split("_")[2])
        self.num_queries = self.num_workers

        self.data = []  # initialise
        self.parse_file()
        self.avg = np.average(self.data[:])

    def parse_file(self):
        with open(self.file_name, "r") as f:
            self.move_seeker_to_results(f)
            self.collect_data(f)

    def move_seeker_to_results(self, f):
        for line in f:
            if "results:" not in line:
                continue
            break
        f.readline()  # skip the first line ( "[" ).

    def collect_data(self, f):
        self.data = []
        for line in f:
            if "]" in line:
                break
            self.data.append(line.strip())

        self.data = [int(x[:-3]) for x in self.data]  # remove "ms," from each data point
        if len(self.data) < 3:
            raise Exception("Not enough data points")


def get_all_fnames(folder_path):
    file_names = []
    for filename in os.listdir(folder_path):
        file_names.append(filename)
    return file_names


def get_from_dpir_results_x_axis(test_results):
    return sorted(map(lambda x: x.num_queries, test_results))


def collect_dpir_test_results(folder_path):
    filtered = filter(lambda name: TestResult.is_test_result(name), get_all_fnames(folder_path))
    test_results = []
    for fname in filtered:
        try:
            test_results.append(TestResult(os.path.join(folder_path, fname)))
        except Exception as e:
            print(f"skipping {fname} due to error: {e}")

    if len(test_results) == 0:
        raise Exception("No test results found.")
    return test_results


class GenericDataPoint:
    def __init__(self, queries, times):
        # self.name = name
        self.queries = queries
        self.avg = np.average(times)
        self.std = np.std(times)


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
    return [*map(lambda k: GenericDataPoint(k, resmap[k]), resmap)]


def add_y_format(ax):
    def y_fmt(y, _):
        return '{:.0f}s'.format(y / 1000)

    # Apply the formatter function to the y-axis tick labels
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(y_fmt))

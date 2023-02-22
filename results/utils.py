import numpy as np
import os


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
        self.avg = np.average(self.data[1:])

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
            self.data.append(line[:-2].strip())

        self.data = [int(x[:-3]) for x in self.data]  # remove "ms," from each data point
        if len(self.data) < 5:
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
        try :
            test_results.append(TestResult(os.path.join(folder_path, fname)))
        except Exception as e:
            print(f"skipping {fname} due to error: {e}")

    if len(test_results) == 0:
        raise Exception("No test results found.")
    return test_results

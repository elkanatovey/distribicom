import os.path

import numpy as np


def get_expansion_times(path):
    with open(path) as f:
        exp_times = []
        for line in f:
            line = line.strip()
            exp_times.append(int(line.split(" ")[2]))
        return np.array(exp_times)


def print_expantion_times(exp_times):
    avg = np.average(exp_times)
    std = np.std(exp_times)
    print(f"avg: {avg}, std: {std}")


if __name__ == '__main__':
    exp_times = get_expansion_times(os.path.join("evals", "expansion-times", "expansion-time-42x41.out"))
    print_expantion_times(exp_times / 42)

    exp_times = get_expansion_times(os.path.join("evals", "expansion-times", "expansion-time-164x164"))
    print_expantion_times(exp_times / 164)

# collects the runtimes of the workers from the log files
from typing import List
from numbers import Number

logs = [
    ("65k", "evals/65k_size/64_workers_per_node/combined/slurm-5816604.out"),
    ("256k", "evals/256k/slurm-6291958.out"),
    ("1M", "evals/scripts_mil_size/slurm-q328-c24.log")
]
float_precision = 3


def extract_time(log_file_path: str) -> (List[Number], List[Number]):
    worker_processing_times: List[float] = []
    worker_expansion_times: List[float] = []

    with open(log_file_path, "r") as f:
        for line in f:
            if "worker::multiplication time:" in line:
                worker_multiplication_time = line.split("worker::multiplication time:")[1].strip().replace("ms", "")
                worker_processing_times.append(float(worker_multiplication_time))
                continue

            if "worker::expansion time:" in line:
                worker_expansion_time = line.split("worker::expansion time:")[1].strip().replace("ms", "")
                worker_expansion_times.append(float(worker_expansion_time))
                continue

    return worker_processing_times, worker_expansion_times


def avg(lst: List[Number]) -> float:
    return sum(lst) / len(lst)


def main():
    for size, log in logs:
        worker_processing_times, worker_expansion_times = extract_time(log)

        print(
            f"db size: {size}, avg worker multiplication time: {round(avg(worker_processing_times), float_precision)}ms")
        print(f"db size: {size}, avg worker expansion time: {round(avg(worker_expansion_times), float_precision)}ms")


if __name__ == '__main__':
    main()

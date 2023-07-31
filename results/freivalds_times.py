import numpy as np

look_for = "Manager::async_verify_worker::verification time:"


def grab_verification_times(fname):
    times = []
    with open(fname, 'r') as f:
        for line in f:
            try:
                if look_for in line:
                    time = float(line.split()[-2])
                    times.append(time)
            except Exception as e:
                continue
    return times


if __name__ == '__main__':
    for (db_size, fname) in [
        ("65k", "evals/65k_size/32_workers_per_node/third_run/slurm-5816869.out"),
        ("256k", "evals/256k/slurm-6291958.out"),
        ("1m", "evals/scripts_mil_size/slurm-q328-c24.log"),
        ("2m", "evals/2mil-slurm.out"),
    ]:
        print(f"{db_size}: {np.average(grab_verification_times(fname))} ms")

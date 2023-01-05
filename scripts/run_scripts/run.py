#!/cs/labs/yossigi/elkanatovey/CLionProjects/distribicom/venv/bin/python3
import argparse
import json
import os
import subprocess
import tempfile
import time
from typing import Dict, Any, List
import math

import google.protobuf.text_format as tx
from distribicom_pb2 import *


def elements_per_ptxt(logt: int, N: int, ele_size: int):
    coeff_per_ele = math.ceil(8 * ele_size / float(logt))
    ele_per_ptxt = N / coeff_per_ele
    assert (ele_per_ptxt > 0)

    return ele_per_ptxt


def create_app_configs(configs: Dict[str, Any], server_hostname) -> Configs:
    cnfgs = Configs()

    cnfgs.scheme = configs["scheme"]
    cnfgs.polynomial_degree = configs["polynomial_degree"]
    cnfgs.logarithm_plaintext_coefficient = configs["logarithm_plaintext_coefficient"]
    cnfgs.db_rows = configs["db_rows"]
    cnfgs.db_cols = configs["db_cols"]
    cnfgs.size_per_element = configs["size_per_element"]

    cnfgs.dimensions = 2
    cnfgs.number_of_elements = cnfgs.db_rows * cnfgs.db_cols * \
                               int(elements_per_ptxt(
                                   cnfgs.logarithm_plaintext_coefficient,
                                   cnfgs.polynomial_degree,
                                   cnfgs.size_per_element
                               ))

    cnfgs.use_symmetric = True
    cnfgs.use_batching = True
    cnfgs.use_recursive_mod_switching = False

    app_cnfgs = AppConfigs()

    app_cnfgs.number_of_workers = configs["number_of_workers"]
    app_cnfgs.main_server_hostname = f'{server_hostname}:{54341}'
    app_cnfgs.query_wait_time = 10  # seconds
    app_cnfgs.worker_num_cpus = configs["worker_num_cpus"]
    app_cnfgs.server_cpus = configs["server_cpus"]
    app_cnfgs.configs.CopyFrom(cnfgs)

    return app_cnfgs


def load_test_settings(test_settings_file: str):
    with open(test_settings_file, 'r') as f:
        all = json.load(f)
        return all


def get_all_hostnames(dir_path, hostname_suffix):
    for hostname_file in get_all_hostname_files(hostname_suffix, dir_path):
        with open(os.path.join(dir_path, hostname_file), 'r') as f:
            yield f.read().strip()


class Settings:
    def __init__(self, settings_file_name):
        self.sleep_time = 5
        self.hostname_suffix = ".hostname"

        self.all = load_test_settings("test_setting.json")

        test_setup = self.all["test_setup"]
        self.num_cpus = test_setup["number_of_cpus_on_server"]  # os.cpu_count() on cluster isn't accurate
        self.num_workers = test_setup["number_of_workers_per_server"]
        self.num_servers = test_setup["number_of_servers"]
        self.workers_print = test_setup["workers_print"]
        self.test_dir = test_setup["shared_folder"]

        self.binaries = self.all["binaries"]
        self.worker_bin = self.binaries["worker"]
        self.server_bin = self.binaries["main_server"]

        self.num_clients = self.all["num_clients"]

        self.app_configs_filename = os.path.join(self.test_dir, "app_configs.txt")
        self.configs = self.all["configs"]
        self.configs["server_cpus"] = self.num_cpus
        self.configs["worker_num_cpus"] = self.num_cpus // self.num_workers


def command_line_args():
    global parser, args
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-c", "--config_file",
        help="path to config file",
        type=str,
        default="test_setting.json"
    )
    return parser.parse_args()


def setup_server(settings, hostname) -> List[subprocess.Popen]:
    print("main server creating configs file")
    app_configs = create_app_configs(settings.configs, hostname)
    with open(settings.app_configs_filename, 'wb') as f:
        f.write(app_configs.SerializeToString())
        print(f"configs written to {settings.app_configs_filename}")
    return [subprocess.Popen([settings.server_bin, settings.app_configs_filename, settings.num_clients])]


def setup_workers(settings) -> List[subprocess.Popen]:
    print("waiting for main server")
    time.sleep(settings.sleep_time)
    out = []
    print(f"creating {settings.num_workers} workers")
    for i in range(settings.num_workers):
        out.append(subprocess.Popen([settings.worker_bin, settings.app_configs_filename]))
    return out


def get_all_hostname_files(hostname_suffix, test_dir):
    return set(filter(lambda x: hostname_suffix in x, os.listdir(test_dir)))


if __name__ == '__main__':
    args = command_line_args()

    settings = Settings(args.config_file)
    hostname = subprocess.run(['hostname'], stdout=subprocess.PIPE).stdout.decode("utf-8").strip()

    fd, hostname_file = tempfile.mkstemp(dir=settings.test_dir, suffix=settings.hostname_suffix, prefix="distribicom_")
    os.write(fd, hostname.encode("utf-8"))
    os.close(fd)

    print(f"written hostname to {hostname_file}")
    print(f"waiting for all other running processes to write.")
    while len(get_all_hostname_files(settings.hostname_suffix, settings.test_dir)) < settings.num_servers:
        print("hostname files found:", len(get_all_hostname_files(settings.hostname_suffix, settings.test_dir)))
        time.sleep(1)

    all_hostnames = sorted(get_all_hostnames(settings.test_dir, settings.hostname_suffix))
    is_main_server = hostname == all_hostnames[0]

    subprocesses = setup_server(settings, hostname) if is_main_server else setup_workers(settings)
    try:
        for (i, sub) in enumerate(subprocesses):
            sub.wait()

    except KeyboardInterrupt:
        print("received KeyboardInterrupt, killing subprocess...")
        for sub in subprocesses:
            sub.kill()
    os.remove(hostname_file)

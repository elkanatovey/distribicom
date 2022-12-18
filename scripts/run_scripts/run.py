import argparse
import json
import os
import subprocess
import tempfile
import time
from typing import Dict, Any
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
    cnfgs.number_of_elements = int(elements_per_ptxt(
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
    app_cnfgs.configs.CopyFrom(cnfgs)

    return app_cnfgs


def load_test_settings(test_settings_file: str):
    with open(test_settings_file, 'r') as f:
        all = json.load(f)
        return all


def get_all_hostnames(dir_path, hostname_suffix):
    hostname_files = filter(lambda x: hostname_suffix in x, os.listdir(dir_path))
    for hostname_file in hostname_files:
        with open(os.path.join(dir_path, hostname_file), 'r') as f:
            yield f.read().strip()


class Settings:
    def __init__(self, settings_file_name):
        self.sleep_time = 5
        self.hostname_suffix = ".hostname"

        self.all = load_test_settings("test_setting.json")
        self.configs = self.all["configs"]
        self.test_dir = self.all["test_setup"]["shared_folder"]
        self.binaries = self.all["binaries"]

        self.worker_bin = self.binaries["worker"]
        self.server_bin = self.binaries["main_server"]

        self.app_configs_filename = os.path.join(self.test_dir, "app_configs.txt")


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


if __name__ == '__main__':
    args = command_line_args()

    settings = Settings(args.config_file)
    hostname = subprocess.run(['hostname'], stdout=subprocess.PIPE).stdout.decode("utf-8").strip()

    fd, filename = tempfile.mkstemp(dir=settings.test_dir, suffix=settings.hostname_suffix, prefix="distribicom_")
    os.write(fd, hostname.encode("utf-8"))
    os.close(fd)

    print(f"written hostname to {filename}")
    print(f"waiting {settings.sleep_time} seconds for all other running processes to write.")
    time.sleep(settings.sleep_time)

    all_hostnames = sorted(get_all_hostnames(settings.test_dir, settings.hostname_suffix))
    is_main_server = hostname == all_hostnames[0]

    binary = settings.worker_bin
    if is_main_server:
        print("main server creating configs file")
        app_configs = create_app_configs(settings.configs, hostname)
        with open(settings.app_configs_filename, 'wb') as f:
            f.write(app_configs.SerializeToString())
            print(f"configs written to {settings.app_configs_filename}")
        binary = settings.server_bin
    else:
        print("waiting for main server")
        time.sleep(settings.sleep_time)

    out = subprocess.run([binary, settings.app_configs_filename], stdout=subprocess.PIPE).stdout.decode("utf-8").strip()

    printer = "main_server" if is_main_server else f"worker-{hostname}"
    print(f"{printer}: {out}")

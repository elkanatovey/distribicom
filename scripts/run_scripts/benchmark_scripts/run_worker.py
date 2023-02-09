
import argparse
import subprocess
import sys
import tempfile
import os
import json
from typing import Dict, Any, List


def load_json(test_settings_file: str):
    with open(test_settings_file, 'r') as f:
        all = json.load(f)
        return all


def setup_worker(config_info, worker_parms) -> List[subprocess.Popen]:
    return [subprocess.Popen([config_info.worker_bin, worker_parms.pir_configs_file,  worker_parms.num_worker_threads])]


class File_Locations:
    def __init__(self, json_file_name):
        self.all = load_json(json_file_name)

        self.binaries = self.all["binaries"]
        self.worker_bin = self.binaries["worker"]
        self.server_bin = self.binaries["main_server"]

        self.hostname_folder = self.all["hostname_folder"]


class Worker_Params:
    def __init__(self, pir_configs_file, num_worker_threads, server_address_filename):

        self.pir_configs_file = pir_configs_file
        self.num_worker_threads = num_worker_threads

        with open(server_address_filename, "r") as file:
            server_address = file.read()
        self.server_address = server_address + "loch-06:54321"


if __name__ == '__main__': # 1 config file 2 pir settings file 3 num queries 4 num worker_threads
    config_info = File_Locations(sys.argv[1])
    server_address_filename = os.path.join(config_info.hostname_folder, "master_hostname.txt")

    worker_parms = Worker_Params(sys.argv[2], int(sys.argv[3]), server_address_filename)

    subprocesses = setup_worker(config_info, worker_parms)
    try:
        for (i, sub) in enumerate(subprocesses):
            sub.wait()

    except KeyboardInterrupt:
        print("received KeyboardInterrupt, killing subprocess...")
        for sub in subprocesses:
            sub.kill()
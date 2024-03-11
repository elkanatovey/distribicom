
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


def setup_server(config_info, server_parms, hostname) -> List[subprocess.Popen]:
    return [subprocess.Popen([config_info.server_bin, server_parms.pir_configs_file, server_parms.num_queries,
                              server_parms.num_workers, server_parms.num_server_threads, hostname])]


class File_Locations:
    def __init__(self, json_file_name):
        self.all = load_json(json_file_name)

        self.binaries = self.all["binaries"]
        self.worker_bin = self.binaries["worker"]
        self.server_bin = self.binaries["main_server"]

        self.hostname_folder = self.all["hostname_folder"]


class Server_Params:
    def __init__(self, pir_configs_file, num_queries,
                 num_workers, num_server_threads):

        self.pir_configs_file = pir_configs_file
        self.num_queries = num_queries
        self.num_workers = num_workers
        self.num_server_threads = num_server_threads


if __name__ == '__main__':  # 1 config file 2 pir settings file 3 num queries 4 num workers 5 num server threads
    config_info = File_Locations(sys.argv[1])
    server_parms = Server_Params(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])

    hostname = subprocess.run(['hostname'], stdout=subprocess.PIPE).stdout.decode("utf-8").strip() + ":54321"
    hostname_filename = os.path.join(config_info.hostname_folder, "master_hostname.txt")
    print(f"hostname is: {hostname}")

    with open(hostname_filename, "w") as file:
        file.write(hostname)

    print(f"written hostname to {hostname_filename}")

    subprocesses = setup_server(config_info, server_parms, hostname)
    try:
        for (i, sub) in enumerate(subprocesses):
            sub.wait()

    except KeyboardInterrupt:
        print("received KeyboardInterrupt, killing subprocess...")
        for sub in subprocesses:
            sub.kill()
    os.remove(hostname_filename)

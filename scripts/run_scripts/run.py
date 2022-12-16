import json
import subprocess
import time
from sys import stdout
from typing import Dict, Any

import google.protobuf.text_format as tx
import math

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
    app_cnfgs.configs.CopyFrom(create_app_configs(configs))

    return app_cnfgs


def load_test_settings(test_settings_file: str):
    with open(test_settings_file, 'r') as f:
        all = json.load(f)
        return all


if __name__ == '__main__':
    all = load_test_settings("test_setting.json")
    configs = all["configs"]


    hostname = subprocess.run(['hostname'], stdout=subprocess.PIPE).stdout.decode("utf-8").strip()
    print(f'hostname: "{hostname}"')
    time.sleep(5)

    #
    # my_host_name = ""
    #
    # if my_host_name == "":
    #     time.sleep(3)
    #
    # app_cnfgs = create_app_configs(configs, "0.0.0.0")
    # with open('test_setting.pb', 'wb') as f:
    #     f.write(app_cnfgs.SerializeToString())
    #
    # # setting created. but does it make sense? I think not.

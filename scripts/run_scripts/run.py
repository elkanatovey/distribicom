import json
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


def create_protobuf_config(configs: Dict[str, Any]) -> Configs:
    c = Configs()

    c.scheme = configs["scheme"]
    c.polynomial_degree = configs["polynomial_degree"]
    c.logarithm_plaintext_coefficient = configs["logarithm_plaintext_coefficient"]
    c.db_rows = configs["db_rows"]
    c.db_cols = configs["db_cols"]
    c.size_per_element = configs["size_per_element"]
    c.dimensions = 2
    c.number_of_elements = int(elements_per_ptxt(
        c.logarithm_plaintext_coefficient,
        c.polynomial_degree,
        c.size_per_element
    ))
    c.use_symmetric = True
    c.use_batching = True
    c.use_recursive_mod_switching = False
    return c


if __name__ == '__main__':
    with open('test_setting.json', 'r') as f:
        all = json.load(f)
        configs = all["configs"]

        a = AppConfigs()
        a.main_server_hostname = "0.0.0.0:53241"
        a.number_of_workers = configs["number_of_workers"]

        a.query_wait_time = 10  # seconds
        a.configs.CopyFrom(create_protobuf_config(configs))

    with open('test_setting.pb', 'wb') as f:
        f.write(a.SerializeToString())

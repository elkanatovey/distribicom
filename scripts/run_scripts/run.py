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
    configs = {
        "db_rows": 5,
        "db_cols": 5,
        # enc_params:
        "scheme": "bgv",
        "polynomial_degree": 4096,
        "logarithm_plaintext_coefficient": 20,
        # pir params:
        "size_per_element": 256
    }

    a = AppConfigs()
    a.main_server_hostname = "todo:port"
    a.number_of_workers = 1
    a.query_wait_time = 10  # seconds
    a.configs.CopyFrom(create_protobuf_config(configs))

    print(a)
    print()
    print(a.SerializeToString())

    # load config file.

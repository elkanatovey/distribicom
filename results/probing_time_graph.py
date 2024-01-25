from typing import List

import matplotlib.pyplot as plt
import numpy as np

import utils
from results import constants

FOLDER_NAME = '/Users/jonathanweiss/CLionProjects/distribicom/results/evals/probe_graph'

"""
the size of the db translated to the dimensions of the matrix.
"""
db_size_to_db_dims = {
    2 ** 20: '164x164',
    2 ** 19: '116x116',
    2 ** 18: '82x82',
    2 ** 17: '58x58',
    2 ** 16: '42x41',
    2 ** 15: '30x29',
    2 ** 14: '21x21',
    2 ** 13: '15x15',
    2 ** 12: '11x10',
    2 ** 11: '8x7',
    2 ** 10: '6x5',
    2 ** 9: '4x4',
    2 ** 8: '3x3',
    2 ** 7: '2x2',
    2 ** 6: '2x2',
    2 ** 5: '2x2',
    2 ** 4: '2x2',
    2 ** 3: '2x2',
    2 ** 2: '2x2',
    2 ** 1: '2x2',
}

POWERS = [16, 17, 18, 19, 20]


def get_list_of_lower_db_sizes(db_size):
    for key in db_size_to_db_dims.keys():
        if key < db_size:
            yield key


def get_mat_dims(test_res: utils.TestResult):
    splt = test_res.file_name.split('_')[-1]
    splt = splt.split('.')[0]
    return splt


def into_dict_of_dims(rslts: List[utils.TestResult]):
    return {get_mat_dims(rslt): rslt for rslt in rslts}


def prepare_ax(xs):
    fig, ax = plt.subplots()
    fig.tight_layout()
    ax.set_xticks(xs)
    ax.set_xticklabels([f'$2^\u007b {i} \u007d $' for i in POWERS])
    utils.add_y_format(ax)
    ax.set_ylabel('round latency')
    ax.set_xlabel('|messages|')
    return ax


def get_list_of_wanted_dimensions(db_size):
    return [*map(lambda x: db_size_to_db_dims[x], get_list_of_lower_db_sizes(db_size))]


def get_plot(xs: List[int], measurements: dict[str, utils.TestResult]):
    ys, y_errs = [], []
    for db_size in xs:
        cumulative_averages = np.zeros(5)

        dims = get_list_of_wanted_dimensions(db_size)
        for dim in dims:
            if dim not in measurements:
                # we know that we have missing dims for db <= 2^9 luckily we can use times from 2^10 to compensate
                # since 2^10 times are an upperbound for 2^9.
                cumulative_averages += np.ones(5)
            else:
                cumulative_averages += np.array(measurements[dim].data[:5])

        ys.append(np.mean(cumulative_averages))
        y_errs.append(np.std(cumulative_averages))
    return y_errs, ys


def main():
    xs = [2 ** i for i in POWERS]
    ax = prepare_ax(xs)

    y_errs, ys = get_plot(xs, into_dict_of_dims(utils.collect_dpir_test_results(FOLDER_NAME)))
    utils.plot_errbars(ax, xs, ys, y_errs, "probe 252~ clients per server", constants.dpir_clr)

    ax.legend(framealpha=0.3)
    ax.set_ylim(0)
    plt.show()


if __name__ == '__main__':
    main()

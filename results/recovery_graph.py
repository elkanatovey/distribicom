import numpy as np
from matplotlib import pyplot as plt
import utils
import constants


class DbData:
    def __init__(self, num_rows, num_cols):
        """
        :param num_rows: Num rows represents the number of queries per group too,
        :param num_cols: num cols represents the total number of groups.
        So for db of 2^16 according to the paper: numrows= 42, num cols = 42*39
        """
        self.num_rows = num_rows
        self.num_cols = num_cols
        self.name = "lala"
        for i in range(40):

            if num_cols * num_rows < 2 ** (i + 1):
                self.name = "$2^{" + str(i) + "}$"
                break


def db_plot(ax, db: DbData, top_p=9):
    ps = np.arange(0, top_p) / 10

    additional_work = []
    additional_work_err = []

    query_expansion_overheads = []
    query_expansion_overheads_err = []
    for i in range(0, top_p):
        total_queries_per_worker, queries_expansion_overhead = calc_additional_work(db, ps[i])
        additional_work.append(np.average(total_queries_per_worker))
        additional_work_err.append(np.std(total_queries_per_worker))

        query_expansion_overheads.append(np.average(queries_expansion_overhead))
        query_expansion_overheads_err.append(np.std(queries_expansion_overhead))

    prms = {
        'fmt': '-o',
        'barsabove': True,
        'capsize': constants.marker_size,
        'ecolor': constants.dpir_clr,
        'color': constants.dpir_clr,

        'linewidth': constants.line_size,
        'markersize': constants.marker_size,
    }
    ax.errorbar(
        ps, additional_work,
        yerr=additional_work_err,
        label="number of queries",
        **prms
    )

    prms['color'] = constants.addra_clr
    prms['ecolor'] = constants.addra_clr
    ax.errorbar(
        ps, query_expansion_overheads,
        yerr=query_expansion_overheads_err,
        label="number of expansions",
        **prms
    )

    # ax.errorbar(ps, additional_work, label=db.name, yerr=additional_work_err, fmt='-o')


def calc_additional_work(db: DbData, p_fault):
    queries_overhead = []
    query_expansion_overhead = []
    for j in range(0, 10):
        # Generate a random coin toss matrix
        coin_toss_matrix = np.random.choice(
            [1, 0],
            size=(db.num_rows, db.num_cols),
            p=[1 - p_fault, p_fault]
        )

        num_oks_in_group = coin_toss_matrix.sum(axis=0)
        num_queries_per_worker_without_redistirbution = np.ones(
            shape=(db.num_rows, db.num_cols)) * num_oks_in_group.reshape(1, db.num_cols)

        num_workers_in_row = coin_toss_matrix.sum(axis=1).reshape(db.num_rows, 1)

        redistributed_on_row = num_queries_per_worker_without_redistirbution.sum(axis=1) / num_workers_in_row

        redistributed_on_row = np.ceil(redistributed_on_row)
        queries_overhead.append(np.max(redistributed_on_row) / db.num_rows)

        ####
        row_sums = np.abs(np.sum(1 - coin_toss_matrix, axis=1))
        failed = max(row_sums)
        query_expansion_overhead.append(1 + (failed / (db.num_cols - failed)))

    return queries_overhead, query_expansion_overhead


# TODO:
#  3. Compute expansion costs.
if __name__ == '__main__':
    fig, ax = plt.subplots()

    dbs = [
        # DbData(260, 39 * 260),
        DbData(164, 39 * 164),
        # DbData(82, 39 * 82),
        # DbData(42, 39 * 42),
    ]
    clrs = [
        constants.dpir_clr,
        # constants.addra_clr,
        # constants.sealpir_clr
    ]

    ln_size = constants.line_size

    constants.change_sizes(2)
    for db in dbs:
        db_plot(ax, db)
        # if db.name == "$2^{20}$":
        #     for p in np.arange(0, 6) / 10:
        #         print(f"number of queries in the busiest worker (p={p}): {get_aditional_work(db, 164, p)}")

    constants.change_sizes(ln_size)

    # y-axis = percentage of additional work per worker.
    # x-axis = failure probability of a worker.
    ax.legend()
    ax.set_xlabel("worker failure probability")
    ax.set_ylabel("Maximal relative overhead\n (On busiest worker)")
    plt.show()

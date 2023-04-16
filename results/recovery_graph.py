import numpy as np
from matplotlib import pyplot as plt, ticker
import utils
import constants
from mpl_axes_aligner import align


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


def db_plot(ax, db: DbData, top_p=10):
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
        label="Additional queries",
        **prms
    )

    prms['color'] = constants.addra_clr
    prms['ecolor'] = constants.addra_clr
    ax.errorbar(
        ps, query_expansion_overheads,
        yerr=query_expansion_overheads_err,
        label="Additional expansions",
        **prms
    )

    ax2 = ax.twinx()

    def relative_tick_formatter(x, pos):
        """Custom tick formatter to display relative tick values"""

        return f'{int((x / db.num_rows) * 100)}%'  # Convert tick value to percentage and format as string

    ax2.yaxis.set_major_formatter(ticker.FuncFormatter(relative_tick_formatter))

    ax2.set_yticks(np.arange(0, 5) * 41)
    org1 = 0.0  # Origin of first axis
    org2 = 0.0  # Origin of second axis
    pos = 0.1  # Position the two origins are aligned
    align.yaxes(ax, org1, ax2, org2, pos)
    ax2.set_ylabel("Relative Overhead on busiest worker")


def calc_additional_work(db: DbData, p_fault):
    queries_overhead = []
    query_expansion_overhead = []
    for j in range(0, 10):
        coin_toss_matrix = np.random.choice(
            [1, 0],
            size=(db.num_rows, db.num_cols),
            p=[1 - p_fault, p_fault]
        )
        faults_mask = np.abs(1 - coin_toss_matrix)

        num_qs_per_group = np.ones(shape=(db.num_rows, db.num_cols)
                                   ) * coin_toss_matrix.sum(axis=0).reshape(1, db.num_cols)
        num_workers_in_row = coin_toss_matrix.sum(axis=1)

        # First Line
        # take fault workers.  Calc how many queries they add to each row.
        # divide these queries equally over all valid workers (PER ROW).
        # take the busiest worker that is still alive and add this division to it.
        # get maximal worker.
        qs_of_faulty_per_row = (num_qs_per_group * faults_mask).sum(axis=1)
        qs_of_faulty_divided_equally = np.ceil(qs_of_faulty_per_row / num_workers_in_row)

        busiest_worker_post_redistribution = (num_qs_per_group * coin_toss_matrix).max(
            axis=1) + qs_of_faulty_divided_equally

        # What is the new load?
        # for each group compute the fallen. Take maximal fallen in each row, add it to maximal non fallen in group.

        # rmv num_rows to count only overhead.
        queries_overhead.append(np.max(busiest_worker_post_redistribution) - db.num_rows)

        ####
        # Count per row the total new queries that need expansion.
        # count the missing * their queries, these guys are the new queries in the row.
        queries_that_need_expansion = num_qs_per_group * faults_mask

        additional_queries_per_row = queries_that_need_expansion.sum(axis=1).reshape(1, db.num_rows)
        redistributed_additional_queries_per_row = additional_queries_per_row / num_workers_in_row.reshape(1,
                                                                                                           db.num_rows)
        query_expansion_overhead.append(np.max(np.ceil(redistributed_additional_queries_per_row)))

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
    ax.set_ylabel("Overhead in busiest worker")

    plt.show()

import numpy as np
from matplotlib import pyplot as plt
import utils
import constants


class DbData:
    def __init__(self, num_rows, num_cols):
        self.num_rows = num_rows
        self.num_cols = num_cols
        self.name = "lala"
        for i in range(40):

            if num_cols * num_rows < 2 ** (i + 1):
                self.name = "$2^{" + str(i) + "}$"
                break


def db_plot(ax, db: DbData, clr, top_p=6):
    ps = np.arange(0, top_p) / 10

    additional_work = []
    additional_work_err = []
    for i in range(0, top_p):
        avg, std = calc_additional_work(db, ps[i])

        additional_work.append(avg)
        additional_work_err.append(std)

    utils.plot_errbars(ax, ps, additional_work, additional_work_err, db.name, clr)
    # ax.errorbar(ps, additional_work, label=db.name, yerr=additional_work_err, fmt='-o')


def calc_additional_work(db: DbData, prob_heads):
    tmps = []
    max_fails_in_row = []
    for j in range(0, 5):
        # Generate a random coin toss matrix
        coin_toss_matrix = np.random.choice(
            [0, 1],
            size=(db.num_rows, db.num_cols),
            p=[1 - prob_heads, prob_heads]
        )

        row_sums = np.sum(coin_toss_matrix, axis=1)
        failed = max(row_sums)
        max_fails_in_row.append(failed)
        tmps.append((failed / (db.num_cols - failed)) * 100)

    avg, std = np.average(tmps), np.std(tmps)
    return avg, std


def get_aditional_work(db, num_queries, p):
    avg, _ = calc_additional_work(db, p)

    return int(np.ceil(avg / 100 * db.num_rows + num_queries))


# TODO:
#  3. Compute expansion costs.
if __name__ == '__main__':
    fig, ax = plt.subplots()

    dbs = [
        # DbData(260, 39 * 260),
        DbData(164, 39 * 164),
        # DbData(82, 39 * 82),
        DbData(42, 39 * 42),
    ]
    clrs = [
        constants.dpir_clr,
        constants.addra_clr,
    ]

    ln_size = constants.line_size

    constants.change_sizes(2)
    for clr, db in zip(clrs, dbs):
        db_plot(ax, db, clr)
        if db.name == "$2^{20}$":
            for p in np.arange(0, 6) / 10:
                print(f"number of queries in the busiest worker (p={p}): {get_aditional_work(db, 164, p)}")

    constants.change_sizes(ln_size)

    # y-axis = percentage of additional work per worker.
    # x-axis = failure probability of a worker.
    ax.legend()
    ax.set_xlabel("worker failure probability")
    ax.set_ylabel("Maximal relative overhead \n(On busiest worker)")
    plt.show()

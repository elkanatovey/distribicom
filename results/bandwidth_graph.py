import matplotlib.pyplot as plt
import numpy as np

kb = 1024
mb = kb * kb

ptx_size = 10 * kb
ctx_size = 50 * kb


def to_kb(n):
    return n // kb


def to_mb(n):
    return n // (mb)


class DPirBandwidth:
    def __init__(self, db_rows, db_cols, number_of_workers):
        self.rows = db_rows
        self.cols = db_cols
        self.number_of_workers = number_of_workers

        # need to compute the number of work per client.
        self.num_groups = max(1, number_of_workers // db_rows)
        self.num_workers_in_group = number_of_workers // self.num_groups

        # // num groups is the amount of duplication of the DB.
        self.num_queries_per_group = number_of_workers // self.num_groups  # // assume num_queries>=num_groups

        self.num_rows_per_worker = db_rows // self.num_workers_in_group
        if self.num_rows_per_worker == 0:
            raise Exception("0 rows per worker")

    def compute_db_sending(self):
        return self.cols * self.num_rows_per_worker * self.number_of_workers * ptx_size

    def compute_sending_queries(self):
        return self.num_workers_in_group * self.num_queries_per_group * self.num_groups * ctx_size

    def compute_worker_response_size(self):
        return self.num_queries_per_group * self.num_rows_per_worker * ctx_size

    def compute_total_workers_response_size(self):
        return self.compute_worker_response_size() * self.number_of_workers

    def client_receive_query_response_size(self):
        # TODO: number of ctxs in response
        raise Exception("Not implemented")


class SealPirBandwidth:
    def __init__(self, db_rows, db_cols, number_of_clients):
        self.rows = db_rows
        self.cols = db_cols
        self.number_of_clients = number_of_clients


def DPirBandwidth_graph(rows, cols):
    xs = [*range(rows, rows * 10, rows)]
    db_size = map(lambda i: DPirBandwidth(rows, cols, i).compute_db_sending(), xs)
    workers_response_size = [*map(lambda i: DPirBandwidth(42, 41, i).compute_total_workers_response_size(), xs)]
    ys = np.array([*map(to_mb, workers_response_size)]) + np.array([*map(to_mb, db_size)])

    return xs, ys


if __name__ == '__main__':
    print(to_mb(DPirBandwidth(164,164,164).compute_worker_response_size()))
    # xs, ys = DPirBandwidth_graph(42, 41)
    # print(xs,ys)
    # need to add to the ys the total client sendings too.

    # plt.plot(xs, ys)
    # plt.show()

# print(server_bandwidth_per_client)

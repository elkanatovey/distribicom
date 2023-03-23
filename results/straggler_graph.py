import numpy as np


def generate_guassian_points(mean, std, num_points):
    return np.random.normal(mean, std, num_points).clip(0)


def add_waiting_points(num_points, mean, arr):
    np.append(arr, [mean]*num_points)


def calculate_sd(array):
    return np.std(array)


if __name__ == '__main__':
    num_workers = 1000
    mean = 2
    std = 2
    num_sds_to_wait = 5

    for dishonest_workers in range(0, 10):
        percentage_dishonest = 0.1*dishonest_workers

        num_honest = num_workers*( 1 - percentage_dishonest)
        curr_points = generate_guassian_points(mean, std, int(num_honest))

        num_dishonest = num_workers - num_honest

        temp_curr = np.copy(curr_points)
        add_waiting_points(int(num_workers-num_honest), mean, temp_curr)
        new_std = calculate_sd(temp_curr)
        print(new_std)






    points = generate_guassian_points(1, 2, 5)

    print(points.shape)


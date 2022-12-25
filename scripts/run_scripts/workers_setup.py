from run import *

if __name__ == '__main__':
    args = command_line_args()

    settings = Settings(args.config_file)

    workers = []
    for i in range(settings.configs["number_of_workers"]):
        curr_worker = subprocess.Popen([settings.worker_bin, settings.app_configs_filename])
        workers.append(curr_worker)

    for worker in workers:
        worker.wait()

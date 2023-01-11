from run import *

"""
    Used for debugging on localhost.
"""
if __name__ == '__main__':
    args = command_line_args()

    settings = Settings(args.config_file, args.servers, args.cpus)

    hostname = subprocess.run(['hostname'], stdout=subprocess.PIPE).stdout.decode("utf-8").strip()

    app_configs = create_app_configs(settings.configs, hostname)
    with open(settings.app_configs_filename, 'wb') as f:
        f.write(app_configs.SerializeToString())
        print(f"configs written to {settings.app_configs_filename}")

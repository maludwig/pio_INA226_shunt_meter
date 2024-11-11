import datetime
import os
from os.path import join, dirname, basename, splitext

__file__ = r'/Users/mitchellludwig/dev/esp_projects/pio_INA226_shunt_meter/version_gen.py'
PROJECT_DIR = dirname(__file__)
SRC_DIR = join(PROJECT_DIR, "src")
VERSION_FILE_PATH = join(SRC_DIR, "shunt_version.h")


# Import the current working construction
# environment to the `env` variable.
# alias of `env = DefaultEnvironment()`
# Import("env")

# Dump construction environment (for debug purpose)
# print(env.Dump())

def before_build(source, target, env):
    # Generate a version based on the current timestamp
    datestamp = datetime.datetime.now().strftime("%Y-%m-%d");
    timestamp = datetime.datetime.now().strftime("%H:%M:%S");
    version = f"{datestamp} {timestamp}"

    # Generate a header file with this version
    with open(VERSION_FILE_PATH, "w") as f:
        f.write("#pragma once\n")
        f.write(f"#define SHUNT_VERSION \"{version}\"\n")

# before_build("a","b","c")
# env.AddPreAction("buildprog", before_build)

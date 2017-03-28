#!/usr/bin/env python

import sys
import os
import glob
import re
import shutil
import errno

is_osx = (sys.platform == "darwin")

compiler = "clang"

preload_flag = "LD_PRELOAD="
if is_osx:
  preload_flag = "DYLD_FORCE_FLAT_NAMESPACE=y DYLD_INSERT_LIBRARIES="

cwd = os.path.realpath(os.getcwd());
script_dir = os.path.dirname(os.path.realpath(__file__))

store_source_path = script_dir + "/storeenv.c"
chroot_source_path = script_dir + "/chroot.c"

cmdline = (" ".join(sys.argv[1:]))

log_path = cwd + "/log"
chroot_so_path = cwd + "/chroot.so"
store_so_path = cwd + "/storeenv.so"
output_dir = cwd + "/standalone/"
run_script = output_dir + "run.sh"
creduce_runner = "test.sh"
creduce_runner_template = creduce_runner + ".template"
creduce_starter = output_dir + "creduce.sh"

def ensureDirExists(filename):
    if not os.path.exists(os.path.dirname(filename)):
        try:
            os.makedirs(os.path.dirname(filename))
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                print(exc)

def print_and_run(command):
    print("exec: " + command)
    return os.system(command)

def compile_shared_libs():
    if print_and_run(compiler + " -fPIC -shared " + store_source_path + " -o " + store_so_path + " -ldl") != 0:
        print("error while compiling. Aborting...")
        exit(1)
    if print_and_run(compiler + " -fPIC -shared " + chroot_source_path + " -o " + chroot_so_path + " -ldl") != 0:
        print("error while compiling. Aborting...")
        exit(1)

def create_reproduce_script():
    shutil.copyfile(chroot_so_path, output_dir + "/chroot.so")
    
    with open(run_script, "w") as f:
        f.write("#!/bin/bash\n")
        f.write("set +e\n")
        f.write("""DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"\n""")
        f.write('CHRSO="$DIR/chroot.so"\n')
        f.write('cd "$DIR"\n')
        f.write('cd "' + cwd[1:] + '"\n')
        f.write("GP_CHROOT_PATH=\"$DIR/\" " + preload_flag + "\"$CHRSO\" " + cmdline + "\n")
        f.write('exit 0\n')
    

def run_user_command():
    print_and_run("GP_CWD=" + cwd + " GP_STORAGE=" + log_path + " " + preload_flag + store_so_path + " " + (" ".join(sys.argv[1:])))

class SystemCall:
    callType = ""
    arg = ""

def parse_log():
    result = []

    with open(log_path, "r") as ins:
        for line in ins:
            sysCall = SystemCall()
            sysCall.callType = line.split(' ', 1)[0]
            sysCall.arg = line.split(' ', 1)[1].strip()
            result.append(sysCall)
    return result

def handle_log():

    print("Parsing log")
    events = parse_log()

    print("Copying tree")
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    for e in events:
        if e.arg.startswith("/dev/"):
            continue
        if e.arg.startswith("/sys/"):
            continue

        #print("saving:" + e.arg)
        newPath = output_dir + e.arg

        if os.path.isdir(e.arg):
            ensureDirExists(newPath + "/.")
        else:
            ensureDirExists(newPath)
            try:
                shutil.copyfile(e.arg, newPath)
                shutil.copystat(e.arg, newPath)
            except OSError as exc:
                pass #print(exc)
            except IOError as exc:
                pass #print(exc)

try:
    os.remove(log_path)
except:
    pass

compile_shared_libs()

run_user_command()

handle_log()

create_reproduce_script()

print("DONE!")

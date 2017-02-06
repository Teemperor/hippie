#!/usr/bin/env python

import sys
import os
import glob
import re
import shutil
import errno

cwd = os.path.realpath(os.getcwd());
script_dir = os.path.dirname(os.path.realpath(__file__))

store_source_path = script_dir + "/storeenv.c"
chroot_source_path = script_dir + "/chroot.c"

log_path = cwd + "/log"
cmdline_path = cwd + "/cmdline"
chroot_so_path = cwd + "/storeenv.so"
store_so_path = cwd + "/chroot.so"
output_dir = cwd + "/standalone/"

def ensureDirExists(filename):
    if not os.path.exists(os.path.dirname(filename)):
        try:
            os.makedirs(os.path.dirname(filename))
        except OSError as exc:
            if exc.errno != errno.EEXIST:
                print(exc)

def print_and_run(command):
    print("exec: " + command)
    os.system(command)
    
def compile_shared_libs():
    print_and_run("gcc -fPIC -shared " + store_source_path + " -o " + store_so_path + " -ldl")
    print_and_run("gcc -fPIC -shared " + chroot_source_path + " -o " + chroot_so_path + " -ldl")
    

def run_user_command():
    print_and_run("GP_STORAGE=" + log_path + " LD_PRELOAD=" + store_so_path + " " + (" ".join(sys.argv[1:])))

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
    
    events = parse_log()
    
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
                shutil.copy(e.arg, newPath)
            except OSError as exc:
                pass #print(exc)

try:
    os.remove(log_path)
except:
    pass

try:
    os.remove(cmdline_path)
except:
    pass

compile_shared_libs()

run_user_command()

handle_log()

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
cwdfile_path = cwd + "/cwd"
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
    if print_and_run("gcc -fPIC -shared " + store_source_path + " -o " + store_so_path + " -ldl") != 0:
        print("error while compiling. Aborting...")
        exit(1)
    if print_and_run("gcc -fPIC -shared " + chroot_source_path + " -o " + chroot_so_path + " -ldl") != 0:
        print("error while compiling. Aborting...")
        exit(1)

def create_reproduce_script():
    f = open(cmdline_path)
    cmdline_line = f.readlines()[0].strip()
    f.close()
    executable = cmdline_line.split(" ")[0]
    shutil.copyfile(executable, output_dir + executable)
    shutil.copystat(executable, output_dir + executable)
    
    shutil.copyfile(chroot_so_path, output_dir + "/chroot.so")
    
    f = open(cwdfile_path)
    chroot_cwd = f.readlines()[0].strip()
    f.close()
    
    with open(run_script, "w") as f:
        f.write("#!/bin/bash\n")
        f.write("set +e\n")
        f.write("""DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"\n""")
        f.write('CHRSO=$(realpath chroot.so)\n')
        f.write('cd "$DIR"\n')
        f.write('cd "' + chroot_cwd[1:] + '"\n')
        f.write("GP_CHROOT_PATH=\"$DIR/\" LD_PRELOAD=\"$CHRSO\" " + cmdline_line + "\n")
        f.write('exit 0\n')

def create_creduce_runner_template():
    f = open(cmdline_path)
    cmdline_line = f.readlines()[0].strip()
    f.close()
    f = open(cwdfile_path)
    chroot_cwd = f.readlines()[0].strip()
    f.close()
    
    with open(output_dir + creduce_runner_template, "w") as f:
        f.write('if [ -f main.cpp ]; then\n  exit 1\nfi\n')
        f.write('rsync  --exclude "*.orig" --exclude "$BASEDIR/creduce_bug_*" --ignore-existing -aAXrq "$BASEDIR"/* . >/dev/null 2>/dev/null\n')
        f.write("bash run.sh 2>err 1>out\n")
        f.write("grep -q goodstring err\n")
        
def create_creduce_starter():
    with open(creduce_starter, "w") as f:
        f.write("#!/bin/bash\n")
        f.write("""DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"\n""")
        f.write('cd "$DIR"\n')
        f.write('echo "#!/bin/bash" > ' + creduce_runner + "\n")
        f.write('echo "BASEDIR=$DIR" >> ' + creduce_runner + "\n")
        f.write('cat ' + creduce_runner_template + ' >> ' + creduce_runner + "\n")
        f.write('chmod +x ' + creduce_runner + '\n')
        #f.write("find . -type f | xargs -L1 file -F ' ' | grep ' text' | grep -vF 'test.sh' | grep -vF 'run.sh' | grep -vF 'creduce.sh' | awk '{print $1}' | xargs creduce --debug ./test.sh\n")
        f.write("creduce --debug ./test.sh ./\n")
    

def run_user_command():
    print_and_run("GP_CWD=" + cwdfile_path + " GP_CMDLINE=" + cmdline_path + " GP_STORAGE=" + log_path + " LD_PRELOAD=" + store_so_path + " " + (" ".join(sys.argv[1:])))

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

try:
    os.remove(cmdline_path)
except:
    pass

compile_shared_libs()

run_user_command()

handle_log()

create_reproduce_script()
create_creduce_runner_template()
create_creduce_starter()

print("DONE!")

#!/usr/bin/env python

import sys
import os
import glob
import re
import shutil
import errno

#from subprocess import call

#print 'Number of arguments:', len(sys.argv), 'arguments.'
#print 'Argument List:', str(sys.argv)

strace_log_prefix = ".senv_log"
line_regex = re.compile("^(\\S+)\\(\"(\\S+)\"([\\S\\s]+) = \d+")
clang_call_regex = re.compile("[\S]*")
clang_arg_call = re.compile(", \\[\"([\\S\\s]+)\"\\], \\[")

output_dir = "standalone"

def ensureDirExists(filename):
    if not os.path.exists(os.path.dirname(filename)):
        try:
            os.makedirs(os.path.dirname(filename))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

def clean_strace_files():
    filelist = glob.glob(strace_log_prefix + ".*")
    for f in filelist:
        os.remove(f)

def run_user_command():
#    os.system("strace -s 16000 -e trace=execve,open,chdir,stat -ffo " + strace_log_prefix  + " " + sys.argv[1])
    os.system("strace -s 16000 -ffo " + strace_log_prefix  + " " + sys.argv[1])

def analyze_all_strace_logs():
    filelist = glob.glob(strace_log_prefix + ".*")
    for f in filelist:
        if handle_strace_log(f):
            return
    print("Error: Could not find a call to clang!")


def handle_strace_log(logfile):
    
    events = parse_strace_log(logfile)
    foundClangStart = False
    for e in events:
        if e.evType == StraceEvent.EXECVE:
            #if clang_call_regex.match(e.path):
                if "\"-cc1\"" in e.line:
                    foundClangStart = True
    if not foundClangStart:
        return False
    
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    
    for e in events:
        if e.evType == StraceEvent.OPEN or e.evType == StraceEvent.STAT:
            if e.path.startswith("/dev/"):
                continue
            if e.path.startswith("/sys/"):
                continue
            
            print("saving:" + e.path)
            newPath = output_dir + e.path
            if os.path.isdir(e.path):
                ensureDirExists(newPath + "/.")
            else:
                ensureDirExists(newPath)
                try:
                    shutil.copy(e.path, newPath)
                except:
                    print("error")
        else:
            print("ARGS:")
            call_args = clang_arg_call.search(e.line).group(1)
            call_args = call_args.replace("\", \"", " ")
            call_args = call_args.replace("-internal-isystem /", "-internal-isystem ./")
            call_args = call_args.replace("-I /", "-I ./")
            call_args = call_args.replace("-internal-externc-isystem /", "-internal-externc-isystem ./")
            call_args = call_args.replace("-resource-dir /", "-resource-dir ./")
            call_args = call_args.replace("-coverage-notes-file /", "-coverage-notes-file ./")
            
            print(call_args)
    return True

class StraceEvent:
    OPEN = 1
    STAT = 2
    EXECVE = 3
    
    evType = 0
    path = ""
    line = ""
                

def parse_strace_log(logfile):
    events = []
    with open(logfile) as f:
        for line in f:
            line = line.strip()
            match = line_regex.search(line)
            if match is not None:
                if match.group(1) == "open":
                    #if match.group(2).endswith(".h") or match.group(2).endswith(".hpp") \
                    #    or match.group(2).endswith(".cc")  or match.group(2).endswith(".cpp"):
                        e = StraceEvent()
                        e.evType = StraceEvent.OPEN
                        e.path = match.group(2)
                        e.line = line
                        events.append(e)
                elif match.group(1) == "stat":
                    #if match.group(2).endswith(".h") or match.group(2).endswith(".hpp") \
                    #    or match.group(2).endswith(".cc")  or match.group(2).endswith(".cpp"):
                        e = StraceEvent()
                        e.evType = StraceEvent.STAT
                        e.path = match.group(2)
                        e.line = line
                        events.append(e)
                elif match.group(1) == "execve":
                    e = StraceEvent()
                    e.evType = StraceEvent.EXECVE
                    e.line = line
                    e.path = os.path.realpath(match.group(2))
                    events.append(e)
    return events
                    

clean_strace_files()

print("Running command...")

run_user_command()

print("Analyzing strace...")

analyze_all_strace_logs()

#clean_strace_files()

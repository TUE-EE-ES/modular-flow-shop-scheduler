#!/usr/bin/env python
import os, sys, time
import glob
import subprocess
import inspect

hcssBinary = "build/HCSS"
import platform
is_windows = any(platform.win32_ver())
if is_windows:
    hcssBinary = hcssBinary + '.exe'

def convert(input, output):
    start = time.time()
    
    if not os.path.isdir(os.path.dirname(output)):
        os.makedirs(os.path.dirname(output))
    
    if not os.path.isfile(hcssBinary) :
        raise Exception("Cannot find HCSS scheduler!")
    print("-- Converting " + input)
        
    sys.stdout.flush()
    sys.stderr.flush()
    if subprocess.call([hcssBinary, '-a', 'cplex', '-i', input, '-o', output], stdout=sys.stdout, ) != 0:
        raise Exception("Scheduler crashed... check previous output!")
        
    print("Converting took " + str(time.time() - start) + " seconds")

def main(args): 
    if len(args) not in [3]:
        raise Exception("Incompatible number of arguments; expected 2 arguments: input dir, output dir")

    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    output_directory = os.path.normpath(args[2])
    
    experiment(input_directory, output_directory)

def experiment(input_directory, output_directory):
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    # TODO: before each optim, do "change delete mipstart *" and read mipstart .mst (with heuristic result)
    # TODO: save binaries for optimization in later 'horizons'
    for f in glob.glob(os.path.join(input_directory, '*.xml')):
        cplex_log = format(time.time()) + ".log"
        f = os.path.normpath(f)
        out_dir = os.path.split(os.path.dirname(f))[1]
        lp_file = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp')
        cplex_log = lp_file + ".log"
        output = os.path.join(output_directory, lp_file)
        
        convert(f, output)
        for line in open(f):
            if 'jobs count="' in line:
                jobcount = int(line.split('="')[1].split('"')[0])
                break
        cplex_command_file = os.path.join(output_directory, os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp.cmd'))
        
        sol_file = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.sol')
        with open(cplex_command_file, 'w') as cplex_file:
                
                cplex_file.write(
"""set logfile {0}
""".format(cplex_log.replace('\\','/')))
                cplex_file.write(
"""xecute mkdir -p {0}
set timelimit {1}
read {2}
optim
write {3}
""".format(out_dir.replace('\\','/'), 2*jobcount, lp_file.replace('\\','/'), sol_file.replace('\\','/')))

if __name__ == '__main__':
    main(sys.argv)

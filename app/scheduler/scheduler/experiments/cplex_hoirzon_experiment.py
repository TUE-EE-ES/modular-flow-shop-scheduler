#!/usr/bin/env python
import os, sys, time
import glob
import subprocess
import inspect

import xml.etree.ElementTree as ET

CPLEX_COMMAND = "~/cplex/127/cplex/bin/x86-64_linux/cplex"

hcssBinary = "build/HCSS"
import platform
is_windows = any(platform.win32_ver())
if is_windows:
    hcssBinary = hcssBinary + '.exe'

def convert(input, output, max_horizon):
    start = time.time()
    
    if not os.path.isdir(os.path.dirname(output)):
        os.makedirs(os.path.dirname(output))
    
    if not os.path.isfile(hcssBinary) :
        raise Exception("Cannot find HCSS scheduler!")
    print("-- Converting " + input)
        
    sys.stdout.flush()
    sys.stderr.flush()
    if subprocess.call([hcssBinary, '-a', 'horizon', '-i', input, '-o', output, '-k', max_horizon], stdout=sys.stdout, ) != 0:
        raise Exception("Scheduler crashed... check previous output!")
        
    print("Converting took " + str(time.time() - start) + " seconds")

def main(args): 
    if len(args) != 4:
        raise Exception("Incompatible number of arguments; expected 3 arguments: input dir, output dir, max_horizon")

    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    output_directory = os.path.normpath(args[2])
    max_horizon = args[3]
    
    experiment(input_directory, output_directory, max_horizon)

def experiment(input_directory, output_directory, max_horizon):
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    # TODO: before each optim, do "change delete mipstart *" and read mipstart .mst (with heuristic result)
    # TODO: save binaries for optimization in later 'horizons'
    for f in glob.glob(os.path.join(input_directory, '*.xml')):
        cplex_log = format(time.time()) + ".log"
        f = os.path.normpath(f)

        out_dir = os.path.split(os.path.dirname(f))[1]
        lp_file = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp')
        output = os.path.join(output_directory, lp_file)
        convert(f, output, max_horizon)

        # find out how many jobs/pages there are
        for line in open(f):
            if 'jobs count="' in line:
                jobcount = int(line.split('="')[1].split('"')[0])
                break
        fixed_operation_times = {}
        i = 0
        while os.path.isfile(output + '.' + str(i)):
            
            cplex_log = lp_file + ".log"
            
            cplex_command_file = os.path.join(output_directory, os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp.cmd'))
            
            sol_file = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.sol' + '.' + str(i))
            
            # create a command file to send to CPLEX
            with open(cplex_command_file, 'w') as cplex_file:
                    
                cplex_file.write(
"""set logfile {0}
""".format(cplex_log.replace('\\','/')))
                cplex_file.write(
"""xecute mkdir -p {0}
set timelimit {1}
set emphasis mip 1
read {2} lp
optim
write {3} sol
y
""".format(out_dir.replace('\\','/'), 20, lp_file.replace('\\','/'), sol_file.replace('\\','/')))
            
            # write an LP file, based on the current job, and the previously calculated starting times
            with open(output + "." + str(i)) as in_file, open(output, 'w') as out_file:
                for line in in_file:
                    out_file.write(line)
                    if "subject to" in line.lower():
                        # fix the starting times up to this job:
                        
                        for job in fixed_operation_times:
                            for operation in fixed_operation_times[job]:
                                out_file.write("o_{}_{} = {}\n".format(job, operation, fixed_operation_times[job][operation]))
            
            
            # run CPLEX on this instance (at most x seconds)
            print "cd {}".format(output_directory) + ";" + CPLEX_COMMAND + " < " + os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp.cmd')
            if subprocess.call(["cd {}".format(output_directory) + ";" + CPLEX_COMMAND + " < " + os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + '.lp.cmd')], stdout=sys.stdout, shell=True) != 0:
                raise "Problem with CPLEX..."
            
            # extract the starting times for the current job/page
            tree = ET.parse(os.path.join(output_directory, sol_file))
            root = tree.getroot()

            fixed_operation_times[i] = {}
            
            for var in root.find('variables').iter('variable'):
                name = var.attrib['name']
                if len(name.split('_')) == 3:
                    if name.startswith('o_{}_'.format(i)):
                        
                        fixed_operation_times[i][name.split('_')[2]] = var.attrib['value']
            
            i += 1

if __name__ == '__main__':
    main(sys.argv)

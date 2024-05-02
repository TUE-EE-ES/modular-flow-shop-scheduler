#!/usr/bin/env python
import os, sys, time
import glob
import subprocess
import inspect
from verify import verify

hcssBinary = "build/HCSS"
import platform
is_windows = any(platform.win32_ver())
if is_windows:
    hcssBinary = hcssBinary + '.exe'

DRY_RUN = False #True # disables scheduling execution and verification when True, but needs the output files to be present

def runScheduler(input, output):
    start = time.time()

    if not os.path.isdir(os.path.dirname(output)):
        os.makedirs(os.path.dirname(output))

    if not os.path.isfile(hcssBinary) :
        raise Exception("Cannot find HCSS scheduler!")
    print("-- Scheduling " + input)
    
    
    if DRY_RUN == False:
        #remove possibly generated previous output!
        for o in glob.glob(output + ".*"):
            if os.path.isfile(o):
                os.unlink(o)
        sys.stdout.flush()
        sys.stderr.flush()    
        if subprocess.call([hcssBinary, '-a', 'branchbound', '-i', input, '-o', output, '-vv', '-l'], stdout=sys.stdout, ) != 0:
            raise Exception("Scheduler crashed... check previous output!")

        invalid_schedule = []
        if not verify(input, output):
            invalid_schedule.append(output)
        if len(invalid_schedule) != 0:
            print("invalid schedule(s) generated:")
            for i in invalid_schedule:
                print(i)
            raise Exception("Scheduler produced incorrect results; check the verification output")

    print("Scheduling took " + str(time.time() - start) + " seconds")

def main(args):
    if(len(args)) != 3:
        print "Expecting <input> <output>"
        raise Exception("Incompatible number of arguments; expected exactly 2 arguments")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    output_directory = os.path.normpath(args[2])

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    res = {}
    for f in glob.glob(input_directory + '/*.xml'):
        f = os.path.normpath(f)

        output = os.path.join(output_directory, os.path.splitext(os.path.basename(f))[0] + '.out')
        start = time.time()
        runScheduler(f, output)
        duration = time.time() - start
        makespans = []
        with open(output) as output_file:
            makespans.append(float(output_file.read().strip('\n').split('\n')[-1].split()[-1])/1e6) # use the last entry as the resulting makespan values
        with open(output + '.lb') as lb_file:
            lowerbound = int(lb_file.read())/1e6
        res[output] = min(makespans), duration, lowerbound
            
    # store a summary of the output for download in CI
    with open(os.path.join(output_directory, 'summary.txt'), 'w') as summary:
        summary.write('Benchmark\tLowerbound\tBest found\n')
        for output in res:
            summary.write('{0}\t{1}\t{2}\n'.format(os.path.splitext(os.path.basename(output))[0], res[output][2], res[output][0]))

if __name__ == '__main__':
    main(sys.argv)

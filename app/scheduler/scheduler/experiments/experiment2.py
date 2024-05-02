#!/usr/bin/env python
import os, sys, time
import glob
import subprocess
import matplotlib as mpl # to run plot without x-server backend
mpl.use('Agg') # must be set before any other import of pylab/matplotlib/pyplot/pandas
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import pandas 
import numpy as np
import inspect
from verify import verify

import logging

import functools
import signal

log = logging.getLogger(__name__)
#log.setLevel(logging.INFO)

hcssBinary = "build/HCSS"
import platform
is_windows = any(platform.win32_ver())
if is_windows:
    hcssBinary = hcssBinary + '.exe'

from multiprocessing import Pool, cpu_count

DRY_RUN = False #True # disables scheduling execution and verification when True, but needs the output files to be present
def plot_box_plot(pdf, title, out, group, columns):

    ax = out[[group] + columns].boxplot(by=group)
    fig = ax.get_figure()
    suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_inches='tight')
    plt.close()

def plot_bar_chart(pdf, title, out, x_label, columns):
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = out[[x_label] + columns].plot(x=x_label, kind="barh")
    else:
        ax = out[[x_label] + columns].plot.barh(x=x_label)
    fig = ax.get_figure()
    lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
           ncol=1, mode="expand", borderaxespad=0.)
    suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_extra_artists=(lgd,), bbox_inches='tight')
    plt.close()


def loadSummaries(files, extract_categories=False):
    df = pandas.DataFrame()
    log.info("Loading files from %s", files)

    for f in glob.glob(files):
        if not os.path.isfile(f):
            continue
        log.info(f)
        frame = pandas.read_csv(f, '\t')
        if extract_categories: 
            category = f.split(os.path.sep)[-2]
            if category == 'subset':
                category = f.split(os.path.sep)[-3]
            frame['category'] = category
        df = pandas.concat([df, frame], ignore_index = True)
    return df
    

def loadMakespans(summary_filename):
    summary = pandas.read_csv(summary_filename, '\t')
    return summary
    
def runScheduler(input, output, k):
    start = time.time()
    
    if not os.path.isdir(os.path.dirname(output)):
        os.makedirs(os.path.dirname(output))
    
    if not os.path.isfile(hcssBinary) :
        raise Exception("Cannot find HCSS scheduler!")
    log.info("Scheduling %s", input)
    
    #remove possibly generated previous output!
    for o in glob.glob(output + ".*"):
        if os.path.isfile(o):
            os.unlink(o)
        
    if DRY_RUN == False:
        sys.stdout.flush()
        sys.stderr.flush()
        completedProcess = subprocess.run([hcssBinary, '-a', 'mdbhcs', '-i', input, '-o', output, '-k', str(k)], capture_output=True)
        if completedProcess.returncode != 0:
            print(completedProcess.returncode)
            raise Exception("Scheduler crashed... check previous output!")
        
        invalid_schedule = []
        for o in glob.glob(output + ".*[0-9]"): # only does the first 10 outputs!!!
            if not verify(input, o):
                invalid_schedule.append(o)
        if len(invalid_schedule) != 0:
            log.error("invalid schedule(s) generated:")
            for i in invalid_schedule:
                log.info(i)
            raise Exception("Scheduler produced incorrect results; check the verification output")
        
    log.info("Scheduling %s took %s seconds", input, str(time.time() - start))

def main(args): 
    if len(args) not in [5,6]:
        raise Exception("Incompatible number of arguments; expected four or five arguments")

    if DRY_RUN:
        log.warn("!!!USING OLD DATA IN DRY RUN!!!")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    output_directory = os.path.normpath(args[2])
    lb_files = os.path.normpath(args[3])
    original_files = os.path.normpath(args[4])
    if len(args) == 6:
        max_k = int(args[5])
    else:
        max_k = 5
    
    for k in range(2,max_k+1):
        print("-- Running experiment with k={0}".format(k))
        experiment(args, input_directory, output_directory + "/" + str(k), lb_files, original_files, k)
        print("-- Done running experiment with k={0}".format(k))
        

def process(f, output_directory, k):
    f = os.path.normpath(f)
       
    output = os.path.join(output_directory, os.path.splitext(os.path.basename(f))[0] + '.out')
    start = time.time()
    runScheduler(f, output, k)
    duration = time.time() - start
    minMakespan = None
    for f in glob.glob(output + ".*[0-9]"): 
        with open(f) as output_file:
            makespan = float(output_file.read().strip('\n').split('\n')[-1].split()[-1]), duration # use the last entry as the resulting makespan values
            if minMakespan is None:
                minMakespan = makespan
            else:
                minMakespan = min(minMakespan, makespan)
    
    return (output, minMakespan)

def initializer():
    """Ignore CTRL+C in the worker process."""
    signal.signal(signal.SIGINT, signal.SIG_IGN)

def experiment(args, input_directory, output_directory, lb_files, original_files, k):
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    res = {}
    #for f in glob.glob(input_directory + '/*.xml'):
    #    output, makespan = process(f, output_directory, k)
    #    res[output] = makespan
    
    proc = functools.partial(process, output_directory=output_directory, k=k)
    nofProcs = cpu_count()-2
    log.info('starting computations on %d cores', nofProcs)
    with Pool(processes=nofProcs, initializer=initializer) as pool:
        try:
            results = pool.map(proc, glob.glob(input_directory + '/*.xml'))
            for f, makespan in results: 
                res[f] = makespan
        except KeyboardInterrupt:
            pool.terminate()
            pool.join()
        
    # store a summary of the output for download in CI
    with open(os.path.join(output_directory, 'summary.txt'), 'w') as summary:
        summary.write('Benchmark\tMakespan\tExecution time\n')
        for output in res:
            summary.write('{0}\t{1}\t{2}\n'.format(os.path.splitext(os.path.basename(output))[0], res[output][0]/1e6, res[output][1]))
    
    # check against the precalculated lowerbounds from CPLEX
    mks = loadMakespans(os.path.join(output_directory, 'summary.txt'))
    lb = loadSummaries(lb_files, extract_categories=True)

    print(lb)
    print(mks)
    out = pandas.merge(lb, mks, on='Benchmark')

    
    print(out)
    out['optimal'] = out.apply(lambda x : x['Lower bound'] == x['Makespan'], axis=1 )
    out['% over lb'] = out.apply(lambda x : (x['Makespan']/x['Lower bound'] -1)*100 if x['Lower bound'] > 0 else np.nan , axis=1 )
    incorrect = out['incorrect'] = out.apply(lambda x : x['Lower bound'] > x['Makespan'], axis=1 )
    
    print("*********************")
    print("*** Optimal cases ***")
    print("*********************")
    print(out[out.optimal == True][['Benchmark', 'Makespan', 'Execution time']])
    print("*************************")
    print("*** Not-optimal cases ***")
    print("*************************")
    print(out[out.optimal == False][['Benchmark', 'Lower bound', '% over lb', 'Makespan', 'Execution time']])
    
    # store the comparison information for download by CI
    out.to_csv(os.path.join(output_directory, 'comparison.txt'), sep='\t')
    if incorrect.sum(): # check if any is True (i.e. non-zero)
    
        print("*************************")
        print("*** INCORRECT cases ***")
        print("*************************")
        print(out[out.incorrect == True][['Benchmark', 'Lower bound', 'Makespan']]) # also includes the 'incorrect' column to show which ones are incorrect now.
        raise Exception("At least one of the generated makespans is incorrect ({0})".format(output_directory))

    with PdfPages(os.path.join(output_directory, 'comparison.pdf')) as pdf:        
        plot_bar_chart(pdf, 'Makespan relative to lowerbound', out, 'Benchmark', ['% over lb'])
        plot_box_plot(pdf, 'Makespan relative to lowerbound', out, 'category', ['% over lb'])
    
if __name__ == '__main__':
    logging.basicConfig()
    main(sys.argv)

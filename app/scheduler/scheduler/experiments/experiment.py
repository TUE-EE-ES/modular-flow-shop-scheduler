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

print("pandas version:", pandas.__version__)
print("matplotlib version:", mpl.__version__)
print("numpy version:", np.__version__)

hcssBinary = "build/HCSS"
import platform
is_windows = any(platform.win32_ver())
if is_windows:
    hcssBinary = hcssBinary + '.exe'

DRY_RUN = False #True # disables scheduling execution and verification when True, but needs the output files to be present

def plot_box_plot(pdf, title, out, group, column):
    data = out[[group, column]]
    ax = data.boxplot(by=group)
    categories = data[group].unique()
    for i, cat in enumerate(sorted(categories)):
        y = data[data[group] == cat][[column]]
        x = np.random.normal(i+1, 0.3/len(categories), len(y))
        # also plot the distribution with some jitter
        plt.plot(x,y, mfc= ['r', 'b', 'g', 'k', 'm', 'orange', 'yellow', 'violet'][i], mec='None', ms=4, marker="o", linestyle="None", alpha=0.2)

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
    for f in glob.glob(files):
        if not os.path.isfile(f):
            continue

        frame = pandas.read_csv(f, '\t')
        if extract_categories:
            frame['category'] = f.split(os.path.sep)[2]
        df = pandas.concat([df, frame], ignore_index = True)
    return df


def loadMakespans(summary_filename):
    summary = pandas.read_csv(summary_filename, '\t')
    return summary

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
        if subprocess.call([hcssBinary, '-a', 'pareto', '-i', input, '-o', output], stdout=sys.stdout, ) != 0:
            raise Exception("Scheduler crashed... check previous output!")

        invalid_schedule = []
        for o in glob.glob(output + ".*[0-9]"):
            if not verify(input, o):
                invalid_schedule.append(o)
        if len(invalid_schedule) != 0:
            print("invalid schedule(s) generated:")
            for i in invalid_schedule:
                print(i)
            raise Exception("Scheduler produced incorrect results; check the verification output")

    print("Scheduling took " + str(time.time() - start) + " seconds")

def main(args):
    if(len(args)) != 6:
        print "Expecting <input> <output> <lowerbounds> <original> <windowed_vishnu>"
        raise Exception("Incompatible number of arguments; expected exactly five arguments")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    output_directory = os.path.normpath(args[2])
    lb_files = os.path.normpath(args[3])
    original_files = os.path.normpath(args[4])
    windowed_vishnu = os.path.normpath(args[5])

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    if not DRY_RUN:
        res = {}
        for f in glob.glob(input_directory + '/*.xml'):
            f = os.path.normpath(f)

            output = os.path.join(output_directory, os.path.splitext(os.path.basename(f))[0] + '.out')
            start = time.time()
            runScheduler(f, output)
            duration = time.time() - start
            makespans = []
            for o in glob.glob(output + ".*[0-9]"): # only does the first 10 outputs!!!
                with open(o) as output_file:
                     makespans.append(float(output_file.read().strip('\n').split('\n')[-1].split()[-1])) # use the last entry as the resulting makespan values
            res[output] = min(makespans), duration
                
        # store a summary of the output for download in CI
        with open(os.path.join(output_directory, 'summary.txt'), 'w') as summary:
            summary.write('Benchmark\tMakespan\tExecution time\n')
            for output in res:
                summary.write('{0}\t{1}\t{2}\n'.format(os.path.splitext(os.path.basename(output))[0], res[output][0]/1e6, res[output][1]))

    # check against the precalculated lowerbounds from CPLEX
    mks = loadMakespans(os.path.join(output_directory, 'summary.txt'))
    vishnu = loadMakespans(windowed_vishnu)

    lb = loadSummaries(lb_files, extract_categories=True)

    out = pandas.merge(lb, mks, on='Benchmark')

    vishnu = vishnu.rename_axis({'Makespan' : 'Makespan Vishnu', 'Execution time' : 'Execution time Vishnu'}, axis="columns")
    out = pandas.merge(out, vishnu, on='Benchmark')

    # compare with original scheduler implementation
    original = loadSummaries(original_files)
    original = original.rename_axis({'Makespan' : 'Makespan original', 'Execution time' : 'Execution time original'}, axis="columns")

    out = pandas.merge(out, original, on="Benchmark")

    out['optimal'] = out.apply(lambda x : x['Lower bound'] == x['Makespan'], axis=1 )
    out['% over lb'] = out.apply(lambda x : (x['Makespan']/x['Lower bound'] -1)*100 if x['Lower bound'] > 0 else np.nan , axis=1 )
    out['execution time ratio vs original'] = out.apply(lambda x : (x['Execution time']/x['Execution time original']) if x['Execution time original'] > 0 else np.nan , axis=1 )
    out['execution time ratio vs Vishnu'] = out.apply(lambda x : (x['Execution time']/x['Execution time Vishnu']) if x['Execution time Vishnu'] > 0 else np.nan , axis=1 )
    incorrect = out['incorrect'] = out.apply(lambda x : x['Lower bound'] > x['Makespan'], axis=1 )

    print "*********************"
    print "*** Optimal cases ***"
    print "*********************"
    print out[out.optimal == True][['Benchmark', 'Makespan', 'Makespan original', 'Execution time', 'Execution time original']]
    print "*************************"
    print "*** Not-optimal cases ***"
    print "*************************"
    print out[out.optimal == False][['Benchmark', 'Lower bound', '% over lb', 'Makespan', 'Makespan original', 'Execution time', 'Execution time original']]

    # store the comparison information for download by CI
    out.to_csv(os.path.join(output_directory, 'comparison.txt'), sep='\t')
    if incorrect.sum(): # check if any is True (i.e. non-zero)

        print "*************************"
        print "*** INCORRECT cases ***"
        print "*************************"
        print out[out.incorrect == True][['Benchmark', 'Lower bound', 'Makespan']] # also includes the 'incorrect' column to show which ones are incorrect now.
        raise Exception("At least one of the generated makespans is incorrect")

    out['Relative makespan'] = out.apply(lambda x : (x['Makespan']/x['Makespan original'] -1)*100 if x['Makespan original'] > 0 else np.nan , axis=1 )
    out['Relative makespan Vishnu'] = out.apply(lambda x : (x['Makespan']/x['Makespan Vishnu'] -1)*100 if x['Makespan Vishnu'] > 0 else np.nan , axis=1 )

    out['Execution time ratio'] = out.apply(lambda x : (x['Execution time']/x['Execution time original'])*100 if x['Execution time original'] > 0 else np.nan , axis=1 )
    out['Execution time ratio Vishnu'] = out.apply(lambda x : (x['Execution time']/x['Execution time Vishnu'])*100 if x['Execution time Vishnu'] > 0 else np.nan , axis=1 )

    with PdfPages(os.path.join(output_directory, 'comparison.pdf')) as pdf:
        # export the makespan relative to the original
        plot_bar_chart(pdf, 'Makespan relative to original DATE heuristic', out, 'Benchmark', ['Relative makespan'])
        plot_bar_chart(pdf, 'Makespan relative to Vishnu windowed', out, 'Benchmark', ['Relative makespan Vishnu'])

        plot_bar_chart(pdf, 'Makespan relative to lowerbound', out, 'Benchmark', ['% over lb'])
        # export the execution time relative to the original
        plot_bar_chart(pdf, 'Execution time relative to original DATE heuristic', out, 'Benchmark', ['Execution time ratio'])
        plot_bar_chart(pdf, 'Execution time relative to Vishnu windowed', out, 'Benchmark', ['Execution time ratio Vishnu'])

        # export the makespan relative to the original
        plot_box_plot(pdf, 'Makespan relative to original DATE heuristic', out, 'category', 'Relative makespan')
        plot_box_plot(pdf, 'Makespan relative to Vishnu windowed', out, 'category', 'Relative makespan Vishnu')

        plot_box_plot(pdf, 'Makespan relative to lowerbound', out, 'category', '% over lb')
        # export the execution time relative to the original
        plot_box_plot(pdf, 'Execution time relative to original DATE heuristic', out, 'category', 'Execution time ratio')
        plot_box_plot(pdf, 'Execution time relative to Vishnu windowed', out, 'category', 'Execution time ratio Vishnu')
if __name__ == '__main__':
    main(sys.argv)

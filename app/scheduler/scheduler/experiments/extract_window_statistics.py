import matplotlib as mpl # to run plot without x-server backend
mpl.use('Agg') # must be set before any other import of pylab/matplotlib/pyplot/pandas
import matplotlib.pyplot as plt
import matplotlib
from matplotlib.backends.backend_pdf import PdfPages
import inspect
import numpy as np
from functools import partial

if matplotlib.__version__ == "1.3.3":
    print "unable to plot using old matplotlib"
    exit()

import os, sys, pandas

def plot_scatter(pdf, data, title, x_axis, y_axes=None, ax=None):

    if y_axes == None:
        y_axes = []
    ax2 = None
    if ax == None:
        fig, (ax, ax2) = plt.subplots(2, sharex=True, figsize=(14,7))
    else:
        fig = ax.get_figure()
    colors = ['r', 'b', 'g', 'k', 'm']
    
    for index, i in enumerate(data.columns):
        if i in y_axes:
            ax.scatter(data[x_axis], data[i], label=i, color=colors[index])
        
    lgd = ax.legend(ncol=1, borderaxespad=0.)
    
    return ax2
    

def main(filename):

    window_data = []
    choice_data = []
    with open(filename) as f:
        # state-based parsing:
        for line in f:
            if line.startswith('Solving FOR') and not line.strip().endswith('.'):
                case = os.path.split(line.strip().split()[-1])[-1]
                iteration = 0
            if line.startswith('Window'):
                start, end = line.split('[')[-1].strip().strip(']').split(',')
                start, end = int(start), int(end)
                window_size = end - start
                window_data.append([case, iteration, window_size, start, end])
                
            if line.startswith('-- Size:'):
                initial, target = line.strip().split(':')[-1].split(' became ')
                initial = int(initial.strip())
                intermediate, pareto = [int(x) for x in target.strip().split('/')]

                choice_data.append([case, iteration, initial, intermediate, pareto])
                iteration += 1
                
    window_df = pandas.DataFrame(window_data, columns=('Benchmark', 'iteration', 'window-size', 'start', 'end'))
    choice_df = pandas.DataFrame(choice_data, columns=('Benchmark', 'iteration', 'initial', 'intermediate', 'pareto'))
    with PdfPages('window_extraction.pdf') as pdf:        
        
        fig = plt.figure()
        fig.text(0.5,0.5, "Evolution of window sizes and \npartial solutions considered \nper scheduled operation", fontsize=24, ha='center')
        pdf.savefig(fig)
        
        for b in window_df['Benchmark'].unique():
            data = window_df[window_df['Benchmark']==b]
            ax = plot_scatter(pdf, data, b, 'iteration', ['window-size', 'start', 'end'])
            
            data = choice_df[choice_df['Benchmark']==b]
            plot_scatter(pdf, data, b, 'iteration', ['initial', 'intermediate', 'pareto'], ax=ax)
            fig = ax.get_figure()
            ax.set_ylim([0,90])
            suptitle = fig.suptitle(b)

            pdf.savefig(fig, bbox_inches='tight')
            plt.close(fig)
    
    print "Window size: \tmin {0}, \tmean {1}, \tmax {2}".format(window_df['window-size'].min(), window_df['window-size'].mean(), window_df['window-size'].max())
    print " - Maximum window size encountered for {0}".format(window_df['Benchmark'][window_df['window-size'].argmax()])
    
    print "Initial nr choices: \tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['initial'].min(), choice_df['initial'].mean(), choice_df['initial'].max())
    print "Intermediate solutions:\tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['intermediate'].min(), choice_df['intermediate'].mean(), choice_df['intermediate'].max())
    print "Pareto solutions: \tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['pareto'].min(), choice_df['pareto'].mean(), choice_df['pareto'].max())
            
if __name__ == "__main__":
    main(*sys.argv[1:])
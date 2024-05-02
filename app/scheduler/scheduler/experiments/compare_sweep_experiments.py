#!/usr/bin/env python
import os, sys, time
import glob
import subprocess
import matplotlib as mpl # to run plot without x-server backend
mpl.use('Agg') # must be set before any other import of pylab/matplotlib/pyplot/pandas
from latexify import latexify, format_axes
latexify(fig_height=2)
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import FuncFormatter
import pandas 
import numpy as np
import inspect
from verify import verify

hcssBinary = "build/HCSS"

def plot_box_plot(pdf, title, out, group, columns):

    ax = out[[group] + columns].boxplot(by=group)
    fig = ax.get_figure()
    suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_inches='tight')
    plt.close()

def plot_bar_chart(pdf, title, out, xlabel=None, ylabel=None):
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = out.plot(kind="bar", zorder=3)
    else:
        ax = out.plot.bar(zorder=3)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 
    
    if ylabel != None:
        ax.set_ylabel(ylabel)
    if xlabel != None:
        ax.set_xlabel(xlabel)
    ax.grid(zorder=0)
    
    gridlines = ax.get_xgridlines() + ax.get_ygridlines()
    for line in gridlines:
        line.set_linestyle(':')
    fig = ax.get_figure()
    #lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
           # ncol=1, mode="expand", borderaxespad=0.)
    #suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    ax.xaxis.grid(False)
    ax.yaxis.grid(True)
    ax.tick_params(direction='out', length=0, width=0)
    plt.axhline(0.03079, color="red", zorder=4)
    pdf.savefig(bbox_inches='tight')
    plt.savefig('k_sensitivity_makespan.pgf', bbox_inches='tight')

    plt.close()

def plot_bar_chart_(pdf, title, out, columns, lowerbound):
    data = out[columns].apply(lambda x : (x/lowerbound-1)*100, axis=1 )
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = data.plot(kind="bar")
    else:
        ax = data.plot.bar()
    
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
    return df.set_index('Benchmark')
    

def loadMakespans(summary_filename):
    summary = pandas.read_csv(summary_filename, '\t').set_index('Benchmark')
    return summary

def main(args): 
    if(len(args)) != 4:
        raise Exception("Incompatible number of arguments; expected exactly 3 arguments")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    comparison_directory = os.path.normpath(args[2])
    lb_files = os.path.normpath(args[3])
    
    lb = loadSummaries(lb_files, extract_categories=True)
    
    comparison = loadSummaries(comparison_directory).rename_axis({"Makespan" : "Comparison"}, axis="columns")
    
    mks = None
    for d in glob.glob(input_directory):
        if os.path.isdir(d):
            case = os.path.split(d)[-1]
            
            try:
                s = loadMakespans(os.path.join(d, "summary.txt")).rename_axis({"Makespan" : case}, axis="columns")[[case]].transpose()
                if(type(mks) != type(None)):
                    mks = mks.append(s)
                else:
                    mks = s
            except:
                pass
    
    
    if(len(mks) > 1):
        mks.index = mks.index.astype(int) # convert strings back to integers

    mks = mks.sort_index()
    
    comparison.to_csv('comparisons.txt')
    # add AVERAGE per K plot
    with PdfPages('paramter_sweep_k.pdf') as pdf:        
        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nmaximum number of intermediate solutions", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)
        
        mean = mks.copy()
        for column in mks.columns:
            if column in comparison.index.values and type(comparison.loc[column]['Comparison']) == np.float64:
                mean[column] = (comparison.loc[column]['Comparison']/mean[column] - 1)

        mean.transpose().mean().to_csv('means.txt')
                
        plot_bar_chart(pdf, 'Average deviation from comparison',  mean.transpose().mean(), xlabel="Number of intermediate solutions $k$", ylabel="Makespan improvement")        

        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nseparate cases", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)
        
                
        # for column in mks.columns:
            # if column in comparison.index.values:
                # if type(comparison.loc[column]['Comparison']) == np.float64:
                    # plot_bar_chart_(pdf, '', mks, [column], comparison.loc[column]['Comparison'])
                # else:
                    # print "multiple values found for comparison... ", column
            # else:
                # print "no comparison found for ", column
        
        mks.to_csv('mks.txt')
        
        #plot_bar_chart_(pdf, column, mks, [column], lb.loc[column]['Lower bound'])
                
if __name__ == '__main__':
    main(sys.argv)

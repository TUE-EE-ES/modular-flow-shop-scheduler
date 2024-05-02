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

hcssBinary = "build/HCSS"

def plot_box_plot(pdf, title, out, group, columns):

    ax = out[[group] + columns].boxplot(by=group)
    fig = ax.get_figure()
    suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_inches='tight')
    plt.close()

def plot_bar_chart(pdf, title, out):
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = out.plot(kind="bar")
    else:
        ax = out.plot.bar()
    fig = ax.get_figure()
    lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
           ncol=1, mode="expand", borderaxespad=0.)
    suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_extra_artists=(lgd,), bbox_inches='tight')
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
    if(len(args)) != 3:
        raise Exception("Incompatible number of arguments; expected exactly 2 arguments")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    lb_files = os.path.normpath(args[2])
    
    lb = loadSummaries(lb_files, extract_categories=True)
    
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
    
    mks = mks.sort_index()
        
    # add AVERAGE per K plot
    with PdfPages('parameter_sweep_k.pdf') as pdf:        
        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nmaximum number of intermediate solutions", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)
        
        mean = mks.copy()
        for column in mks.columns:
            if column in lb.index.values and type(lb.loc[column]['Lower bound']) == np.float64:
                mean[column] = (mean[column]/lb.loc[column]['Lower bound'] - 1)* 100
                
        plot_bar_chart(pdf, 'Average deviation from lower bound',  mean.transpose().mean())        

        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nseparate cases", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)
                
        for column in mks.columns:
            if column in lb.index.values:
                if type(lb.loc[column]['Lower bound']) == np.float64:
                    plot_bar_chart_(pdf, '', mks, [column], lb.loc[column]['Lower bound'])
                else:
                    print "multiple values found for lowerbound... ", column
            else:
                print "no lower bound found for ", column

        mks.to_csv('mks.txt')
        
        #plot_bar_chart_(pdf, column, mks, [column], lb.loc[column]['Lower bound'])
                
if __name__ == '__main__':
    main(sys.argv)

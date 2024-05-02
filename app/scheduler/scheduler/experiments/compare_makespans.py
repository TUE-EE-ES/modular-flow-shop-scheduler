#!/usr/bin/env python
from __future__ import print_function
import os, sys, time
import glob
import subprocess
import matplotlib as mpl # to run plot without x-server backend
mpl.use('Agg') # must be set before any other import of pylab/matplotlib/pyplot/pandas
from latexify import latexify, format_axes
latexify(fig_height=3)
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import FuncFormatter
import pandas 
import numpy as np
import inspect
#from verify import verify
def plot_box_plot(pdf, title, out, group, columns, pgf=None):
    flierprops = dict(linewidth= 0.5, marker='+', markerfacecolor='black', markersize=4, markeredgewidth=0.4)

    lw = {'linewidth':0.5}
    ax = out[[group] + columns].boxplot(by=group, capprops=lw, boxprops=lw, medianprops=lw, whiskerprops=lw, flierprops=flierprops)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 

    fig = ax.get_figure()
    # clear all auto-generated titles
    fig.texts = [] 
    fig.suptitle("")
    ax.set_xlabel("")
    ax.set_title("")
    
    if pgf != None:
        plt.savefig(pgf)
        
    suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_inches='tight')

    plt.close()
    
def plot_box_plot_(pdf, out, group, pgf=None):
    global a 
    a = out
    flierprops = dict(linewidth= 0.5, marker='+', markerfacecolor='black', markersize=4, markeredgewidth=0.4)
    lw = {'linewidth':0.5}
    for ax in out.boxplot(by='category', capprops=lw, medianprops=lw, flierprops=flierprops, boxprops=lw, whiskerprops=lw):
        #for c in out.columns:
        #    if c != group:
        #        ax_fig = out[[c, group]].boxplot(by=group, return_type="axes", ax=ax)
        #        print(ax, ax_fig)
        #        if ax == None:
        #            ax = ax_fig[c]
        ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 

        fig = ax.get_figure()
        # clear all auto-generated titles
        #fig.texts = [] 
        fig.suptitle("")
        ax.set_xlabel(group)
        #ax.set_title("")
        ax.set_ylabel('% improvement over HCS')


    fig.tight_layout() # does not seem to work properly yet?
    if pgf != None:
        plt.savefig(pgf)
        
    #suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    pdf.savefig(bbox_inches='tight')
    plt.show()
    plt.close()

def plot_bar_chart(pdf, title, out, pgf=None, xlabel=None, ylabel=None, y_level=None):
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = out.plot(kind="bar", legend=False)
    else:
        ax = out.plot.bar(legend=False)
    #ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 
    
    if ylabel != None:
        ax.set_ylabel(ylabel)
    if xlabel != None:
        ax.set_xlabel(xlabel)
    ax.grid()
    ax.set_axisbelow(True)
    
    ax.tick_params(direction='out', length=0, width=0)

    gridlines = ax.get_xgridlines() + ax.get_ygridlines()
    for line in gridlines:
        line.set_linestyle(':')
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 
    fig = ax.get_figure()
    plt.axhline(y_level, color="red")
    ax.xaxis.grid(False)
    ax.yaxis.grid(True)
    #lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
    #       ncol=1, mode="expand", borderaxespad=0.)
    suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_inches='tight')
    if pgf != None:
        plt.savefig(pgf, bbox_inches='tight')

    plt.close()

def plot_bar_chart_(pdf, title, out, columns, lowerbound):
    data = out[columns].apply(lambda x : (x/lowerbound-1)*100, axis=1 )
    if(inspect.ismethod(pandas.DataFrame().plot)):
        ax = data.plot(kind="bar")
    else:
        ax = data.plot.bar()
    
    ax.grid()
    ax.set_axisbelow(True)
    
    gridlines = ax.get_xgridlines() + ax.get_ygridlines()
    for line in gridlines:
        line.set_linestyle(':')
    
    fig = ax.get_figure()
    lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
           ncol=1, mode="expand", borderaxespad=0.)
    suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    pdf.savefig(bbox_extra_artists=(lgd,), bbox_inches='tight')
    plt.close()
    
def plot_cdf(pdf, title, ser, pgf=None, **kwargs):
    #ser[len(ser)] = ser.iloc[-1]
    ser = ser.sort_values()
    cum_dist = np.linspace(0.,1.,len(ser))
    ser_cdf = pandas.Series(cum_dist, index=ser)
    #Finally, plot the function as steps:
    ax = ser_cdf.plot(drawstyle='steps', grid=True, **kwargs)
    fig = ax.get_figure()

    ax.set_xlabel('% above lower bound')
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 
    ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: '{:.0%}'.format(x))) 

    ax.set_ylabel('% of benchmark instances')

    fig.tight_layout() # does not seem to work properly yet?
    if pgf != None:
        plt.savefig(pgf)

    #suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    pdf.savefig(bbox_inches='tight')
    plt.show()
    plt.close()
    
def loadSummaries(files, extract_categories=False):
    df = pandas.DataFrame()
    for f in glob.glob(files):
        if not os.path.isfile(f):
            continue
            
        frame = pandas.read_csv(f, '\t')
        if extract_categories: 
            category_name = f.split(os.path.sep)[-2]
           
            if category_name == 'subset':
                category_name = f.split(os.path.sep)[-3]
                
            frame['category'] = category_name
        df = pandas.concat([df, frame], ignore_index = True)
    return df.set_index('Benchmark')
    

def loadMakespans(summary_filename):
    summary = pandas.read_csv(summary_filename, '\t').set_index('Benchmark')
    return summary

def main(args): 
    if(len(args)) != 6:
        raise Exception("Incompatible number of arguments; expected exactly 5 arguments")
    # extract the input directory:
    input_directory = os.path.normpath(args[1])
    input_directory_2 = os.path.normpath(args[2]) # BHCS
    input_directory_all = os.path.normpath(args[3]) # MD-BHCS
    comparison_directory = os.path.normpath(args[4])
    lb_files = os.path.normpath(args[5])
    
    lb = loadSummaries(lb_files, extract_categories=True)
    
    comparison = loadSummaries(comparison_directory).rename_axis({"Makespan" : "Comparison"}, axis="columns")
    #rint(comparison.join(lb))
    mks = None
    for d in glob.glob(input_directory):
        if os.path.isdir(d):
            case = os.path.split(d)[-1]
            if case == os.path.split(input_directory)[-1]:
                case = "BHCS"
            else:
                case = "MD-BHCS"
            print(case)
            try:
                s = loadMakespans(os.path.join(d, "summary.txt")).rename_axis({"Makespan" : case}, axis="columns")[[case]].transpose()
                if(type(mks) != type(None)):
                    mks = mks.append(s)
                else:
                    mks = s
            except:
                pass
    for d in glob.glob(input_directory_2):
        if os.path.isdir(d):
            case = os.path.split(d)[-1]
            if case == os.path.split(input_directory)[-1]:
                case = "BHCS"
            else:
                case = "MD-BHCS"            
            try:
                s = loadMakespans(os.path.join(d, "summary.txt")).rename_axis({"Makespan" : case}, axis="columns")[[case]].transpose()
                if(type(mks) != type(None)):
                    mks = mks.append(s)
                else:
                    mks = s
            except:
                pass
    mks_all = None
    for d in glob.glob(input_directory_all):
        if os.path.isdir(d):
            case = os.path.split(d)[-1]
            try:
                s = loadMakespans(os.path.join(d, "summary.txt")).rename_axis({"Makespan" : case}, axis="columns")[[case]].transpose()
                if(type(mks_all) != type(None)):
                    mks_all = mks_all.append(s)
                else:
                    mks_all = s
            except:
                pass
    #if(len(mks) > 1):
    mks_all.index = mks_all.index.astype(int) # convert strings back to integers
    mks_all = mks_all.sort_index()
    mks = mks.sort_index()
    comparison = comparison[['Comparison']]
    comparison = comparison.join(mks.transpose())
    comparison.to_csv('comparisons.txt')
    #rint(comparison)
    # add AVERAGE per K plot
    with PdfPages('makespan_overview.pdf') as pdf:        
        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nmaximum number of partial solutions", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)
        
        comp = comparison.copy()
        #for column in comp.columns:
        #    if column != 'Comparison':
        #        comp[column] = comp.apply(lambda x : (x[column]/x['Comparison']-1) if x['Comparison'] != 0 else 0, axis=1 )
        #print('*** comp ***')
        over_lb = mks.transpose().join(lb[['Lower bound']])
        
        #for column in over_lb.columns:
            #if column != 'Lower bound':
                #over_lb[column] = over_lb.apply(lambda x : (x[column]/x['Lower bound']-1), axis=1 )
                #plot_cdf(pdf, column, pandas.Series(over_lb[column]).sort_values()[:-2], pgf=str(column)+"_makespan_cdf.pgf")
        
        mean = mks.copy()
        for column in mks.columns:
            if column in comparison.index.values and type(comparison.loc[column]['Comparison']) == np.float64:
                mean[column] = (comparison.loc[column]['Comparison']/mean[column] - 1)
        mean_all = mks_all.copy()
        for column in mks_all.columns:
            if column in comparison.index.values and type(comparison.loc[column]['Comparison']) == np.float64:
                mean_all[column] = (comparison.loc[column]['Comparison']/mean_all[column] - 1)                
        print(mean_all)
        plot_bar_chart(pdf, '',  mean_all.transpose().mean(), pgf='k-sensitivity-makespan.pgf', xlabel="Number of partial solutions $k$", ylabel="Makespan improvement", y_level=mean.transpose().mean()['BHCS'])        
        print(mean.transpose().mean())
        mean_all.transpose().mean().to_csv('means.txt')

        fig = plt.figure()
        fig.text(0.5,0.5, "Sensitivity to\nparameter $k$\nseparate cases", fontsize=24, ha='center')
        pdf.savefig(fig)
        plt.close(fig)

        lb.ix[lb['category'] == "sheetProperties", "category"] = "H"
        lb.ix[lb['category'] == "bookletA", "category"] = "RB"
        lb.ix[lb['category'] == "bookletB", "category"] = "RA"
        lb.ix[lb['category'] == "length", "category"] = "BA"
        lb.ix[lb['category'] == "thickness", "category"] = "BB"

        categorized = mean.transpose().join(lb['category'])
        #grouped = categorized.groupby("category")
        
        #for g in grouped.groups:
        #    data = grouped.get_group(g)
        #    print(data)
        print("** Plotting **")
        for c in categorized.columns:
            if c != "category":
                print(c)
                plot_box_plot(pdf, c, categorized[[c, 'category']], 'category', [c], pgf=str(c)+"_makespan_comparison.pgf")
                #plot_cdf(pdf, c, categorized[[c]], pgf=str(c)+"_makespan_cdf.pgf")
                plt.show()
        plot_box_plot_(pdf, categorized, 'category', pgf="makespan_comparison_all.pgf")

        
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

#main(['main',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/experiment_fwd_28-02-2017/',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/experiment_2_10-02-2017_3d_absolute_push_vs_nr_ops/20/',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/experiment_2_10-02-2017_3d_absolute_push_vs_nr_ops/*/',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/original/*/*.txt',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/lowerbounds/*/*'
#     ])        
        
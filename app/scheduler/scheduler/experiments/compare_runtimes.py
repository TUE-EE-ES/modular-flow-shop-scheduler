import matplotlib as mpl # to run plot without x-server backend
mpl.use('Agg') # must be set before any other import of pylab/matplotlib/pyplot/pandas
from latexify import latexify, format_axes
latexify(fig_height=3)
import matplotlib.pyplot as plt
import matplotlib
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.ticker import FuncFormatter
import inspect
import numpy as np
from functools import partial

import glob

import os, sys, pandas

def plot_box_plot(pdf, title, out, group, column, pgf=None, xlabel=None, ylabel=None, **kwargs):
    data = out[[group, column]]
    ax = data.boxplot(by=group, sym='+', **kwargs)
    categories = data[group].unique()
    # for i, cat in enumerate(sorted(categories)):
        # y = data[data[group] == cat][[column]]
        # x = np.random.normal(i+1, 0.3/len(categories), len(y))
        # # also plot the distribution with some jitter
        # plt.plot(x,y, mfc= ['r', 'b', 'g', 'k', 'm', 'orange', 'yellow', 'violet'][i], mec='None', ms=4, marker="o", linestyle="None", alpha=0.2)

    fig = ax.get_figure()
    # clear all auto-generated titles
    fig.texts = [] 
    fig.suptitle("")
    ax.set_xlabel("")
    ax.set_title("")
    #ax.set_xlabel('Category')
    ax.set_ylabel('Time/iteration (ms)')
    ax.set_ylim(None, 600)

    fig.tight_layout() # does not seem to work properly yet?
    if pgf != None:
        plt.savefig(pgf)

    #suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    pdf.savefig(bbox_inches='tight')
    plt.show()
    plt.close()
    
    
def plot_cdf(pdf, title, ser, xlabel=None, ylabel=None, pgf=None, **kwargs):
    #ser[len(ser)] = ser.iloc[-1]

    cum_dist = np.linspace(0.,1.,len(ser))
    ser_cdf = pandas.Series(cum_dist, index=ser.sort_values())
    #Finally, plot the function as steps:
    ax = ser_cdf.plot(drawstyle='steps', grid=True, **kwargs)
    fig = ax.get_figure()

    if xlabel != None:
        ax.set_xlabel(xlabel)
    if ylabel != None:
        ax.set_ylabel(ylabel)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: '{:.0%}'.format(y))) 
#    ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: '{:.0%}'.format(x))) 

    

    fig.tight_layout() # does not seem to work properly yet?
    if pgf != None:
        plt.savefig(pgf)

    #suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    pdf.savefig(bbox_inches='tight')
    plt.show()
    plt.close()    
def plot_box_plot_(pdf, title, out, group, pgf=None, **kwargs):
    data = out
    lw = {'linewidth' : 0.5}
    for ax in data.boxplot(by=group, sym='+', capprops=lw, boxprops=lw, whiskerprops=lw, medianprops=lw, **kwargs):
    #categories = data[group].unique()
    # for i, cat in enumerate(sorted(categories)):
        # y = data[data[group] == cat][[column]]
        # x = np.random.normal(i+1, 0.3/len(categories), len(y))
        # # also plot the distribution with some jitter
        # plt.plot(x,y, mfc= ['r', 'b', 'g', 'k', 'm', 'orange', 'yellow', 'violet'][i], mec='None', ms=4, marker="o", linestyle="None", alpha=0.2)

        fig = ax.get_figure()
        # clear all auto-generated titles
        #fig.texts = [] 
        fig.suptitle("")
        ax.set_xlabel(group)
        #ax.set_title("")
        ax.set_ylabel('Time/iteration (ms)')
        ax.set_ylim(None, 600)

        fig.tight_layout() # does not seem to work properly yet?
    if pgf != None:
        plt.savefig(pgf)

    #suptitle = fig.suptitle(title) # TODO: fix suptitle placement
    pdf.savefig(bbox_inches='tight')
    plt.show()
    plt.close()

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
    
def plot_bar_chart(pdf, title, out, xlabel=None, ylabel=None, pgf=None):
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

    gridlines = ax.get_xgridlines() + ax.get_ygridlines()
    for line in gridlines:
        line.set_linestyle(':')
    fig = ax.get_figure()
    #lgd = plt.legend(bbox_to_anchor=(0., 1.02, 1., .102), loc='upper center',
            #ncol=1, mode="expand", borderaxespad=0.)
    #lgd.remove()
    #suptitle = fig.suptitle(title)  # TODO: fix suptitle placement
    fig.tight_layout() # does not seem to work properly yet?
    ax.xaxis.grid(False)
    ax.yaxis.grid(True)
    ax.tick_params(direction='out', length=0, width=0)
    plt.axhline(8.699014, color="orange")
    pdf.savefig(bbox_inches='tight')
    if pgf != None:
        plt.savefig(pgf, bbox_inches='tight')

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
    return df

def extract_runtime_data(filename):
    runtime_data = []
    choice_data = []
    k = -1
    with open(filename) as f:
        # state-based parsing:
        for line in f:
            if line.startswith('-- Running experiment'):
                k = int(line.strip().split('=')[-1])
            if line.startswith('Solving FOR') and not line.strip().endswith('.'):
                case = os.path.split(line.strip().split()[-1])[-1]
                iteration = 0
#            if line.startswith('Window'):
#                start, end = line.split('[')[-1].strip().strip(']').split(',')
#                start, end = int(start), int(end)
#                window_size = end - start
#                window_data.append([case, iteration, window_size, start, end])
                
            if line.startswith('-- Size:'):
                initial, target = line.strip().split(':')[-1].split(' became ')
                initial = int(initial.strip())
                intermediate, pareto = [int(x) for x in target.strip().split('/')]

                choice_data.append([case, iteration, initial, intermediate, pareto])
                #iteration += 1
            if line.startswith('Scheduled operation'):
                t = int(line.strip().split(" ")[-1].split('ms')[0])
                iteration += 1
                runtime_data.append([k, case, iteration, t])
                
    return runtime_data

def main(filename_bhcs, filename_mdbhcs, original_files, lb_files, k):

    k = int(k)
    original_schedules = loadSummaries(original_files)    
    lb_files = os.path.normpath(lb_files)

    lb = loadSummaries(lb_files, extract_categories=True)
    categories = lb[['Benchmark', 'category']]
    
    
    #window_df = pandas.DataFrame(window_data, columns=('Benchmark', 'iteration', 'window-size', 'start', 'end'))
    #choice_df = pandas.DataFrame(choice_data, columns=('Benchmark', 'iteration', 'initial', 'intermediate', 'pareto'))
    runtime_df = pandas.DataFrame(extract_runtime_data(filename_bhcs), columns=('case', 'Benchmark', 'iteration', 'Time'))
    runtime_df['case'] = 'BHCS'
    runtime_df_2 = pandas.DataFrame(extract_runtime_data(filename_mdbhcs), columns=('case', 'Benchmark', 'iteration', 'Time'))
    
    runtime_df['Benchmark'] = runtime_df['Benchmark'].apply(lambda x : x.strip('.xml'))
    runtime_df_2['Benchmark'] = runtime_df_2['Benchmark'].apply(lambda x : x.strip('.xml'))
    runtime_df_2_full = runtime_df_2.copy()

    runtime_df['case'] = 'BHCS'

    runtime_df_2 = runtime_df_2[runtime_df_2['case'] == k]
    runtime_df_2['case'] = 'MD-BHCS'
    
    runtime_df = pandas.merge(runtime_df, categories, on='Benchmark')
    runtime_df_2 = pandas.merge(runtime_df_2, categories, on='Benchmark')
    runtime_df.to_csv("runtime_details.csv", sep="\t")
    
    runtime_instance = runtime_df_2.groupby(["case", "Benchmark"], as_index=False).agg({'iteration': np.max, 'Time':np.sum})
    runtime_instance['iteration'] = runtime_instance['iteration'] + 1
    combined_orig_runtime = runtime_instance.merge(original_schedules)
    combined_orig_runtime.to_csv('combined_orig_runtime.csv')

    with PdfPages('runtime_overview.pdf') as pdf:        
        
        #plot_cdf(pdf, "", combined_orig_runtime['Time']/1000/combined_orig_runtime['Execution time'], xlabel="Execution time ratio", ylabel="Percentage of cases", pgf="runtime_comparison_original.pgf")
        
        fig = plt.figure()
        fig.text(0.5,0.5, "Evolution of window sizes and \npartial solutions considered \nper scheduled operation", fontsize=24, ha='center')
        pdf.savefig(fig)
        
        mean_data = []
        runtime_df.ix[runtime_df['category'] == "sheetProperties", "category"] = "H"
        runtime_df.ix[runtime_df['category'] == "bookletA", "category"] = "RB"
        runtime_df.ix[runtime_df['category'] == "bookletB", "category"] = "RA"
        runtime_df.ix[runtime_df['category'] == "length", "category"] = "BA"
        runtime_df.ix[runtime_df['category'] == "thickness", "category"] = "BB"
        
        runtime_df_2.ix[runtime_df_2['category'] == "sheetProperties", "category"] = "H"
        runtime_df_2.ix[runtime_df_2['category'] == "bookletA", "category"] = "RB"
        runtime_df_2.ix[runtime_df_2['category'] == "bookletB", "category"] = "RA"
        runtime_df_2.ix[runtime_df_2['category'] == "length", "category"] = "BA"
        runtime_df_2.ix[runtime_df_2['category'] == "thickness", "category"] = "BB"
        #runtime_df.to_csv('runtime_details.csv')
        cases = runtime_df_2.groupby('case')
        for k in cases.groups:
            case_data = cases.get_group(k)
            case_data.to_csv('runtime_details_{0}.csv'.format(k))
            
            result = []
            datas = case_data.groupby('Benchmark')
            
            for b in datas.groups:
                data = datas.get_group(b)
                #print(data['Time'].mean())
                d = data.ix[data['Time'].argmax()]
                if int(d['iteration']) != 1 and int(d['Time']) > 400:
                    print("{0} - {1} - {2}ms - {3}".format(d['case'], d['Benchmark'], d['Time'], d['iteration']))
                result.append([data.iloc[0]['case'], data.iloc[0]['Benchmark'], data.iloc[0]['category'], data['Time'].min(), data['Time'].mean(), data['Time'].max()])
                
                #ax = plot_scatter(pdf, data, b, 'iteration', ['Time'])
                
                #fig = ax.get_figure()
                
                #suptitle = fig.suptitle(b)

                #pdf.savefig(fig, bbox_inches='tight')
                #plt.close(fig)
            # plot_box_plot(pdf, 'Runtime', pandas.DataFrame(result, columns=('case', 'Benchmark', 'category', 'Min', 'Mean', 'Max')), 'category', 'Mean', whis='range', pgf="k_" + str(k) + "_runtime_boxplot.pgf")
            plot_box_plot(pdf, 'Runtime', case_data, 'category', 'Time', whis="range", pgf="k_" + str(k) + "_runtime_boxplot.pgf")
            mean_data += result

        # add box plot of combination of the two experiments
        #mean_data += result
        cases = runtime_df[['case', 'Time']].groupby('case')
        print(cases['Time'].agg([pandas.np.min, pandas.np.max, pandas.np.mean], index=False))
        
        print("Number of rows (total) ", runtime_df.shape[0])
        plot_bar_chart(pdf, '', runtime_df_2_full[['case', 'Time']].groupby('case').mean(), xlabel="Number of partial solutions $k$", ylabel="Avg. time/iteration (ms)", pgf="k-sensitivity-runtime.pgf")
       
        del runtime_df['case']
        del runtime_df_2['case']
        runtime_df_3 = pandas.merge(runtime_df.rename(columns={'Time' : 'BHCS'}), runtime_df_2.rename(columns={'Time' : 'MD-BHCS'}), on=["Benchmark", "iteration", 'category'])
        del runtime_df_3['iteration']
        plot_box_plot_(pdf, '', runtime_df_3, 'category', whis="range", pgf="comparison_runtime.pgf")
    
    #mean_df = pandas.DataFrame(mean_data, columns=('case', 'Benchmark', 'category', 'Min', 'Mean', 'Max'))
    
    #mean_df.to_csv('mean_running_time.csv')
    #print "Window size: \tmin {0}, \tmean {1}, \tmax {2}".format(window_df['window-size'].min(), window_df['window-size'].mean(), window_df['window-size'].max())
    #print " - Maximum window size encountered for {0}".format(window_df['Benchmark'][window_df['window-size'].argmax()])
    
    #print "Initial nr choices: \tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['initial'].min(), choice_df['initial'].mean(), choice_df['initial'].max())
    #print "Intermediate solutions:\tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['intermediate'].min(), choice_df['intermediate'].mean(), choice_df['intermediate'].max())
    #print "Pareto solutions: \tmin {0}, \tmean {1}, \tmax {2}".format(choice_df['pareto'].min(), choice_df['pareto'].mean(), choice_df['pareto'].max())

if __name__ == '__main__':
    main(*sys.argv[1:])    
    
#main('D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/exp_fwd_28-02-2017',
#     'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/concat.txt',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/original/*/*.txt',
#      'D:/Projects/TUe/optimizationnscheduling/HCSS_windowed/experiments/lowerbounds/*/*'
#     )    
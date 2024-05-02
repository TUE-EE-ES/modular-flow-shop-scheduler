#!/usr/bin/env python
import os, sys, time
import glob
import pandas 
import numpy as np
from verify import verify

def main(args): 
    if(len(args)) != 3:
        raise Exception("Incompatible number of arguments; expected exactly 2 arguments")

    cplex_solutions_matcher = os.path.normpath(args[1])
    lb_output_directory = os.path.normpath(args[2])
    
    res = {}
    other = {}
    
    mks = None
    for d in glob.glob(cplex_solutions_matcher):
        if not os.path.isdir(d):
            category, case = os.path.split(d)
            category = os.path.split(category)[1]
            case = os.path.splitext(case)[0]
            if category not in res:
                res[category] = {}
            lb = -1
            optimal = False
            solution_found = True
            with open(os.path.join(os.path.split(d)[0], case + ".lp.log")) as log_file:
                for line in log_file:
                    if "Time limit exceeded, no integer solution" in line:
                        print case, "- no solution found"
                        solution_found = False
                        optimal = False
                        lb = float("NaN")
        
                    if "Current MIP best bound" in line:
                        if not "is infinite" in line:
                            lb = float(line.split('=')[1].split(' ')[2])/1e6
                        
                    if "MIP - Integer infeasible" in line:
                        solution_found = False
                        lb = float("NaN")
                        optimal = "Infeasible"
                        
                    if "MIP - Integer optimal" in line:
                        lb = float(line.split('=')[1].strip())/1e6
                        if "tolerance" not in line:
                            optimal = True
                        else:
                            optimal = "Within 0.01% Tolerance"

            
            res[category][case] = {'lowerbound' : lb, 'solutionfound' : solution_found, "optimal" : optimal}
            other[case] = {'category' : category, 'lowerbound' : lb, 'solutionfound' : solution_found, "optimal" : optimal}
            
            
    for category in res:
        cat = res[category]
        print len(cat), "cases in category", category
        for case in cat:
            print " ", case, cat[case]
            
    
    df = pandas.DataFrame.from_dict(other, orient="index")
    infeasible = df[df['optimal'] == 'Infeasible']
    if len(infeasible.index):
        print "Infeasible cases detected! Model is not correct"
        print infeasible
    df.to_csv(lb_output_directory + "/" + "lowerbounds.txt", '\t')
    print df
    #print df[df['solutionfound'] == False]
if __name__ == '__main__':
    main(sys.argv)

from __future__ import print_function
from intervaltree import Interval, IntervalTree
import xml.etree.ElementTree as ET
import sys

def extract_timings(root):
    pTimesNode = root.find('processingTimes')
    sTimesNode = root.find('setupTimes')
    dTimesNode = root.find('relativeDueDates')

    # build up explicit representation of processingtimes, setuptimes and duedates
    defaultProcTime = int(pTimesNode.attrib['default'])
    defaultSetupTime = int(sTimesNode.attrib['default'])
    defaultDueDate = int(dTimesNode.attrib['default']) if dTimesNode.attrib['default'] != 'inf' else float('inf')

    nOps = int(root.find('jobs').find('operations').attrib['count'])
    nJobs = int(root.find('jobs').attrib['count'])

    processingTimes = [[defaultProcTime for i in range(nOps)] for x in range(nJobs)]
    setupTimes = {}
    dueDates = {}

    
    # potentially use maps instead of arrays for this, as the matrix is sparse
    for child in pTimesNode:
        j = int(child.attrib['j'])
        op = int(child.attrib['op'])
        t = int(float(child.attrib['value']))
        processingTimes[j][op] = t
    
    for child in sTimesNode:
        j1 = int(child.attrib['j1'])
        op1 = int(child.attrib['op1'])
        j2 = int(child.attrib['j2'])
        op2 = int(child.attrib['op2'])
        t = int(float(child.attrib['value']))
        if (j1,op1) not in setupTimes:
            setupTimes[(j1, op1)] = {}
        setupTimes[(j1,op1)][(j2,op2)] = t
    
    for child in dTimesNode:
        j1 = int(child.attrib['j1'])
        op1 = int(child.attrib['op1'])
        j2 = int(child.attrib['j2'])
        op2 = int(child.attrib['op2'])
        t = int(float(child.attrib['value']))
        if (j1,op1) not in dueDates:
            dueDates[(j1, op1)] = {}
        dueDates[(j1,op1)][(j2,op2)] = t
        
    return processingTimes, setupTimes, dueDates, defaultSetupTime, defaultDueDate 

def verify(input, asapst):    
    root = ET.parse(input).getroot()
    asapst = convert_asapst(asapst)
    flowVector = [int(child.attrib['value']) for child in root.find('flowVector')] # which operation is mapped onto which machine

    
    
    processingTimes, setupTimes, dueDates, defaultSetupTime, defaultDueDate  = extract_timings(root)
    
    if len(asapst) != len(processingTimes):
        raise Exception("ASAPST not compatible with XML; incorrect number of jobs")
    if len(asapst[0]) != len(processingTimes[0]):
        raise Exception("ASAPST not compatible with XML; incorrect number of operations per job")
        
    overlapping_verified = verify_not_overlapping(root, processingTimes, asapst, flowVector)
    timing_verified = verify_timing_requirements(root, processingTimes, setupTimes, dueDates, asapst, flowVector, defaultSetupTime, defaultDueDate)
    return overlapping_verified and timing_verified
    
def verify_timing_requirements(root, processingTimes, setupTimes, dueDates, asapst, flowVector, defaultSetupTime, defaultDueDate):
    # for each re-entrant machine, check which operations are now following each other
    # for each of them, check if the difference in asapst time is more than the proc + setup time
    # and check that each deadline is satisfied

    valid = True
    
    # first extract the sequence in which the operations on each machine are executed
    operationsOnMachine = {}
    
    sequence = {}
    for i, j in enumerate(flowVector):
        if j not in operationsOnMachine:
            operationsOnMachine[j] = []
        operationsOnMachine[j].append(i)
   
    for machine in operationsOnMachine:
        sequence[machine] = []
        # extract the sequence of operations on this machine, in terms of job, op combination
        current_job = {}
        for op in operationsOnMachine[machine]:
            current_job[op] = 0
    
        
        while True:
            done = True
            for op in operationsOnMachine[machine]:
                if current_job[op] < len(processingTimes):
                    done = False
                    
            if done:
                break
            
            times = {}
            
            for op in operationsOnMachine[machine]:
                if(current_job[op] < len(asapst)):
                    times[(current_job[op], op,)] = asapst[current_job[op]][op]

            job, min_op = min(times, key=times.get)
            sequence[machine].append((job, min_op))
            current_job[min_op] = current_job[min_op] + 1
        
    
    # for each operation:
    for i, processingTime in enumerate(processingTimes):
        for o, start_time in enumerate(processingTime):
            # check due dates
            if (i,o) in dueDates:
                for job, op in dueDates[(i, o)]:
                    if asapst[job][op] - asapst[i][o] > dueDates[(i, o)][(job, op)]:
                        valid = False
                        print("Deadline violated: time from {0} to {1} is {2} and should be at most {3}".format((i,o), (job,op), asapst[job][op] - asapst[i][o], dueDates[(i, o)][(job, op)]) )
            # check inter-job setup times:
            if (i,o) in setupTimes:
                if (i, o+1) in setupTimes[(i, o)]:
                    if asapst[i][o+1] - asapst[i][o] < setupTimes[(i, o)][(i, o+1)]:
                        valid = False
                        print("Setup time violated: time from {0} to {1} is {2} and should be at least {3}".format((i,o), (i,o+1), asapst[i][o+1] - asapst[i][o], setupTimes[(i, o)][(i, o+1)]) )
    
    # for each machine:
    for machine in operationsOnMachine:
        s = sequence[machine]
        # check the sequence dependent setup times
        # by checking if the asapst is at least <proc time + setup time> away 
        # from the previous operation
        for (i, o), (j, p) in zip(s[:-2], s[1:]):
            
            t = processingTimes[i][o] 
            if (i, o) in setupTimes and (j, p) in setupTimes[(i, o)]:
                t = t + setupTimes[(i, o)][(j, p)]
            else:
                t = t + defaultSetupTime
            if asapst[j][p] - asapst[i][o] < t:
                valid = False
                print("setup time violated!: processing time + setup time from {0} to {1} is {2} and should be at least {3}".format((i,o), (j,p), asapst[j][p] - asapst[i][o], t))
        
    return valid

def convert_asapst(asapst):
    result = []
    with open(asapst) as f:
        for line in f:
            start_times = [int(x.split('.')[0].strip()) for x in line.split('\t')]
            result.append(start_times)
            
    return result
    
def verify_not_overlapping(root, processingTimes, asapst, flowVector):
    
    interval_tree = [IntervalTree() for i in set(flowVector)]

    # build the interval tree
    for i, start_times in enumerate(asapst):
        for op, start_time in enumerate(start_times):
            if(processingTimes[i][op] != 0): # interval tree needs a non-zero interval size
                interval_tree[flowVector[op]][start_time:start_time + processingTimes[i][op]] = ("Job", str(i), "Op", str(op),)
        
    valid = True
    # check each start time
    for i, start_times in enumerate(asapst):
        for op, start_time in enumerate(start_times):
            if(processingTimes[i][op] != 0): # interval tree needs a non-zero interval size
                overlaps = interval_tree[flowVector[op]].overlap(start_time, start_time + processingTimes[i][op])
                if len(overlaps) != 1:
                    print("Overlapping:")
                    for overlap in overlaps:
                        j = overlap[2]
                        print("  Job", j[1], "\top", j[3], "\t->", overlap[0], "-", overlap[1])
                    valid = False
            
    return valid


if __name__ == "__main__":
    print(verify(*sys.argv[1:]))
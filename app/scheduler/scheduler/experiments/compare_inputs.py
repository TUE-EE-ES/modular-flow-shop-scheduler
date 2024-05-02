import sys

def load_input(filename):
    input = {}
    for line in open(filename):
        if line.find('label="[') != -1:
            edge, value = line.strip().split('label="[')
            edge = [int(x) for x in edge.replace('[color="red",', '').strip('[\'').split('->')]
            value = int(value.strip('\"];'))
            
            if edge[0] not in input:
                input[edge[0]] = {}
            input[edge[0]][edge[1]] = value
    return input
    
def compare(input1, input2):

    equal = True
    for a in input1:
        for b in input1[a]:
            if a in input2 and b in input2[a]:
                equal = equal and input1[a][b] == input2[a][b]
                if(input1[a][b] != input2[a][b]):
                    print a,"->",b, 'not equal', input1[a][b], input2[a][b]
            else:
                print a,"->",b, 'in input 1, but not in input 2' 
                
    for a in input2:
        for b in input2[a]:
            if a in input1 and b in input1[a]:
                equal = equal and input1[a][b] == input2[a][b]
            else:
                print a,"->",b, 'in input 2, but not in input 1' 
            
    return equal
    
def main(filename1, filename2):
    input1 = load_input(filename1)
    input2 = load_input(filename2)
    return compare(input1, input2)
    
if __name__ == "__main__":
    print("equal input" if main(*sys.argv[1:]) else "NOT equal input")
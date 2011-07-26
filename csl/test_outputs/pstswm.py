# import the python contraint module
from constraint import *

pstswm=Problem()


# parameter Declarations
class p:
    default=4
    def values(self):
        ls=[]
        ls=range(1, 16, 1)
        return ls

pstswm.addVariable("p",p().values())
class q:
    default=4
    def values(self):
        ls=[]
        ls=range(1, 16, 1)
        return ls

pstswm.addVariable("q",q().values())
class FTOPT:
    default="distriuted"
    def values(self):
        ls=[]
        ls=[ "distributed", "single_transpose", "double_transpose" ]
        return ls

pstswm.addVariable("FTOPT",FTOPT().values())
class LTOPT:
    default="distributed"
    def values(self):
        ls=[]
        ls=[ "distributed", "transpose_based" ]
        return ls

pstswm.addVariable("LTOPT",LTOPT().values())

# Constraint Declarations
def pq (p,q):
    return ((p * q) == 16)


def ftLT (FTOPT,LTOPT):
    return ((FTOPT == "double_transpose") or ((FTOPT == "double_transpose") and (LTOPT == "distributed")))


# Specification
def specification (FTOPT,p,LTOPT,q):
    return (pq(p, q) and ftLT(FTOPT, LTOPT))

pstswm.addConstraint(FunctionConstraint(specification), \
("FTOPT","p","LTOPT","q"))


# format the output and print solutions
solution = pstswm.getSolution()

# print the label
keys=solution.keys()
keyLabel = ''
for key in keys:
    keyLabel +=  key + '\t'

print keyLabel

solutions=pstswm.getSolutions()

for solution in solutions:
    oneLine = ''
    for key in keys:
        oneLine +=  str(solution[key]) + '\t'
    print oneLine

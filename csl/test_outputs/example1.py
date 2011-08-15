# import the python contraint module
from constraint import *

simple=Problem()


# parameter Declarations
class x:
    def values(self):
        ls=[]
        ls=range(1, 8, 1)
        return ls

simple.addVariable("x",x().values())
class y:
    def values(self):
        ls=[]
        ls=range(1, 8, 1)
        return ls

simple.addVariable("y",y().values())
class z:
    def values(self):
        ls=[]
        ls=range(1, 8, 1)
        return ls

simple.addVariable("z",z().values())

# Constraint Declarations
def cone (z,x):
    return ((x + z) >= z)


def ctwo (z,y):
    return (y > z)


# Specification
def specification (z,y,x):
    return (cone(z, x) and ctwo(z, y))

simple.addConstraint(FunctionConstraint(specification), \
("z","y","x"))


# format the output and print solutions
solution = simple.getSolution()

# print the label
keys=solution.keys()
keyLabel = ''
for key in keys:
    keyLabel +=  key + '\t'

print keyLabel

solutions=simple.getSolutions()

for solution in solutions:
    oneLine = ''
    for key in keys:
        oneLine +=  str(solution[key]) + '\t'
    print oneLine

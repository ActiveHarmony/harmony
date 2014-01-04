# import the python contraint module
from constraint import *

tiling_mm=Problem()


# Constant Declarations

# type:: int
l1_cache=128


# type:: int
l2_cache=4096


# type:: int
register_file_size=16

# Code Region Declarations

# code region loopI is defined

# code region loopJ is defined

# code region loopK is defined
# Region Set Declarations

# regionSet loop is defined
#  elems: [loopI, loopJ, loopK]

# parameter Declarations
class tile:
    default=32
    def values(self):
        ls=[]
        for i in range(1,8):
            ls.append(pow(2,i))
        return ls

# Region Set association: loop
tiling_mm.addVariable("loopI_tile", tile().values())
tiling_mm.addVariable("loopJ_tile", tile().values())
tiling_mm.addVariable("loopK_tile", tile().values())

class unroll:
    default=1
    def values(self):
        ls=[]
        ls=range(1, 8, 2)
        return ls

# Region Set association: loop
tiling_mm.addVariable("loopI_unroll", unroll().values())
tiling_mm.addVariable("loopJ_unroll", unroll().values())
tiling_mm.addVariable("loopK_unroll", unroll().values())


# Constraint Declarations
def mm_l1 (loopK_tile,loopJ_tile):
    return ((loopK_tile * loopJ_tile) <= ((l1_cache * 1024) / 16))


def mm_l2 (loopI_tile,loopK_tile):
    return ((loopK_tile * loopI_tile) <= ((l2_cache * 1024) / 16))


def mm_unroll (loopK_unroll,loopI_unroll,loopJ_unroll):
    return (((loopI_unroll * loopJ_unroll) * loopK_unroll) <= register_file_size)


# Specification
def specification (loopI_tile,loopK_tile,loopK_unroll,loopI_unroll,loopJ_tile,loopJ_unroll):
    return ((mm_l1(loopK_tile, loopJ_tile) and mm_l2(loopI_tile, loopK_tile)) and mm_unroll(loopK_unroll, loopI_unroll, loopJ_unroll))

tiling_mm.addConstraint(FunctionConstraint(specification), \
("loopI_tile","loopK_tile","loopK_unroll","loopI_unroll","loopJ_tile","loopJ_unroll"))


# format the output and print solutions
solution = tiling_mm.getSolution()

# print the label
keys=solution.keys()
keyLabel = ''
for key in keys:
    keyLabel +=  key + '\t'

print keyLabel

solutions=tiling_mm.getSolutions()

for solution in solutions:
    oneLine = ''
    for key in keys:
        oneLine +=  str(solution[key]) + '\t'
    print oneLine



i=$1
j=$2
k=$3


echo "

    source: rmatmult3_at.spd
    procedure: 0
    loop: 0
    
    UI=$i
    UJ=$j
    UK=$k
    

    known(kmax>=3)
    known(imax>=3)
    known(jmax>=3)
    permute([1,2,3])

    tile(0,2,20)
    tile(0,2,20)
    tile(0,5,20)
    #print
    
    # with all three loops unrolled
    unroll(0,4,UK)
    unroll(0,5,UJ)
    unroll(0,6,UI)    

    # with only UJ and UI
    #unroll(0,5,UJ)
    #unroll(0,6,UI)

    

    print
" > temp.script;
./chill.v.0.1.8 temp.script;
./s2c.v.0.1.8 rmatmult3_at.lxf > rmatmult3_at_modified.c;
GCC_COMMAND="gcc -O2 -march=pentium4 -mmmx -msse -msse2 -mfpmath=sse ";


$GCC_COMMAND -c -fpic rmatmult3_at_modified.c;
#$GCC_COMMAND -shared -lc -o  $out_file  rmatmult3_at_modified.o;
du -b rmatmult3_at_modified.o > du_${i}_${j}_${k};

#out_file=rmatmult3_${i}_${j}_${k}.so;

#$GCC_COMMAND -shared -lc -o  $out_file  rmatmult3_at_modified.o;


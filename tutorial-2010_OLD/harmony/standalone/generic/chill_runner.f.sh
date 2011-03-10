scc -V -.spd $1.f
porky -forward-prop $1.spd $1.sp1
porky -dead-code $1.sp1 $1.sp2
./chill.v.0.1.8 $1.script
s2f $1.lxf > $1.modified.f 

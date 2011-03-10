scc -V -.spd gemm.f
porky -forward-prop gemm.spd gemm.sp1
porky -dead-code gemm.sp1 gemm.sp2
./chill.v.0.1.8 gemm.script
s2f gemm.lxf > gemm.modified.f 

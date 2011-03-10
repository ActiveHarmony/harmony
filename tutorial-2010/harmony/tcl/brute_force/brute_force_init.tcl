proc brute_force_init {appName} {
    
    global ${appName}_brute_force_bundles
    upvar #0 ${appName}_bundles bundles
    global ${appName}_simplex_time
    #set ${appName}_brute_force_bundles [lsort $bundles]
    set ${appName}_brute_force_bundles $bundles
    upvar #0 ${appName}_brute_force_bundles brutefb
    set ${appName}_simplex_time 0
    global ${appName}_search_done
    set ${appName}_search_done 0
    global int_max_value
    set int_max_value 2147483647
    global best_perf_so_far
    global best_coordinate_so_far
    set best_perf_so_far $int_max_value
    set best_coordinate_so_far {}

    global ${appName}_code_timestep
    set  ${appName}_code_timestep 1
    
    # code generation related parameters
    global ${appName}_code_generation_params
    set ${appName}_code_generation_params(generate_code) 0
    set ${appName}_code_generation_params(gen_method) 2

    # method 1 parameters
    set ${appName}_code_generation_params(cserver_host) "spoon"
    set ${appName}_code_generation_params(cserver_port) 2002
    set ${appName}_code_generation_params(cserver_connection) 0

    # method 2 parameters
    set ${appName}_code_generation_params(code_generation_destination) "rahulp@armour:/fs/armour/rahulp/scratch/confs/"

    foreach bun $brutefb {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun} $bun
        puts $bunv
        puts $minv
        set bunv $minv
        #recompute_dependencies $bun $appName
    }
    global ${appName}_simplex_time
    global ${appName}_time
    upvar #0 ${appName}_time time
    incr ${appName}_simplex_time

    #upvar #0 code_generation_params(cserver_connection) c_connection
    #upvar #0 code_generation_params(generate_code) g_code
    #upvar #0 code_generation_params(cserver_port) cserver_port
    #upvar #0 code_generation_params(cserver_host) cserver_host
    #upvar #0 code_generation_params(gen_method) gen_code_method
    #if { $g_code == 1 } {
	#    if { $gen_code_method == 1 } {
	#        send_candidate_simplex $appName
	#    } else {
	#        write_candidate_simplex $appName
	#    }
	#}

}

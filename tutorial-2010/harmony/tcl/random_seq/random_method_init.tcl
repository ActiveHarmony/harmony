proc random_init { appName } {




    puts ":::: appname from init: $appName"




    global ${appName}_code_timestep
    set ${appName}_code_timestep 1
    global ${appName}_search_done
    set ${appName}_search_done 0
    global int_max_value
    set int_max_value 2147483647
    global ${appName}_best_perf_so_far
    global ${appName}_best_coordinate_so_far
    set ${appName}_best_perf_so_far $int_max_value
    set ${appName}_best_coordinate_so_far {}

    global ${appName}_max_search_iterations
    set ${appName}_max_search_iterations 100

    global ${appName}_code_generation_params
    set ${appName}_code_generation_params(generate_code) 0 
    set ${appName}_code_generation_params(gen_method) 2

    # method 1 parameters
    set ${appName}_code_generation_params(cserver_host) "spoon"
    set ${appName}_code_generation_params(cserver_port) 2002
    set ${appName}_code_generation_params(cserver_connection) 0

    # method 2 parameters
    set ${appName}_code_generation_params(code_generation_destination) "tiwari@spoon:/fs/spoon/tiwari/scratch/confs/"

    upvar #0 ${appName}_bundles bundles
    global ${appName}_simplex_time
    set ${appName}_simplex_time 1
    foreach bun $bundles {
        upvar #0 ${appName}_bundle_${bun}(value) bunv
        upvar #0 ${appName}_bundle_${bun}(minv) minv
        upvar #0 ${appName}_bundle_${bun}(maxv) maxv
        upvar #0 ${appName}_bundle_${bun}(domain) domain
        upvar #0 ${appName}_bundle_${bun} $bun
        set bunv [random_uniform $minv $maxv 1]
        puts $bunv
    }
}

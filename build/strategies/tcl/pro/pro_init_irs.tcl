#
# Copyright 2003-2011 Jeffrey K. Hollingsworth
#
# This file is part of Active Harmony.
#
# Active Harmony is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Active Harmony is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
#
proc parallel_simplex_init {appName} {
    # initializes all the global variables
    # Assign co-efficients for the simplex reflection, expansion and contraction
    # set dimension for the search space

    global space_dimension

    # the step variable keeps track of the stages of multi-directional simplex
    #   method. when step=-1, initial complete simplex is constructed
    global pro_step

    # upper bound on the number of PRO iterations
    global pro_max_iterations

    # which initial simplex construction method?
    global initial_simplex_method

    # initial complete simplex method params
    global icsm_1_params
    global icsm_2_params
    global icsm_3_params
    global icsm_4_params
    global icsm_5_params
    global icsm_6_params

    #ANN tree parameters
    global ann_params

    # code generation parameters
    global code_generation_params

    # space exploration parameters
    global space_explore_params

    # do projection?
    global do_projection

    # which projection?
    # 1--> ANN
    # 2--> Project dim by dim to the nearest boundary
    global projection_method

    # points variable keeps track of the points in the current simplex
    global simplex_points

    # keep track of performances of all the points in the simplex
    global simplex_performance

    # keep track of performances of the points in the candidate simplex
    global candidate_performance

    # performance related variables: These variables are for experimental
    #  measurement purposes:

    # pro_step_cost: this keeps track of the cost (maximum time) of each PRO
    #  iteration
    global pro_step_cost

    # all_perfs: saves all perf reports that we have received so far.
    global simplex_all_perfs

    # this keeps track of how many pro iterations we have performed so far.
    global simplex_iteration

    # whenever we accept a change in the actual simplex we want to save the
    #  performance related to the points in the simplex. save_perfs variable
    #  is a boolean which determines whether we should/should not update the
    #  values in the simplex_performance variable.
    global simplex_save_perfs

    # to keep track of the experimental simplex. when we reach the ACCEPT
    #  state, the actual simplex will be replaced with this simplex
    global candidate_simplex

    # last_reflected_simplex is for a special case: if expansion of the best
    #  is not successful, we can resort to this saved simplex to accept
    #  reflection - saves us an extra iteration of PRO.
    global last_reflected_simplex
    global last_reflected_perf

    # npoints --> size of the simplex. This value cannot be
    #  more than the number of processors used to run the job
    global simplex_npoints

    # low variable keeps track of the index of "best" point in the simplex.
    #   Reflection, Expansion and Contraction are done w.r.t. to this point.
    global simplex_low
    global label_text

    # same for candidate simplex
    global candidate_low

    # co-efficients
    # reflection
    global simplex_alpha

    # shrink
    global simplex_beta

    # expansion
    global simplex_gamma
    global simplex_gamma_2

    # NOT USED YET. will need it in the future
    global ${appName}_simplex_time

    # this variable keeps track of reflection cost for the most current
    #  reflection step
    global simplex_reflection_cost

    # this keeps track of the INDEX of the best point that came out of the most
    #  current reflection. It provides an easier way to do expansion of the best
    #  point after reflection
    global simplex_reflection_minimumvertix

    # we need to keep track of the minimum value that we see during the
    #   reflection phase. this value will be later used in the expansion phase
    global simplex_reflection_minimum

    # was the reflection successful?
    global simplex_reflection_success

    # expansion of the best point was successful?
    global simplex_expansion_success

    # converged??
    global simplex_converged
    global curr_new_dir_trial
    global max_new_dir_trial
    global new_directions_method

    # we have three methods so far
    global ndm_1_params
    global ndm_2_params
    global ndm_3_params
    global ndm_4_params

    # When comparing two configurations, how much do they need to differ by to
    #  consider them as "different" performance
    global tolerance

    # {{perf index} ... {perf index}} sorted by perfs for current iteration
    global sorted_perfs

    # debug mode?
    global debug_mode

    global best_perf_so_far
    global best_coordinate_so_far
    global cand_best_coord
    global last_best_cand_perf
    global last_k_best
    global include_last_random_simplex
    global save_to_best_file
    global int_max_value
    set int_max_value 2147483647
    # Assignment of initial values for the global variables
    ###########################################################################
    # FIRST PART (standard params):: NO NEED TO CHANGE THESE PARAMS
    ###########################################################################
    set sorted_perfs {}
    set simplex_converged 0
    set simplex_expansion_cost -1
    set simplex_reflection_minimum -1
    set last_reflected_simplex {}
    set last_reflected_perf {}
    set simplex_step 0
    set include_last_random_simplex 0

    set simplex_points {}
    set candidate_simplex {}
    set simplex_low(index) -1
    set simplex_low(value) $int_max_value
    set candidate_low(index) -1
    set candidate_low(value) $int_max_value

    set simplex_alpha 2
    set simplex_beta 0.5
    set simplex_gamma 3
    set simplex_gamma_2 2

    set ${appName}_simplex_time -1
    set pro_step -1
    set simplex_save_perfs 1
    set simplex_iteration -1

    set simplex_performance {}
    set candidate_performance {}

    set simplex_reflection_cost {}
    set simplex_reflection_minimumvertix -1
    set simplex_reflection_success -1
    set simplex_expansion_success -1

    set next_iteration 0
    set expand_best 0
    set debug_mode 1

    set last_k_best [list]
    set best_perf_so_far  $int_max_value
    set last_best_cand_perf $int_max_value
    set best_coordinate_so_far [list]
    set cand_best_coord [list]
    set save_to_best_file 0
    ###########################################################
    # SECOND PART :: Search specific parameters
    ###########################################################
    set space_dimension 5
    set simplex_npoints 8 
    set pro_max_iterations 1000
    set tolerance 50

    # projection related methods
    #  method 1: contact the point server, which uses Approximate Nearest
    #            Neighbor (ANN) algorithm to find nearest points in high
    #            dimensions
    # method 2: project to boundary values
    set do_projection 1
    set projection_method 2

    # ANN RELATED :
    #   use_ann :: use ann nearest neighbor calculcation?
    #   num_independent dimensions: how many independent dimensions?
    #   group info :: if {3 2} : first three vars belong to one group and the
    #                            remaining 2 to the second group
    #   pserver_host : projection/points server hostname
    #   pserver_port : projection/points server port number
    #   connected : flag which specifies whether a connection to the projection
    #               server has been establish : need to make a connection at the
    #               start of the search algorithm

    set ann_params(use_ann) 0
    set ann_params(pserver_host) "brood00"
    set ann_params(pserver_port) 2077
    set ann_params(connected) 0

    # space exploration parameters
    set space_explore_params(num_iterations) 0
    set space_explore_params(random_step) 0
    set space_explore_params(best_perf) $int_max_value
    set space_explore_params(best_point) {}

    # initial simplex params
    set initial_simplex_method 4

    # method 1 params
    set icsm_1_params(init_point) {50 20 20 4 4}
    set icsm_1_params(init_distance) {4 3}
    set icsm_1_params(num_neighbors) 1000

    # method 2 params
    set icsm_2_params(init_point) {200 200 200 4}
    set icsm_2_params(scaling_vector) {80 30 80 2}
    set icsm_2_params(center) {0 0}

    # method 3 takes the same parameter as method 2
    # method 4 is a random simplex generation method
    set icsm_4_params(use_exploration_point) 0
    set icsm_4_params(filename) "random_simplex.tcl"
    set icsm_4_params(init_point) {30 30 30 4 4 }

    # method 5 reads the initial simplex from a file
    set icsm_5_params(filename) "init_simplex_from_file.tcl"

    set icsm_6_params(init_point) {30 30 30 4 4 }
    set icsm_6_params(init_distance) 20
    set icsm_6_params(use_exploration_point) 1
    set curr_new_dir_trial 1
    set max_new_dir_trial 5

    set new_directions_method 4

    # new directions params
    # method 1: uses ann to construct a new simplex around the current best
    #           point
    #           starting distance: a list, which must be of length num_groups
    #           curr_new_dir_distance : keeps track of how far we have looked
    #             so far. This is also a list and must be of length num_groups
    #           distance_step : how to increment or decrement the distance
    set ndm_1_params(starting_distance) {4 4 4 1 1}
    set ndm_1_params(curr_distance) {10 4}
    set ndm_1_params(distance_step) {1 1}

    set ndm_2_params(start_scale) 4
    set ndm_2_params(step) -0.25
    set ndm_2_params(scaling_vector) {4 4}

    set ndm_3_params(how_many) 10
    set ndm_3_params(next_index) 0

    set ndm_4_params(starting_distance) {4 4 4 1 1}
    set ndm_4_params(curr_distance) {2}
    set ndm_4_params(distance_step) {2}

    puts "Using IRS init file ::"

}

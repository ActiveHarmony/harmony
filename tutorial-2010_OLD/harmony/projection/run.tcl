load ./nearest_neighbor.so
projection_startup 


set test [do_projection "100 10 20 4 9"]
set test_2 [string_to_char_star $test]
puts $test_2

projection_end

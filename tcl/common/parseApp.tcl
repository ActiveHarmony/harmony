#
# Copyright 2003-2012 Jeffrey K. Hollingsworth
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

# we are going to have the following global variables:
# 
# AppName_socket_bundle_BundleName - array for each bundle containing the following
#			fields: type 
#			and according (int) min, max, step, value, domain (all the values from min to max)
#			(enum) nr_opt, opt1,... optk, value
#			it also contains a field called depend which is a
#			list of bundles that depend on the value of the
#			current bundle
#
# AppName_socket_bundles - list with all bundles
#
# AppName_socket_nodes - list with all the nodes
#
# AppName_socket_node_NodeName - array for each bundle containing all the fields
#			as described in the RSL specification:
#			hostname, os, seconds, replicate, memory, etc.
#
# AppName_socket_time - the "counter" of events for the application

proc car {l} { return [lindex $l 0]}
proc cadr {l} { return [lindex $l 1]}
proc caddr {l} { return [lindex $l 2]}
proc cadddr {l} { return [lindex $l 3]}
proc caddddr {l} { return [lindex $l 4]}
proc cdr {l} { return [lrange $l 1 end]}
proc cddr {l} { return [lrange $l 2 end]}

# prints array
proc parr {a} {
    upvar #0 $a arr
    puts "$a"
    foreach tag [lsort [array names arr]] {
        puts "$tag: $arr($tag)";
    }
    puts ""
}


proc recompute_dependencies {name appName} {

  #increment the global time of the application
  global ${appName}_time
  incr ${appName}_time

  if {[string match "*bundle*" $name]} {
      computeBundle $name $appName
  } elseif "[string match "*node*" $name]" {
      computeNode $name $appName
  } else {
      computeOther $name $appName
  }
}

proc dependencies {type name appName} {

  upvar #0 $name arrayName
  upvar #0 ${appName}_bundles bundles
    
  switch $type {
    "enum" {
 	for {set i 1} {$i<=$arrayName(nr_opt)} {incr i 1} {
	    foreach bun $bundles {
		if [string match "*${bun}*" $arrayName(opt${i})] {
		    lappend ${appName}_bundle_${bun}(depend) $name
		}
	    }
	}
    }
    "real" -
    "int" {
	foreach bun $bundles {
	    if {[string match "*$bun*" $arrayName(min)] || 
	       [string match "*$bun*" $arrayName(max)] ||
	       [string match "*$bun*" $arrayName(step)] } {
		global ${appName}_bundle_${bun}
		lappend ${appName}_bundle_${bun}(depend) $name
	    }
	}
    }
    "node" {
	# this is for nodes
	foreach bun $bundles {
	    set matched 0
	    foreach tag [lsort [array names arrayName]] {
		if {[string match "*$bun*" $arrayName($tag)]} {
		  set matched 1
		}
	    }
	    if {$matched > 0} {
		global ${appName}_bundle_${bun}
		lappend ${appName}_bundle_${bun}(depend) $name
	    }
	}	    
    }
    "others" {
	#this is for other options: link, communication, etc.
	foreach bun $bundles {
	    if  {[string match "*$bun*" $arrayName(expression)]} {
		global  ${appName}_bundle_${bun}
		lappend ${appName}_bundle_${bun}(depend) $name
	    }
	}
    }
  }
}

proc harmonyApp {simple_name app_desc socket} {

  global current_appName
  set current_appName $simple_name
  set name ${simple_name}_${socket}

  #define global variable AppName_bundles
  global ${name}_bundles
  set ${name}_bundles {}

  #define global variable AppName_nodes
  global ${name}_nodes
  set ${name}_nodes {}

  #define global variable AppName_nodes
  global ${name}_others
  set ${name}_others {}

  #define global time for the AppName
  global ${name}_time
  set ${name}_time 0

  # go through each bundle and node description
  foreach obj $app_desc {
	set cmd [concat $obj $name]
        switch [car $obj] {
	    "node" -
	    "harmonyBundle" {
		eval $cmd
	    }
	    "link" {
		harmonyOther [concat [car $obj] "{[cadddr $obj]}" $name]
	    } 
            "obsGoodness" {
                eval $cmd
            }
	    "predGoodness" {
                eval $cmd
            }
 	    default {
		harmonyOther [concat $obj $name]
	    }
	}
  }	
  
  upvar #0 ${name}_bundles bundles
  puts "Bundles: $bundles"
  puts ""
  upvar #0 ${name}_nodes nodes
  puts "Nodes: $nodes"
  puts ""
  upvar #0 ${name}_others others
  puts "Others: $others"
  puts ""

  #print dependencies
  puts "Dependencies:"
  foreach bun $bundles {
      upvar #0 ${name}_bundle_${bun}(depend) depend
      puts "${name}_bundle_${bun}: $depend"
  }
  puts ""

  #drawharmonyApp $name
  
  # now we have to see if we created global bundles and draw them
  set aName [string range $name 0 [expr [string last "_" $name]-1]]
  global ${aName}_bundles
  global ${aName}_draw
  puts "Exists? ${aName}_bundles"
  puts "Exists? .application_${aName}"
  puts "Exists? [info exists ${aName}_bundles]"
  puts "Exists? [info exists ${aName}_draw]"
  
  #if {[info exists ${aName}_bundles] && ![info exists ${aName}_draw]} {
  #    drawharmonyApp $aName
  #}
  return $simple_name
}


proc computeOther {name appName} {
  #define global variable AppName_node_NodeName
  global ${name}

  upvar #0 ${appName}_bundles bundles
  upvar #0 ${appName}_nodes nodes

  #define some cool things
  foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(value) $bun
  }


  foreach node $nodes {
      global ${appName}_node_${node}
      foreach tag [lsort [array names ${appName}_node_${node}]] {
	  if  "[expr 1+[string last "v" $tag]]==[string length $tag]" {
	      set length [string length $tag]
	      set newtag [string range $tag 0 [expr $length-2]]

	      upvar #0 ${appName}_node_${node}($tag) tvalue
	      set ${node}($newtag) $tvalue
	  }
      }
  }



  upvar #0 ${name}(expression) tvalue
  set ${name}(value) [expr $tvalue]
}



proc harmonyOther {args} {
    
  #get the name of the application
  set appName [lindex [car $args] end]
  set name [car [car $args]]

 
  #update AppName_bundles global variables
  global ${appName}_others
  lappend ${appName}_others $name
  
  #define global variable AppName_node_NodeName
  global ${appName}_other_${name}

  upvar #0 ${appName}_bundles bundles

  puts "$appName ; $name"
  puts "[info exists $bundles]"

  #define some cool things ???
  # I think that you need the 
  foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(value) $bun
  }

  set ${appName}_other_${name}(expression) [lindex [car $args] 1]

  computeOther ${appName}_other_$name $appName

  dependencies "others" ${appName}_other_$name $appName  
}



proc computeBundle {name appName} {

  global ${name}
  upvar #0 ${name} arrayName

  upvar #0 ${appName}_bundles bundles
  #define values as global 
  foreach bundle $bundles {
      upvar #0 ${appName}_bundle_${bundle}(value) $bundle
  }


  upvar #0 ${name}(type) type
  switch $type {
      "enum" {
      }
      "real" -
      "int"  {
	  
	  upvar #0 ${name}(min) min    
	  upvar #0 ${name}(max) max    
	  upvar #0 ${name}(step) step
	  upvar #0 ${name}(value) tvalue

	  
	  set  ${name}(minv) [expr $min]
	  set  ${name}(maxv) [expr $max]
	  set  ${name}(stepv) [expr $step]
	  
	  if {$tvalue<[expr $min]} { 
	      set ${name}(value) [expr $min]
	  } else {
	      if {$tvalue>[expr $max]} { 
		  set ${name}(value) [expr $max]
	      }   }

	  upvar #0 ${name}(minv) minv
	  upvar #0 ${name}(maxv) maxv
	  upvar #0 ${name}(stepv) stepv

	  set ${name}(domain) {}
	  for {set i $minv} {$i<=$maxv} {set i [expr $i+$stepv]} {
	      lappend ${name}(domain) $i
	  }
      }
  }
}

proc harmonyBundle {name bundle_desc appName} {
  global ${appName}_bundle_${name}

  global ${appName}_bundles

  puts $${appName}_bundles

  lappend ${appName}_bundles $name
  upvar #0 ${appName}_bundles bundles

  #set dependencies as empty list
  set ${appName}_bundle_${name}(depend) {}

  #create array for bundle according to its type 
  switch [car $bundle_desc] {
	"enum" { 
	  set ${appName}_bundle_${name}(type) "enum"

	  set nv 0
       	  foreach value [cadr $bundle_desc] {
	    incr nv
	    set ${appName}_bundle_${name}(opt${nv}) $value	    
	  }
	  set ${appName}_bundle_${name}(nr_opt) $nv

	  # the value is the first of the options
	  global ${appName}_bundle_${name}
	  set ${appName}_bundle_${name}(value) [car [cadr $bundle_desc]]

	  #determine dependencies
	  dependencies "enum" ${appName}_bundle_${name} $appName
  	}
	
	"int" { 
	    set ${appName}_bundle_${name}(type) "int"
	    
	    set ${appName}_bundle_${name}(min) [car [cadr $bundle_desc]]
	    set ${appName}_bundle_${name}(max) [cadr [cadr $bundle_desc]]
	    set ${appName}_bundle_${name}(step) [caddr [cadr $bundle_desc]]
	    set ${appName}_bundle_${name}(ivalue) [cadddr [cadr $bundle_desc]]
	    set ${appName}_bundle_${name}(iscale) [caddddr [cadr $bundle_desc]]
	    
	    # only carry this out if the initial simplex method is defined to be
            #  "user_defined"
            upvar #0 init_simplex_method init_method
            if { [string equal $init_method "user_defined"] == 1 } {
                upvar #0 ${appName}_bundle_${name}(ivalue) ivalue
                # set the value of the initial point here.
                set ${appName}_bundle_${name}(value) $ivalue
            } else {
                # defaults to using the min value as one of the initial points.
                upvar #0 ${appName}_bundle_${name}(min) min
                set ${appName}_bundle_${name}(value) $min
            }
            

	    computeBundle "${appName}_bundle_${name}" $appName
  
	    #determine dependencies
	    dependencies "int" ${appName}_bundle_${name} $appName
	}
	
	"real" { 
	  set ${appName}_bundle_${name}(type) "real"

          set ${appName}_bundle_${name}(min) [car [cadr $bundle_desc]]
          set  ${appName}_bundle_${name}(max) [cadr [cadr $bundle_desc]]
          set  ${appName}_bundle_${name}(step) [caddr [cadr $bundle_desc]]

	    set ${appName}_bundle_${name}(value) 0

	    computeBundle ${appName}_bundle_${name} $appName

	  #determine dependencies
	  dependencies "real" ${appName}_bundle_${name} $appName
	}
  }

  #determine if the bundle is global or local
  if {[caddr $bundle_desc]=="global"} {
      #we have a global description
      
      global ${appName}_bundle_${name}
      set ${appName}_bundle_${name}(isglobal) 1

      #for this we need to create a new global bundle
      
      #first get the real name of the application without the socket number
      set aName [string range $appName 0 [expr [string last "_" $appName]-1]]
      puts "aName=$aName"

      #check if the bundle was already created
      puts "Check if bundle was created"
      global ${aName}_bundle_${name}
      if {[info exists ${aName}_bundle_${name}]} {
	  #do nothing here
	  #it means that we only need to set certain details
	  puts "Bundle was created. Do nothing"
      } else {
	  puts "We have to create bundle because info returned [info exists ${aName}_bundle_${name}] for ${aName}_bundle_${name}"
	  #next execute the normal procedure as above for the new bundle
	  harmonyBundle $name [list [car $bundle_desc] [cadr $bundle_desc]] $aName
      }

      #now we need to create some links between the global variable and 
      # the local one. Though I am not sure what would be the best way to do
      # this I will make the local variable dependent on the global one. 
      # Another issue that appears because of this is the ability of the
      # language to make dependencies beyond the barrier of an individual 
      # application.
      
      upvar \#0 ${aName}_bundle_${name}(deplocals) deplocals
      lappend deplocals ${appName}_bundle_${name}
      
      upvar \#0 ${aName}_bundle_${name}(isglobal) isglobl
      set isglobl -1

  } else {
      global ${appName}_bundle_${name}
      set ${appName}_bundle_${name}(isglobal) 0
  }
}

proc computeNode {name appName} {
  upvar #0 ${appName}_bundles bundles
  upvar #0 ${appName}_nodes nodes

  #define global variable AppName_node_NodeName
  global ${name}

  upvar #0 ${name} arrayName

  #define values as global 
  foreach bundle $bundles {
      upvar #0 ${appName}_bundle_${bundle}(value) $bundle
  }

  foreach node $nodes {
      global ${appName}_node_${node}
      foreach tag [lsort [array names ${appName}_node_${node}]] {
	  if  "[expr 1+[string last "v" $tag]]==[string length $tag]" {
	      set length [string length $tag]
	      set newtag [string range $tag 0 [expr $length-2]]

	      upvar #0 ${appName}_node_${node}($tag) tvalue
	      set ${node}($newtag) $tvalue
	  }
      }
  }


  foreach tag [lsort [array names arrayName]] {
      if "[expr 1+[string last "v" $tag]]!=[string length $tag]" {
	  switch $tag {
	      "seconds" -
	      "cycles" -
	      "memory" -
	      "replicate" {
		  set ${name}(${tag}v) [expr $arrayName($tag) ]
	      }   
	      default {
		  upvar #0 ${name}($tag) tvalue 

		  if {[string last "$" $tvalue]>=0} {
		      set ${name}(${tag}v) [expr ($tvalue)]
		  } else {
		      set ${name}(${tag}v) $arrayName($tag)
		  }
	      }
	  }   
      }   
  }

}

proc node {name args} {
  #get the name of the application
  set appName [lindex $args end]

  #define global variable AppName_node_NodeName
  global ${appName}_node_${name}

  #update AppName_bundles global variables
  global ${appName}_nodes
  lappend ${appName}_nodes $name

  upvar #0 ${appName}_bundles bundles

  #define some cool things
  foreach bun $bundles {
	upvar #0 ${appName}_bundle_${bun}(value) $bun
  }


  foreach opt $args {
    set ${appName}_node_${name}([car $opt]) [cadr $opt]
  }

  if "[lsearch [array names ${appName}_node_${name}] replicate]<0" {
      set ${appName}_node_${name}(replicate) "1"
  }

  unset ${appName}_node_${name}($appName)

  computeNode ${appName}_node_${name} $appName

  dependencies "node"  ${appName}_node_${name} $appName

}

proc case {args} {
  upvar appName aName
  upvar #0 ${aName}_nodes nodes

  foreach node $nodes {
      global ${aName}_node_${node}
      foreach tag [lsort [array names ${aName}_node_${node}]] {
	  if  "[expr 1+[string last "v" $tag]]==[string length $tag]" {
	      set length [string length $tag]
	      set newtag [string range $tag 0 [expr $length-2]]

	      upvar #0 ${aName}_node_${node}($tag) tvalue
	      set ${node}($newtag) $tvalue
	  }
      }
  }

    set bun_name [car $args]
    foreach option [cdr $args] {
	if {$bun_name==[car $option]} {
	    return [expr [cadr $option]] 
	}
    }
    return 0
}

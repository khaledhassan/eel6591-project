#                                   (parameter examples)
# $ns_ node-config -addressingType 	flat or hierarchical or expanded
#                  -adhocRouting   	DSDV or DSR or TORA
#                  -llType	   		LL
#                  -macType	   		Mac/802_11
#                  -propType	   	"Propagation/TwoRayGround"
#                  -ifqType	   		"Queue/DropTail/PriQueue"
#                  -ifqLen	   		50
#                  -phyType	   		"Phy/WirelessPhy"
#                  -antType	   		"Antenna/OmniAntenna"
#                  -channelType    	"Channel/WirelessChannel"
#                  -topoInstance   	$topo
#                  -energyModel    	"EnergyModel"
#                  -initialEnergy  	(in Joules)
#                  -rxPower        	(in W)
#                  -txPower        	(in W)
#                  -agentTrace     	ON or OFF
#                  -routerTrace    	ON or OFF
#                  -macTrace       	ON or OFF
#                  -movementTrace  	ON or OFF

# ======================================================================
# Define options
# ======================================================================
set val(chan)         Channel/WirelessChannel  ;# channel type
set val(prop)         Propagation/TwoRayGround ;# radio-propagation model
set val(netif)        Phy/WirelessPhy          ;# network interface type
set val(mac)          Mac/802_11               ;# MAC type
set val(ifq)          Queue/DropTail/PriQueue  ;# Interface queue type
set val(ll)           LL                       ;# Link layer type
set val(ant)          Antenna/OmniAntenna      ;# Antenna type

set opt(x)			1000		;# X dimension of the topography
set opt(y)			1000		;# Y dimension of the topography
set opt(cp)			"Traffic-file"
set opt(sc)			"gaussMax5.ns_movements"

set opt(seed)		3.0
set opt(stop)		50.0	;# simulation time

set val(ifqlen)       50                       ;# max packet in ifq
set opt(nn)           50                        ;# number of mobilenodes

set val(rp)           AODV                     ;# ad-hoc routing protocol 
set opt(tr)	      out.tr			;# tracefile
set opt(out)	      outnam.nam		;# nam outputfile

# create instance of simulator
set ns_    [new Simulator]
$ns_ use-newtrace 


###

proc usage { argv0 }  {
        puts "Usage: $argv0"
        puts "\tmandatory arguments:"
        puts "\t\t\[-x MAXX\] \[-y MAXY\]"
        puts "\toptional arguments:"
        puts "\t\t\[-cp conn pattern\] \[-sc scenario\] \[-nn nodes\]"
        puts "\t\t\[-seed seed\] \[-stop sec\] \[-out namOutput\] [-tr tracefile\]\n"
}

proc getopt {argc argv} {
        global opt
        lappend optlist cp nn seed sc stop tr x y

        for {set i 0} {$i < $argc} {incr i} {
                set arg [lindex $argv $i]
                if {[string range $arg 0 0] != "-"} continue

                set name [string range $arg 1 end]
                set opt($name) [lindex $argv [expr $i+1]]
        }
}


getopt $argc $argv


getopt $argc $argv

if { $opt(x) == 0 || $opt(y) == 0 } {
        usage $argv0
        exit 1
}

if {$opt(seed) > 0} {
        puts "Seeding Random number generator with $opt(seed)\n"
        ns-random $opt(seed)
}
# setup trace support
set namf [open $opt(out) w]
set tracefd     [open $opt(tr) w]
$ns_ namtrace-all-wireless $namf $opt(x) $opt(y)
$ns_ trace-all $tracefd  

# create topology
set topo	[new Topography]
$topo load_flatgrid $opt(x) $opt(y)


set god_ [create-god $opt(nn)]

# Configure nodes
$ns_ node-config -adhocRouting $val(rp) \
                 -llType $val(ll) \
                 -macType $val(mac) \
                 -ifqType $val(ifq) \
                 -ifqLen $val(ifqlen) \
                 -antType $val(ant) \
                 -propType $val(prop) \
                 -phyType $val(netif) \
                 -topoInstance $topo \
                 -channelType $val(chan) \
                 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
                 -movementTrace ON

for {set i 0} {$i < $opt(nn) } {incr i} {
    set node_($i) [$ns_ node]
    $node_($i) random-motion 0       ;# disable random motion
} 


for {set i 0} {$i < $opt(nn)} {incr i} {
    $node_($i) namattach $namf
# 20 defines the node size in nam, must adjust it according to your scenario
   $ns_ initial_node_pos $node_($i) 50
}


# 
# Define node movement model
#
puts "Loading connection pattern..."
source $opt(cp)

# 
# Define traffic model
#
puts "Loading scenario file..."
source $opt(sc)


#
# Tell nodes when the simulation ends
#
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).000000001 "$node_($i) reset";
}
# tell nam the simulation stop time


$ns_ at  $opt(stop).000000001 "puts \"NS EXITING...\" ; $ns_ halt"

proc stop {} {
    global ns_ tracefd
    close $tracefd
    close $namf
}
puts "Starting Simulation..."
$ns_ run

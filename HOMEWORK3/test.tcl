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

# create instance of simulator
set ns_    [new Simulator]
$ns_ use-newtrace 


# setup trace support
set tracefd     [open test.tr w]
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
}
puts "Starting Simulation..."
$ns_ run

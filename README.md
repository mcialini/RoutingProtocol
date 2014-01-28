RoutingProtocol
===============

Given a graph of 4 nodes with preset edge costs, this code will simulate a distance-vector routing algorithm in which each node communicates to its neighbors all of its current cheapest paths to every other node in the graph. Upon receiving one of these routing packets, a node will update its routing table with the new minimum costs it has seen. If a node uses another node x to reach node y, then the routing packets it sends out to x will have a cost of INFINITY, or 9999, to node y. This is the Split Horizon with Poison Reverse strategy which will eliminate multi-node routing loops.

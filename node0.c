#include <stdio.h>
#define INFINITY 9999
#define THISNODE 0

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
} rtpkt;

extern int TRACE;
extern double clocktime;

int lkcost0[4];		/*The link cost between node 0 and other nodes*/
struct distance_table 		/*Define distance table*/
{
  int costs[4][4];	/* where costs[i,j] stores the minimum distance to node i through node j */
} dt0;

struct routing_table
{
	int destination[4]; 
	int costs[4]; /* costs[i] stores cost of shortest path to i */
	int hop[4]; /* hop[i] stores next hop on shortest path to i from THISNODE */
} rt0;

struct distance_table DT0;	/* Global distance table for node 0 */
struct routing_table RT0; /* Global Routing Table for node 0 */

/* students to write the following two routines, and maybe some others */
void rtinit0() 
{
	printf("*********************************************************************\n");
	printf("*********************************************************************\n");
	printf("*******************  Network Simulator Started  *********************\n");
	printf("*********************************************************************\n");
	printf("*********************************************************************\n\n");
	printf("rtinit0 called at %f.\n",clocktime);
	
	int i,j;
	
	/* Initialize distance table */
	for (i=0;i<4;i++) 
		for (j=0;j<4;j++) 
			DT0.costs[i][j] = INFINITY;
	
	/* Add values that we currently know */
	lkcost0[0] = 0;
	lkcost0[1] = 1;
	lkcost0[2] = 10;
	lkcost0[3] = 7;
	
	for (i=0; i<4;i++) {
		DT0.costs[i][i] = lkcost0[i];
		RT0.destination[i] = i;
		RT0.costs[i] = lkcost0[i]; /* Initial minimum cost to each node is the direct link */
		RT0.hop[i] = i;
	}
	
	printf("\tInitial distance table:\n");
	printdt0(&DT0);
	printf("\tInitial routing table:\n");
	printrt0(&RT0);
	
	/* Advertise initial minimum costs to all neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost0[i] != INFINITY) {
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++)
				pkt.mincost[j] = RT0.costs[j];
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}


void rtupdate0(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
	printf("rtupdate0 called at %f.\n",clocktime);
	printf("\tRouting packet received from node %d.\n",rcvdpkt->sourceid);
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	int numUpdated = 0; /* Keeps track of how many entries in DT0 are changed */
	int minsUpdated = 0; /* Keeps track if the routing table has changed. If it hasn't, the node shouldn't send out routing packets. */
	int source = rcvdpkt->sourceid;	
	int newcost = 0; /* holder variable for mincost array indices in rtpkt */
	
	/* Check if cost to other nodes through source has changed, and update DT0 accordingly. */
	for(i=0;i<4;i++) {
		/* The total cost to get to node i via source is the link cost to source + rcvdpkt.mincost[i] */
		newcost = rcvdpkt->mincost[i] + lkcost0[source]; 
		if (newcost > INFINITY) 
			newcost = INFINITY;
		if (newcost != DT0.costs[i][source]) { /* If the distance to i via source has changed, update */
			DT0.costs[i][source] = newcost;	
			numUpdated++;
			/* Check if the min cost path to i has changed after receiving from source */
			/* Must completely recalculate and not just compare against previous min in case link cost has changed */
			for (j=0;j<4;j++) {
				if (DT0.costs[i][j] < min) {
					min = DT0.costs[i][j];
					hop = j;
				}
			}
			if (RT0.costs[i] != min)
				minsUpdated++;
			RT0.costs[i] = min;
			RT0.hop[i] = hop;
			min = INFINITY;
			hop = THISNODE;
		}
	}
	
	
	printf("\t%d changes made to distance table:\n",numUpdated);
	printdt0(&DT0);
	printf("\t Updated routing table:\n");
	printrt0(&RT0);
	
	if (minsUpdated) {
		for (i=0;i<4;i++) {
			if (i != THISNODE && lkcost0[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
				struct rtpkt pkt;
				pkt.sourceid = THISNODE;
				pkt.destid = i;
				/* POISON REVERSE: if the min cost to some node x is through node i, the advertised min cost to x should be INFINITY */
				for (j=0;j<4;j++) {
					if (RT0.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
						pkt.mincost[j] = INFINITY;
					else 
						pkt.mincost[j] = RT0.costs[j];
				}		
				
				tolayer2(pkt);
				printf("\tRouting packet sent to node %d:\n",pkt.destid);
				printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
			}
		}
	}
	
}

printrt0(rtptr)
  struct routing_table *rtptr;
{
  printf("\t           RT0          \n");
  printf("\t  Dest | Cost | Next Hop\n");
  printf("\t  -----|------|---------\n");
  printf("\t     1 |  %3d |  %3d    \n",rtptr->costs[1], rtptr->hop[1]);
  printf("\t     2 |  %3d |  %3d    \n",rtptr->costs[2], rtptr->hop[2]);
  printf("\t     3 |  %3d |  %3d    \n",rtptr->costs[3], rtptr->hop[3]);
}

printdt0(dtptr)
  struct distance_table *dtptr;
{
  printf("\t                via     \n");
  printf("\t   D0 |    1     2    3 \n");
  printf("\t  ----|-----------------\n");
  printf("\t     1|  %3d   %3d   %3d\n",dtptr->costs[1][1], dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("\tdest 2|  %3d   %3d   %3d\n",dtptr->costs[2][1], dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("\t     3|  %3d   %3d   %3d\n",dtptr->costs[3][1], dtptr->costs[3][2],dtptr->costs[3][3]);
}


/* called when cost from 0 to linkid changes from current value to newcost*/
linkhandler0(linkid, newcost)   
  int linkid, newcost;
{
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	
	printf("linkhandler0 called at %f.\n",clocktime);
	printf("\tCost of link between %d and %d is now %d.\n",THISNODE,linkid,newcost);
	
	for (i=0;i<4;i++) {
		/* Subtract previous cost of the link netween this node and linkid, then add the newcost */
		DT0.costs[i][linkid] = DT0.costs[i][linkid] - lkcost0[linkid] + newcost;
	}
	lkcost0[linkid] = newcost;		
	
	/* Recompute the new costs of the shortest paths to each node */
	for (i=0;i<4;i++) {
		/* Find the lowest cost in each row */
		for (j=0;j<4;j++) {
			if (DT0.costs[i][j] < min ) {
				min = DT0.costs[i][j];
				hop = j;
			}
		}
		RT0.costs[i] = min;
		RT0.hop[i] = hop;
		min = INFINITY;
		hop = THISNODE;
	}
	
	printf("\tUpdated distance table:\n");
	printdt0(&DT0);
	printf("\t Updated routing table:\n");
	printrt0(&RT0);
	
	/* Send out routing packets to neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost0[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++) {
				if (RT0.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
					pkt.mincost[j] = INFINITY;
				else 
					pkt.mincost[j] = RT0.costs[j];
			}		
					
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}

void
getfinal0(void)
{
	printf("\n**********************************************\n");
	printf("                FINAL RESULTS\n");
	printf("**********************************************\n\n");
	printf("\tFinal distance table for node %d:\n",THISNODE);
	printdt0(&DT0);
	printf("\tFinal routing table for node %d:\n",THISNODE);
	printrt0(&RT0);
	printf("\n");
}

#include <stdio.h>
#define INFINITY 9999
#define THISNODE 1

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
} rtpkt;

extern int TRACE;
extern double clocktime;

int lkcost1[4];		/*the link cost between node 1 and other nodes*/
struct distance_table /*define distance table*/
{
  int costs[4][4];
} dt1;

struct routing_table
{
	int destination[4]; 
	int costs[4]; /* costs[i] stores cost of shortest path to i */
	int hop[4]; /* hop[i] stores next hop on shortest path to i from THISNODE */
} rt1;

struct distance_table DT1;	/* Global distance table for node 1 */
struct routing_table RT1; /* Global Routing Table for node 1 */

/* students to write the following two routines, and maybe some others */
void rtinit1() 
{
	printf("rtinit1 called at %f.\n",clocktime);
	
	int i,j;
	
	/* Initialize distance table */
	for (i=0;i<4;i++) 
		for (j=0;j<4;j++) 
			DT1.costs[i][j] = INFINITY;
	
	/* Add values that we currently know */
	lkcost1[0] = 1;
	lkcost1[1] = 0;
	lkcost1[2] = 1;
	lkcost1[3] = INFINITY;
	
	for (i=0; i<4;i++) {
		DT1.costs[i][i] = lkcost1[i];
		RT1.destination[i] = i;
		RT1.costs[i] = lkcost1[i]; /* Initial minimum cost to each node is the direct link */
		RT1.hop[i] = i;
	}
	
	printf("\tInitial distance table:\n");
	printdt1(&DT1);
	printf("\tInitial routing table:\n");
	printrt1(&RT1);
	
	/* Advertise initial minimum costs to all neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost1[i] != INFINITY) {
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++)
				pkt.mincost[j] = RT1.costs[j];
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}


void rtupdate1(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
	printf("rtupdate1 called at %f.\n",clocktime);
	printf("\tRouting packet received from node %d.\n",rcvdpkt->sourceid);
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	int numUpdated = 0; /* Keeps track of how many entries in DT1 are changed */
	int minsUpdated = 0;
	int source = rcvdpkt->sourceid;	
	int newcost = 0; /* holder variable for mincost array indices in rtpkt */
	
	/* Check if cost to other nodes through source has changed, and update DT1 accordingly. */
	for(i=0;i<4;i++) {
		/* The total cost to get to node i via source is the link cost to source + rcvdpkt.mincost[i] */
		newcost = rcvdpkt->mincost[i] + lkcost1[source]; 
		if (newcost > INFINITY) 
			newcost = INFINITY;
		if (newcost != DT1.costs[i][source]) { /* If the distance to i via source has changed, update */
			DT1.costs[i][source] = newcost;	
			numUpdated++;
			/* Check if the min cost path to i has changed after receiving from source */
			/* Must completely recalculate and not just compare against previous min in case link cost has changed */
			for (j=0;j<4;j++) {
				if (DT1.costs[i][j] < min) {
					min = DT1.costs[i][j];
					hop = j;
				}
			}
			if (RT1.costs[i] != min)
				minsUpdated++;
			RT1.costs[i] = min;
			RT1.hop[i] = hop;
			min = INFINITY;
			hop = THISNODE;
		}
	}
	
	
	printf("\t%d changes made to distance table:\n",numUpdated);
	printdt1(&DT1);
	printf("\t Updated routing table:\n");
	printrt1(&RT1);
	
	if (minsUpdated) {
		for (i=0;i<4;i++) {
			if (i != THISNODE && lkcost1[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
				struct rtpkt pkt;
				pkt.sourceid = THISNODE;
				pkt.destid = i;
				/* POISON REVERSE: if the min cost to some node x is through node i, the advertised min cost to x should be INFINITY */
				for (j=0;j<4;j++) {
					if (RT1.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
						pkt.mincost[j] = INFINITY;
					else 
						pkt.mincost[j] = RT1.costs[j];
				}		
				
				tolayer2(pkt);
				printf("\tRouting packet sent to node %d:\n",pkt.destid);
				printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
			}
		}
	}
	
}

printdt1(dtptr)
  struct distance_table *dtptr;
{
  printf("\t             via     \n");
  printf("\t   D1 |    0     2 \n");
  printf("\t  ----|--------------\n");
  printf("\t     0|  %3d   %3d \n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("\tdest 2|  %3d   %3d \n",dtptr->costs[2][0], dtptr->costs[2][2]);
  printf("\t     3|  %3d   %3d \n",dtptr->costs[3][0], dtptr->costs[3][2]);
}

printrt1(rtptr)
  struct routing_table *rtptr;
{
  printf("\t           RT1          \n");
  printf("\t  Dest | Cost | Next Hop\n");
  printf("\t  -----|------|---------\n");
  printf("\t     0 |  %3d |  %3d    \n",rtptr->costs[0], rtptr->hop[0]);
  printf("\t     2 |  %3d |  %3d    \n",rtptr->costs[2], rtptr->hop[2]);
  printf("\t     3 |  %3d |  %3d    \n",rtptr->costs[3], rtptr->hop[3]);
}


/* called when cost from 0 to linkid changes from current value to newcost*/
linkhandler1(linkid, newcost)   
  int linkid, newcost;
{
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	
	printf("linkhandler1 called at %f.\n",clocktime);
	printf("\tCost of link between %d and %d is now %d.\n",THISNODE,linkid,newcost);
	
	for (i=0;i<4;i++) {
		/* Subtract previous cost of the link netween this node and linkid, then add the newcost */
		DT1.costs[i][linkid] = DT1.costs[i][linkid] - lkcost1[linkid] + newcost;
	}
	lkcost1[linkid] = newcost;		
	
	/* Recompute the new costs of the shortest paths to each node */
	for (i=0;i<4;i++) {
		/* Find the lowest cost in each row */
		for (j=0;j<4;j++) {
			if (DT1.costs[i][j] < min ) {
				min = DT1.costs[i][j];
				hop = j;
			}
		}
		RT1.costs[i] = min;
		RT1.hop[i] = hop;
		min = INFINITY;
		hop = THISNODE;
	}
	
	printf("\tUpdated distance table:\n");
	printdt1(&DT1);
	printf("\t Updated routing table:\n");
	printrt1(&RT1);
	
	/* Send out routing packets to neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost1[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++) {
				if (RT1.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
					pkt.mincost[j] = INFINITY;
				else 
					pkt.mincost[j] = RT1.costs[j];
			}		
					
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}

void
getfinal1(void)
{
	printf("\tFinal distance table for node %d:\n",THISNODE);
	printdt1(&DT1);
	printf("\tFinal routing table for node %d:\n",THISNODE);
	printrt1(&RT1);
	printf("\n");
}
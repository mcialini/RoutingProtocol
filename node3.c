#include <stdio.h>
#define INFINITY 9999
#define THISNODE 3

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
} rtpkt;

extern int TRACE;
extern double clocktime;

int lkcost3[4];		/*the link cost between node 3 and other nodes*/
struct distance_table /*define distance table*/
{
  int costs[4][4];
} dt3;

struct routing_table
{
	int destination[4]; 
	int costs[4]; /* costs[i] stores cost of shortest path to i */
	int hop[4]; /* hop[i] stores next hop on shortest path to i from THISNODE */
} rt3;

struct distance_table DT3;	/* Global distance table for node 0 */
struct routing_table RT3; /* Global Routing Table for node 0 */

/* students to write the following two routines, and maybe some others */
void rtinit3() 
{
	printf("rtinit3 called at %f.\n",clocktime);
	
	int i,j;
	
	/* Initialize distance table */
	for (i=0;i<4;i++) 
		for (j=0;j<4;j++) 
			DT3.costs[i][j] = INFINITY;
	
	/* Add values that we currently know */
	lkcost3[0] = 7;
	lkcost3[1] = INFINITY;
	lkcost3[2] = 2;
	lkcost3[3] = 0;
	
	for (i=0; i<4;i++) {
		DT3.costs[i][i] = lkcost3[i];
		RT3.destination[i] = i;
		RT3.costs[i] = lkcost3[i]; /* Initial minimum cost to each node is the direct link */
		RT3.hop[i] = i;
	}
	
	printf("\tInitial distance table:\n");
	printdt3(&DT3);
	printf("\tInitial routing table:\n");
	printrt3(&RT3);
	
	/* Advertise initial minimum costs to all neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost3[i] != INFINITY) {
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++)
				pkt.mincost[j] = RT3.costs[j];
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}


void rtupdate3(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
	printf("rtupdate3 called at %f.\n",clocktime);
	printf("\tRouting packet received from node %d.\n",rcvdpkt->sourceid);
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	int numUpdated = 0; /* Keeps track of how many entries in DT3 are changed */
	int minsUpdated = 0;
	int source = rcvdpkt->sourceid;	
	int newcost = 0; /* holder variable for mincost array indices in rtpkt */
	
	/* Check if cost to other nodes through source has changed, and update DT3 accordingly. */
	for(i=0;i<4;i++) {
		/* The total cost to get to node i via source is the link cost to source + rcvdpkt.mincost[i] */
		newcost = rcvdpkt->mincost[i] + lkcost3[source]; 
		if (newcost > INFINITY) 
			newcost = INFINITY;
		if (newcost != DT3.costs[i][source]) { /* If the distance to i via source has changed, update */
			DT3.costs[i][source] = newcost;	
			numUpdated++;
			/* Check if the min cost path to i has changed after receiving from source */
			/* Must completely recalculate and not just compare against previous min in case link cost has changed */
			for (j=0;j<4;j++) {
				if (DT3.costs[i][j] < min) {
					min = DT3.costs[i][j];
					hop = j;
				}
			}
			if (RT3.costs[i] != min)
				minsUpdated++;
			RT3.costs[i] = min;
			RT3.hop[i] = hop;
			min = INFINITY;
			hop = THISNODE;
		}
	}
	
	
	printf("\t%d changes made to distance table:\n",numUpdated);
	printdt3(&DT3);
	printf("\t Updated routing table:\n");
	printrt3(&RT3);
	
	if (minsUpdated) {
		for (i=0;i<4;i++) {
			if (i != THISNODE && lkcost3[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
				struct rtpkt pkt;
				pkt.sourceid = THISNODE;
				pkt.destid = i;
				/* POISON REVERSE: if the min cost to some node x is through node i, the advertised min cost to x should be INFINITY */
				for (j=0;j<4;j++) {
					if (RT3.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
						pkt.mincost[j] = INFINITY;
					else 
						pkt.mincost[j] = RT3.costs[j];
				}		
				
				tolayer2(pkt);
				printf("\tRouting packet sent to node %d:\n",pkt.destid);
				printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
			}
		}
	}
	
}

printrt3(rtptr)
  struct routing_table *rtptr;
{
  printf("\t           RT3          \n");
  printf("\t  Dest | Cost | Next Hop\n");
  printf("\t  -----|------|---------\n");
  printf("\t     0 |  %3d |  %3d    \n",rtptr->costs[0], rtptr->hop[0]);
  printf("\t     1 |  %3d |  %3d    \n",rtptr->costs[1], rtptr->hop[1]);
  printf("\t     2 |  %3d |  %3d    \n",rtptr->costs[2], rtptr->hop[2]);
}


printdt3(dtptr)
  struct distance_table *dtptr;
{
  printf("\t             via     \n");
  printf("\t   D3 |    0     2 \n");
  printf("\t  ----|--------------\n");
  printf("\t     0|  %3d   %3d\n",dtptr->costs[0][0],dtptr->costs[0][2]);
  printf("\tdest 1|  %3d   %3d\n",dtptr->costs[1][0],dtptr->costs[1][2]);
  printf("\t     2|  %3d   %3d\n",dtptr->costs[2][0],dtptr->costs[2][2]);

}

/* called when Cost from  3 to linkid changes from current value to newcost*/
linkhandler3(linkid, newcost)   
  int linkid, newcost;
{

}

void
getfinal3(void)
{
	printf("\tFinal distance table for node %d:\n",THISNODE);
	printdt3(&DT3);
	printf("\tFinal routing table for node %d:\n",THISNODE);
	printrt3(&RT3);
	printf("\n");
}
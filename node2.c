#include <stdio.h>
#define INFINITY 9999
#define THISNODE 2

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
} rtpkt;

extern int TRACE;
extern double clocktime;

int lkcost2[4];		/*the link cost between node 2 and other nodes*/
struct distance_table /*define distance table*/
{
  int costs[4][4];
} dt2;

struct routing_table
{
	int destination[4]; 
	int costs[4]; /* costs[i] stores cost of shortest path to i */
	int hop[4]; /* hop[i] stores next hop on shortest path to i from THISNODE */
} rt2;

struct distance_table DT2;	/* Global distance table for node 2 */
struct routing_table RT2; /* Global Routing Table for node 2 */

/* students to write the following two routines, and maybe some others */
void rtinit2() 
{
	printf("rtinit2 called at %f.\n",clocktime);
	
	int i,j;
	
	/* Initialize distance table */
	for (i=0;i<4;i++) 
		for (j=0;j<4;j++) 
			DT2.costs[i][j] = INFINITY;
	
	/* Add values that we currently know */
	lkcost2[0] = 10;
	lkcost2[1] = 1;
	lkcost2[2] = 0;
	lkcost2[3] = 2;
	
	for (i=0; i<4;i++) {
		DT2.costs[i][i] = lkcost2[i];
		RT2.destination[i] = i;
		RT2.costs[i] = lkcost2[i]; /* Initial minimum cost to each node is the direct link */
		RT2.hop[i] = i;
	}
	
	printf("\tInitial distance table:\n");
	printdt2(&DT2);
	printf("\tInitial routing table:\n");
	printrt2(&RT2);
	
	/* Advertise initial minimum costs to all neighbors */
	for (i=0;i<4;i++) {
		if (i != THISNODE && lkcost2[i] != INFINITY) {
			struct rtpkt pkt;
			pkt.sourceid = THISNODE;
			pkt.destid = i;
			for (j=0;j<4;j++)
				pkt.mincost[j] = RT2.costs[j];
			tolayer2(pkt);
			printf("\tRouting packet sent to node %d:\n",pkt.destid);
			printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
		}
	}
}


void rtupdate2(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
	printf("rtupdate2 called at %f.\n",clocktime);
	printf("\tRouting packet received from node %d.\n",rcvdpkt->sourceid);
	int i,j;
	int min = INFINITY;
	int hop = THISNODE;
	int numUpdated = 0; /* Keeps track of how many entries in DT2 are changed */
	int minsUpdated = 0;
	int source = rcvdpkt->sourceid;	
	int newcost = 0; /* holder variable for mincost array indices in rtpkt */
	
	/* Check if cost to other nodes through source has changed, and update DT2 accordingly. */
	for(i=0;i<4;i++) {
		/* The total cost to get to node i via source is the link cost to source + rcvdpkt.mincost[i] */
		newcost = rcvdpkt->mincost[i] + lkcost2[source]; 
		if (newcost > INFINITY) 
			newcost = INFINITY;
		if (newcost != DT2.costs[i][source]) { /* If the distance to i via source has changed, update */
			DT2.costs[i][source] = newcost;	
			numUpdated++;
			/* Check if the min cost path to i has changed after receiving from source */
			/* Must completely recalculate and not just compare against previous min in case link cost has changed */
			for (j=0;j<4;j++) {
				if (DT2.costs[i][j] < min) {
					min = DT2.costs[i][j];
					hop = j;
				}
			}
			if (RT2.costs[i] != min)
				minsUpdated++;
			RT2.costs[i] = min;
			RT2.hop[i] = hop;
			min = INFINITY;
			hop = THISNODE;
		}
	}
	
	
	printf("\t%d changes made to distance table:\n",numUpdated);
	printdt2(&DT2);
	printf("\t Updated routing table:\n");
	printrt2(&RT2);
	
	if (minsUpdated) {
		for (i=0;i<4;i++) {
			if (i != THISNODE && lkcost2[i] != INFINITY) {	/* Nodes are neighbors, so send a routing packet */
				struct rtpkt pkt;
				pkt.sourceid = THISNODE;
				pkt.destid = i;
				/* POISON REVERSE: if the min cost to some node x is through node i, the advertised min cost to x should be INFINITY */
				for (j=0;j<4;j++) {
					if (RT2.hop[j] == i) /* Path to j is through i, so set cost to INFINITY */
						pkt.mincost[j] = INFINITY;
					else 
						pkt.mincost[j] = RT2.costs[j];
				}		
				
				tolayer2(pkt);
				printf("\tRouting packet sent to node %d:\n",pkt.destid);
				printf("\t[Min Costs to node0 = %d, node1 = %d, node2 = %d, node3 = %d]\n", pkt.mincost[0], pkt.mincost[1], pkt.mincost[2], pkt.mincost[3]);
			}
		}
	}
}

printrt2(rtptr)
  struct routing_table *rtptr;
{
  printf("\t           RT2          \n");
  printf("\t  Dest | Cost | Next Hop\n");
  printf("\t  -----|------|---------\n");
  printf("\t     0 |  %3d |  %3d    \n",rtptr->costs[0], rtptr->hop[0]);
  printf("\t     1 |  %3d |  %3d    \n",rtptr->costs[1], rtptr->hop[1]);
  printf("\t     3 |  %3d |  %3d    \n",rtptr->costs[3], rtptr->hop[3]);
}


printdt2(dtptr)
  struct distance_table *dtptr;  
{
  printf("\t                via     \n");
  printf("\t   D2 |    0     1    3 \n");
  printf("\t  ----|-----------------\n");
  printf("\t     0|  %3d   %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][1],dtptr->costs[0][3]);
  printf("\tdest 1|  %3d   %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][1],dtptr->costs[1][3]);
  printf("\t     3|  %3d   %3d   %3d\n",dtptr->costs[3][0], dtptr->costs[3][1],dtptr->costs[3][3]);
}


/* called when Cost from  2 to linkid changes from current value to newcost*/
linkhandler2(linkid, newcost)   
  int linkid, newcost;
{

}

void
getfinal2(void)
{
	printf("\tFinal distance table for node %d:\n",THISNODE);
	printdt2(&DT2);
	printf("\tFinal routing table for node %d:\n",THISNODE);
	printrt2(&RT2);
	printf("\n");
}
#include <stdlib.h>


#include "GridPos.h"



// returns true if path found
// outFullPathLength is grid spots count in path (including start and goal).
// if outFullPathLength = 0, start and goal equal, and outFullPath set to NULL
// outFullPath destroyed by caller if not NULL
// outFullPath contains full path including start and goal
char pathFind( int inMapH, int inMapW,
               char *inBlockedMap, 
               GridPos inStart, GridPos inGoal, 
               int *outFullPathLength,
               GridPos **outFullPath,
               // if not-NULL, set to closest reachable cooridinates to inGoal
               // this will be inGoal if path find is successful, or closest
               // possible if path finding fails
               GridPos *outClosest = NULL );



// find shortest path that also passes through inWaypoint
char pathFind( int inMapH, int inMapW,
               char *inBlockedMap, 
               GridPos inStart, GridPos inWaypoint, GridPos inGoal, 
               int *outFullPathLength,
               GridPos **outFullPath,
               // if not-NULL, set to closest reachable cooridinates to inGoal
               // this will be inGoal if path find is successful, or closest
               // possible if path finding fails
               GridPos *outClosest = NULL );


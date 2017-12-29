#include "pathFind.h"

#include <math.h>

#include <stdlib.h>


#include "minorGems/util/SimpleVector.h"






typedef struct pathSearchRecord {
        GridPos pos;
        
        int squareIndex;
        
        int cost;
        double estimate;
        double total;
        
        // index of pred in done queue
        int predIndex;
        
        // links to create structure of search queue
        pathSearchRecord *nextSearchRecord;

    } pathSearchRecord;


static double getGridDistance( GridPos inA, GridPos inB ) {
    int dX = inA.x - inB.x;
    int dY = inA.y - inB.y;
    
    // manhattan distance
    return fabs( dX ) + fabs( dY );
    //return sqrt( dX * dX + dY * dY );
    }


static char equal( GridPos inA, GridPos inB ) {
    return inA.x == inB.x && inA.y == inB.y;
    }



typedef struct pathSearchQueue {
        pathSearchRecord *head;
    } pathSearchQueue;


// returns true if A better than B (sorting function
inline static char isRecordBetter( pathSearchRecord *inA, 
                                   pathSearchRecord *inB ) {
    
    if( inA->total <= inB->total ) {
        
        if( inA->total == inB->total ) {
            
            // pick record with lower estimated cost to break tie
            if( inA->estimate < inB->estimate ) {
                return true;
                }
            }
        else {
            return true;
            }
        }
    return false;
    }

    

// sorted insertion
static void insertSearchRecord( pathSearchQueue *inQueue, 
                                pathSearchRecord *inRecordToInsert ) {
    
    // empty queue
    if( inQueue->head == NULL ) {
        inQueue->head = inRecordToInsert;
        return;
        }

    // better than head
    if( isRecordBetter( inRecordToInsert, inQueue->head ) ) {
        inRecordToInsert->nextSearchRecord = inQueue->head;    
        inQueue->head = inRecordToInsert;
        return;
        }
    
    // general case, search for spot to insert
    
    pathSearchRecord *currentRecord = inQueue->head;
    pathSearchRecord *nextRecord = currentRecord->nextSearchRecord;

    while( nextRecord != NULL ) {
        
        if( isRecordBetter( inRecordToInsert, nextRecord ) ) {
            
            // insert here
            inRecordToInsert->nextSearchRecord = nextRecord;
            
            currentRecord->nextSearchRecord = inRecordToInsert;
            return;
            }
        else {
            // keep going
            currentRecord = nextRecord;
            nextRecord = currentRecord->nextSearchRecord;
            }
        }
    

    // hit null, insert at end
    currentRecord->nextSearchRecord = inRecordToInsert;
    }



// sorted removal
static pathSearchRecord *pullSearchRecord( pathSearchQueue *inQueue, 
                                    int inSquareIndex ) {

    if( inQueue->head == NULL ) {
        return NULL;
        }

    if( inQueue->head->squareIndex == inSquareIndex ) {
        // pull head
        pathSearchRecord *currentRecord = inQueue->head;

        inQueue->head = currentRecord->nextSearchRecord;

        currentRecord->nextSearchRecord = NULL;

        return currentRecord;
        }
    

    pathSearchRecord *previousRecord = inQueue->head;
    pathSearchRecord *currentRecord = previousRecord->nextSearchRecord;
    

    while( currentRecord != NULL && 
           currentRecord->squareIndex != inSquareIndex ) {
    
        previousRecord = currentRecord;
        
        currentRecord = previousRecord->nextSearchRecord;
        }
    
    if( currentRecord == NULL ) {
        return NULL;
        }
    
    // else pull it

    // skip it in the pointer chain
    previousRecord->nextSearchRecord = currentRecord->nextSearchRecord;
    
    
    currentRecord->nextSearchRecord = NULL;

    return currentRecord;
    }





char pathFind( int inMapH, int inMapW,
               char *inBlockedMap, 
               GridPos inStart, GridPos inGoal,
               int *outFullPathLength,
               GridPos **outFullPath,
               GridPos *outClosest ) {

    // watch for degen case where start and goal are equal
    if( equal( inStart, inGoal ) ) {
        
        if( outFullPathLength != NULL ) {
            *outFullPathLength = 0;
            }
        if( outFullPath != NULL ) {
            *outFullPath = NULL;
            }
        return true;
        }
        

    
    int xTotalDelta = abs( inGoal.x - inStart.x );
    int yTotalDelta = abs( inGoal.y - inStart.y );


    // insertion-sorted queue of records waiting to be searched
    pathSearchQueue recordsToSearch;

    
    // keep records here, even after we're done with them,
    // to ensure they get deleted
    SimpleVector<pathSearchRecord*> searchQueueRecords;


    SimpleVector<pathSearchRecord> doneQueue;
    
    
    int numFloorSquares = inMapH * inMapW;


    // quick lookup of touched but not done squares
    // indexed by floor square index number
    char *openMap = new char[ numFloorSquares ];
    memset( openMap, false, numFloorSquares );

    char *doneMap = new char[ numFloorSquares ];
    memset( doneMap, false, numFloorSquares );

            
    pathSearchRecord startRecord = 
        { inStart,
          inStart.y * inMapW + inStart.x,
          0,
          getGridDistance( inStart, inGoal ),
          getGridDistance( inStart, inGoal ),
          -1,
          NULL };

    // can't keep pointers in a SimpleVector 
    // (change as vector expands itself)
    // push heap pointers into vector instead
    pathSearchRecord *heapRecord = new pathSearchRecord( startRecord );
    
    searchQueueRecords.push_back( heapRecord );
    
    
    recordsToSearch.head = heapRecord;


    openMap[ startRecord.squareIndex ] = true;
    


    char done = false;
            
            
    //while( searchQueueRecords.size() > 0 && !done ) {
    while( recordsToSearch.head != NULL && !done ) {

        // head of queue is best
        pathSearchRecord bestRecord = *( recordsToSearch.head );
        
        recordsToSearch.head = recordsToSearch.head->nextSearchRecord;


        if( false )
            printf( "Best record found:  "
                    "(%d,%d), cost %d, total %f, "
                    "pred %d, this index %d\n",
                    bestRecord.pos.x, bestRecord.pos.y,
                    bestRecord.cost, bestRecord.total,
                    bestRecord.predIndex, doneQueue.size() );
        
        doneMap[ bestRecord.squareIndex ] = true;
        openMap[ bestRecord.squareIndex ] = false;

        
        doneQueue.push_back( bestRecord );

        int predIndex = doneQueue.size() - 1;

        
        if( equal( bestRecord.pos, inGoal ) ) {
            // goal record has lowest total score in queue
            done = true;
            }
        else {
            // add neighbors
            GridPos neighbors[8];
                    
            GridPos bestPos = bestRecord.pos;

            
            // pick which neighbors to explore first
            // we want our path to walk in the long direction first
            if( yTotalDelta > xTotalDelta ) {    
                neighbors[0].x = bestPos.x;
                neighbors[0].y = bestPos.y - 1;

                neighbors[1].x = bestPos.x;
                neighbors[1].y = bestPos.y + 1;
                
                neighbors[2].x = bestPos.x - 1;
                neighbors[2].y = bestPos.y;
                
                neighbors[3].x = bestPos.x + 1;
                neighbors[3].y = bestPos.y;                
                }
            else {
                neighbors[2].x = bestPos.x;
                neighbors[2].y = bestPos.y - 1;

                neighbors[3].x = bestPos.x;
                neighbors[3].y = bestPos.y + 1;
                
                neighbors[0].x = bestPos.x - 1;
                neighbors[0].y = bestPos.y;
                
                neighbors[1].x = bestPos.x + 1;
                neighbors[1].y = bestPos.y;
                }
            
            // always prefer straight to diagonal
            neighbors[4].x = bestPos.x - 1;
            neighbors[4].y = bestPos.y - 1;
            
            neighbors[5].x = bestPos.x - 1;
            neighbors[5].y = bestPos.y + 1;
            
            neighbors[6].x = bestPos.x + 1;
            neighbors[6].y = bestPos.y + 1;
            
            neighbors[7].x = bestPos.x + 1;
            neighbors[7].y = bestPos.y - 1;

            // watch for case where our current pos is blocked
            // this can only happen when our start pos is blocked
            int bestSquareIndex = bestPos.y * inMapW + bestPos.x;
            
            char currentBlocked = false;
            
            if( inBlockedMap[ bestSquareIndex ] ) {
                currentBlocked = true;
                }
            
            
            
            // one step to neighbors from best record
            int cost = bestRecord.cost + 1;

            for( int n=0; n<8; n++ ) {
                int y = neighbors[n].y;
                int x = neighbors[n].x;
                
                // skip neighbors that are off the edge of the map
                if( x < 0 || x >= inMapW ||
                    y < 0 || y >= inMapH ) {
                
                    continue;
                    }
                
                
                if( currentBlocked && 
                    y == bestPos.y - 1 ) {
                    // forbid "down" (including diag down) moves 
                    // if our current position is blocked
                    // object we're standing on is drawn in front of us
                    // so it looks weird
                    continue;
                    }
                

                int neighborSquareIndex = y * inMapW + x;
                
                if( ! inBlockedMap[ neighborSquareIndex ] ) {
                    // floor
                    
                    char alreadyOpen = openMap[ neighborSquareIndex ];
                    char alreadyDone = doneMap[ neighborSquareIndex ];
                    
                    if( !alreadyOpen && !alreadyDone ) {
                        
                        // for testing, color touched nodes
                        // mGridColors[ neighborSquareIndex ].r = 1;
                        
                        // add this neighbor
                        double dist = 
                            getGridDistance( neighbors[n], 
                                             inGoal );
                            
                        // add cross-product to heuristic
                        // to make paths prettier
                        
                        // idea from:
                        // theory.stanford.edu/~amitp/GameProgramming/

                        // problem:  diagonal paths are hard to watch
                        // because they are so bumpy
                        /*
                        int dx1 = neighbors[n].x - inGoal.x;
                        int dy1 = neighbors[n].y - inGoal.y;
                        
                        int dx2 = inStart.x - inGoal.x;
                        int dy2 = inStart.y - inGoal.y;
                        
                        int cross = abs( dx1 * dy2 - dx2*dy1 );
                        
                        dist += cross * 0.001;
                        */
                        
                        // track how we got here (pred)
                        pathSearchRecord nRecord = { neighbors[n],
                                                     neighborSquareIndex,
                                                     cost,
                                                     dist,
                                                     dist + cost,
                                                     predIndex,
                                                     NULL };
                        pathSearchRecord *heapRecord =
                            new pathSearchRecord( nRecord );
                        
                        searchQueueRecords.push_back( heapRecord );
                        
                        insertSearchRecord( 
                            &recordsToSearch, heapRecord );

                        openMap[ neighborSquareIndex ] = true;
                        }
                    else if( alreadyOpen ) {
                        pathSearchRecord *heapRecord =
                            pullSearchRecord( &recordsToSearch,
                                              neighborSquareIndex );
                        
                        // did we reach this node through a shorter path
                        // than before?
                        if( cost < heapRecord->cost ) {
                            
                            // update it!
                            heapRecord->cost = cost;
                            heapRecord->total = heapRecord->estimate + cost;
                            
                            // found a new predecessor for this node
                            heapRecord->predIndex = predIndex;
                            }

                        // reinsert
                        insertSearchRecord( &recordsToSearch, heapRecord );
                        }
                            
                    }
                }
                    

            }
        }

    char failed = false;
    if( ! done ) {
        failed = true;
        }
    

    delete [] openMap;
    delete [] doneMap;
    

    if( failed && outClosest != NULL ) {
        // find visited spot with closest
        
        double minEst = inMapW + inMapH;
        GridPos minPos = inStart;
        
        for( int i=0; i<searchQueueRecords.size(); i++ ) {
            pathSearchRecord *r = searchQueueRecords.getElementDirect( i );
            if( r->estimate < minEst ) {
                minEst = r->estimate;
                minPos = r->pos;
                }
            }        
        *outClosest = minPos;
        }
    


    for( int i=0; i<searchQueueRecords.size(); i++ ) {
        delete *( searchQueueRecords.getElement( i ) );
        }
    
    
    if( failed ) {
        return false;
        }
    

    if( outClosest != NULL ) {
        // reached goal
        *outClosest = inGoal;
        }
    
    
            
    // follow index to reconstruct path
    // last in done queue is best-reached goal node

    int currentIndex = doneQueue.size() - 1;
            
    pathSearchRecord *currentRecord = 
        doneQueue.getElement( currentIndex );

    pathSearchRecord *predRecord = 
        doneQueue.getElement( currentRecord->predIndex );
            
    done = false;

    SimpleVector<GridPos> finalPath;
    finalPath.push_back( currentRecord->pos );

    while( ! equal(  predRecord->pos, inStart ) ) {
        currentRecord = predRecord;
        finalPath.push_back( currentRecord->pos );

        predRecord = 
            doneQueue.getElement( currentRecord->predIndex );
        
        }

    // finally, add start
    finalPath.push_back( predRecord->pos );
    

    SimpleVector<GridPos> finalPathReversed;
    
    int numSteps = finalPath.size();
    
    for( int i=numSteps-1; i>=0; i-- ) {
        finalPathReversed.push_back( *( finalPath.getElement( i ) ) );
        }


    if( outFullPathLength != NULL ) {
        *outFullPathLength = finalPath.size();
        }
    if( outFullPath != NULL ) {
        *outFullPath = finalPathReversed.getElementArray();
        }
    
    
    return true;
    }




char pathFind( int inMapH, int inMapW,
               char *inBlockedMap, 
               GridPos inStart, GridPos inWaypoint, GridPos inGoal, 
               int *outFullPathLength,
               GridPos **outFullPath,
               // if not-NULL, set to closest reachable cooridinates to inGoal
               // this will be inGoal if path find is successful, or closest
               // possible if path finding fails
               GridPos *outClosest ) {

    
    char firstFound = pathFind( inMapH, inMapW,
                                inBlockedMap,
                                inStart, inWaypoint,
                                outFullPathLength,
                                outFullPath,
                                outClosest );
    

    if( ! firstFound ) {
        return false;
        }
    

    // else find second leg and append
    int secondLength;
    GridPos *secondPath;
    GridPos secondClosest;
    
    char secondFound = pathFind( inMapH, inMapW,
                                 inBlockedMap,
                                 inWaypoint, inGoal,
                                 &secondLength,
                                 &secondPath,
                                 &secondClosest );
    
    if( !secondFound ) {
        if( outClosest != NULL ) {
            *outClosest = secondClosest;
            }
        delete [] *outFullPath;
        *outFullPath = NULL;
        
        return false;
        }

    // both legs found, combine

    SimpleVector<GridPos> steps;
    
    for( int i=0; i< *outFullPathLength; i++ ) {
        steps.push_back( (*outFullPath)[ i ] );
        }

    // don't repeat waypoint
    for( int i=1; i<secondLength; i++ ) {
        steps.push_back( secondPath[i] );
        }
    delete [] secondPath;
    delete [] *outFullPath;
    
    *outFullPathLength = steps.size();
    *outFullPath = steps.getElementArray();
    
    return true;
    }

    
    

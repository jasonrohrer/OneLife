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





char  pathFind( int inMapH, int inMapW,
                char *inBlockedMap, 
                GridPos inStart, GridPos inGoal,
                int *outFullPathLength,
                GridPos **outFullPath ) {

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
            GridPos neighbors[4];
                    
            GridPos bestPos = bestRecord.pos;
                    
            neighbors[0].x = bestPos.x;
            neighbors[0].y = bestPos.y - 1;

            neighbors[1].x = bestPos.x;
            neighbors[1].y = bestPos.y + 1;

            neighbors[2].x = bestPos.x - 1;
            neighbors[2].y = bestPos.y;

            neighbors[3].x = bestPos.x + 1;
            neighbors[3].y = bestPos.y;
                    
            // one step to neighbors from best record
            int cost = bestRecord.cost + 1;

            for( int n=0; n<4; n++ ) {
                int neighborSquareIndex = 
                    neighbors[n].y * inMapW + neighbors[n].x;
                
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
    
    for( int i=0; i<searchQueueRecords.size(); i++ ) {
        delete *( searchQueueRecords.getElement( i ) );
        }
    
    
    if( failed ) {
        return false;
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

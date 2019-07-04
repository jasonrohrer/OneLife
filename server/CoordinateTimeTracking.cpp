#include "CoordinateTimeTracking.h"




char CoordinateTimeTracking::checkExists( int inX, int inY, 
                                          timeSec_t inCurTime ) {
    for( int i=0; i<mRecords.size(); i++ ) {
        CoordinateXYRecord *r = mRecords.getElement( i );
        
        if( r->x == inX && r->y == inY ) {
            r->t = inCurTime;
            return true;
            }
        }
    
    // insert new
    CoordinateXYRecord r = { inX, inY, inCurTime };
    mRecords.push_back( r );
    
    return false;
    }



void CoordinateTimeTracking::cleanStale( timeSec_t inStaleTime ) {
    double startTime = Time::getCurrentTime();
    
    int beforeCleanSize = mRecords.size();
    
    for( int i=0; i<mRecords.size(); i++ ) {
        CoordinateXYRecord *r = mRecords.getElement( i );
        
        if( r->t <= inStaleTime ) {
            mRecords.deleteElement( i );
            i--;
            }
        }
    printf( "(%f ms ) Before cleaning, %d, after %d\n", 
            ( Time::getCurrentTime() - startTime ) * 1000,
            beforeCleanSize,
            mRecords.size() );
    }


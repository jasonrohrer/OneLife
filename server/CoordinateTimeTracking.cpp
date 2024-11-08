#include "CoordinateTimeTracking.h"



CoordinateTimeTracking::CoordinateTimeTracking()
        :mNextIndex( 0 ) {

    // vector contains structs, which can be safely memcpy'd
    mRecords.toggleFastMethods( true );
    }




char CoordinateTimeTracking::checkExists( int inX, int inY, 
                                          timeSec_t inCurTime ) {
    
    int dir = 1;
    
    int numRecords = mRecords.size();
    

    if( numRecords > 0 ) {
        
        CoordinateXYRecord *testR = mRecords.getElement( mNextIndex );

        if( testR->y == inY &&
            testR->x == inX ) {
            
            testR->t = inCurTime;
            
            return true;
            }


        if( testR->y > inY ||
            ( testR->y == inY 
              && 
              testR->x > inX ) ) {
            dir = -1;
            }


        if( dir == 1 ) {
            for( mNextIndex = mNextIndex; 
                 mNextIndex < numRecords; mNextIndex ++ ) {
                
                CoordinateXYRecord *r = mRecords.getElement( mNextIndex );
                if( r->y == inY &&
                    r->x == inX ) {
                    
                    r->t = inCurTime;
                    
                    return true;
                    }
                
                if( r->y > inY ||
                    ( r->y == inY &&
                      r->x > inX ) ) {
                    // walked past spot and not found
                    // keep mNextIndex as insert point
                    break;
                    }
                }
            }
        else if( dir == -1 ) {
            for( mNextIndex = mNextIndex; mNextIndex > -1; mNextIndex -- ) {
                CoordinateXYRecord *r = mRecords.getElement( mNextIndex );
                if( r->y == inY &&
                    r->x == inX ) {
                    
                    r->t = inCurTime;
                    
                    return true;
                    }
                
                if( r->y < inY ||
                    ( r->y == inY &&
                      r->x < inX ) ) {
                    // walked past spot and not found
                    // keep mNextIndex as insert point
                    break;
                    }
                }
            }
        }
    else {
        // no records, use this as insertion point
        mNextIndex = 0;
        }
    

    // not found
    
    // insert new
    CoordinateXYRecord r = { inX, inY, inCurTime };
    
    if( mNextIndex == numRecords ) {
        // stick on end
        mRecords.push_back( r );
        }
    else if( mNextIndex == -1 ) {
        // stick on front
        mRecords.push_front( r );
        mNextIndex = 0;
        }
    else {
        mRecords.push_middle( r, mNextIndex );
        }
    return false;
    }



void CoordinateTimeTracking::cleanStale( timeSec_t inStaleTime ) {
    int beforeCleanSize = mRecords.size();
    
    // pre-allocate space
    SimpleVector<CoordinateXYRecord> temp( beforeCleanSize );
    
    
    for( int i=0; i<mRecords.size(); i++ ) {
        CoordinateXYRecord *r = mRecords.getElement( i );
        
        if( r->t > inStaleTime ) {
            temp.push_back( *r );
            }
        }
    mRecords.deleteAll();
    mRecords.push_back_other( &temp );
    

    // start index over after cleaning
    mNextIndex = 0;
    }


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/system/Time.h"


typedef struct IPRecord {
        char *ipAddress;
        double lastBadConnectionTime;
        int badConnectionCount;
    } IPRecord;



static SimpleVector<IPRecord> records;


// ban an IP after 10 bad connections
static int badConnectionCountThreshold = 10;


// decrement bad connection count per IP every 5 minutes
static double decrementSeconds = 300;

static double lastStaleProcessingTime = 0;

// process stale records every 5 minutes
static double staleProcessingInterval = 300;




void freeIPRecord( IPRecord inR ) {
    delete [] inR.ipAddress;
    }

    


void initIPBanList() {
    }




void freeIPBanList() {
    for( int i=0; i<records.size(); i++ ) {
        freeIPRecord( records.getElementDirect( i ) );
        }
    records.deleteAll();
    }



static IPRecord *getRecordForIP( char *inIPAddress ) {
    for( int i=0; i<records.size(); i++ ) {
        IPRecord *r = records.getElement( i );
        
        if( strcmp( r->ipAddress, inIPAddress ) == 0 ) {
            return r;
            }
        }

    return NULL;
    }



static void deleteRecordForIP( char *inIPAddress ) {
    for( int i=0; i<records.size(); i++ ) {
        IPRecord *r = records.getElement( i );
        
        if( strcmp( r->ipAddress, inIPAddress ) == 0 ) {
            freeIPRecord( *r );
            records.deleteElement( i );
            return;
            }
        }
    }



static int getBadConnectionCount( char *inIPAddress ) {
    IPRecord *r = getRecordForIP( inIPAddress );
    
    if( r == NULL ) {
        return 0;
        }
    
    double timePassed = Time::getCurrentTime() - r->lastBadConnectionTime;
    
    int decrementedCount = r->badConnectionCount - 
        lrint( timePassed / decrementSeconds );

    // note that we don't ever decrement the actual count
    // (unless the decremented value reaches 0, and we delete the whole record)
    // so for an active bot that is hammering, we make them wait longer and
    // longer before trying to accept another one of their connections
    // if that one is bad too, we reset the lastBadConnectionTime to the present
    // without reducing their count, which means that they will have to
    // wait even longer next time to have their decremented count drop
    // down below the threshold.

    if( decrementedCount <= 0 ) {
        deleteRecordForIP( inIPAddress );
        return 0;
        }
    
    return decrementedCount;
    }



static void processStaleRecords() {
    double curTime = Time::getCurrentTime();
    if( curTime - lastStaleProcessingTime > staleProcessingInterval ) {
        printf( "STALE processing of %d records\n", records.size() );
        for( int i=0; i<records.size(); i++ ) {
            IPRecord *r = records.getElement( i );
            
            if( getBadConnectionCount( r->ipAddress ) == 0 ) {
                // record has been deleted
                i--;
                }
            }
        
        lastStaleProcessingTime = curTime;
        }
    }



void addBadConnectionForIP( char *inIPAddress ) {
    processStaleRecords();
    
    IPRecord *r = getRecordForIP( inIPAddress );
    
    if( r != NULL ) {
        r->badConnectionCount ++;
        r->lastBadConnectionTime = Time::getCurrentTime();
        }
    else {
        IPRecord newR = { stringDuplicate( inIPAddress ),
                          Time::getCurrentTime(),
                          1 };
        
        records.push_back( newR );                          
        }
    }



char isIPBanned( char *inIPAddress ) {
    if( getBadConnectionCount( inIPAddress ) > badConnectionCountThreshold ) {
        return true;
        }
    return false;
    }


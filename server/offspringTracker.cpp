#include "offspringTracker.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/system/Time.h"


// expire after 10 days of no activity, so stale emails don't linger
// indefinitely
static double expirationSeconds = 3600 * 24 * 10;

static double lastCheckTime = 0;

// check every hour
static double checkInterval = 3600;



typedef struct OffspringTrackingRecord {
        char *email;
        int lineageID;
        double expireTime;
    } OffspringTrackingRecord;
        


static SimpleVector<OffspringTrackingRecord> records;


void initOffspringTracker() {
    }



void freeOffspringTracker() {
    for( int i=0; i<records.size(); i++ ) {
        delete [] records.getElement(i)->email;
        }
    records.deleteAll();
    }


static int findRecordIndex( char *inEmail ) {
    for( int i=0; i<records.size(); i++ ) {
        OffspringTrackingRecord *r = records.getElement( i );
        if( strcmp( r->email, inEmail ) == 0 ) {
            return i;
            }
        }
    return -1;
    }



static OffspringTrackingRecord *findRecord( char *inEmail ) {
    int i = findRecordIndex( inEmail );
    
    if( i != -1 ) {
        return records.getElement( i );
        }
    else {
        return NULL;
        }
    }



static void cleanStaleRecords() {
    double curTime = Time::getCurrentTime();
    if( curTime - lastCheckTime > checkInterval ) {
        lastCheckTime = curTime;
        
        for( int i=0; i<records.size(); i++ ) {
            OffspringTrackingRecord *r = records.getElement( i );
            
            if( curTime > r->expireTime ) {
                
                delete [] r->email;
                records.deleteElement( i );
                i--;
                }
            } 
        }
    }





// lineageID is player's own life ID if they are female
// or their mother's life ID if they are male.
//
// if female, we look only at their own descendants.
// if male, we look at their sister's decendants.
void trackOffspring( char *inPlayerEmail, int inLineageIDToTrack ) {
    OffspringTrackingRecord *r = findRecord( inPlayerEmail );
    
    if( r != NULL ) {
        r->lineageID = inLineageIDToTrack;
        }
    else {
        OffspringTrackingRecord newR = { stringDuplicate( inPlayerEmail ),
                                         inLineageIDToTrack,
                                         Time::getCurrentTime() + 
                                         expirationSeconds };
        records.push_back( newR );
        }
    
    cleanStaleRecords();
    }



char isOffspringTracked( char *inPlayerEmail ) {
    return ( findRecord( inPlayerEmail ) != NULL );
    }



int getOffspringLineageID( char *inPlayerEmail ) {
    OffspringTrackingRecord *r = findRecord( inPlayerEmail );
    
    if( r != NULL ) {
        return r->lineageID;
        }
    else {
        return -1;
        }
    }



void clearOffspringLineageID( char *inPlayerEmail ) {
    int i = findRecordIndex( inPlayerEmail );
        
    if( i != -1 ) {
        delete [] records.getElement( i )->email;
        records.deleteElement( i );
        }
    }


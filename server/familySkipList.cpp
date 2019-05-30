#include "familySkipList.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/system/Time.h"



typedef struct FamilySkipListRecord {
        char *babyEmail;
        
        SimpleVector<int> *skippedLineageList;
        
        double lastUpdateTime;
    } FamilySkipListRecord;
        

static void freeRecordMembers( FamilySkipListRecord *inR ) {
    delete [] inR->babyEmail;
    delete inR->skippedLineageList;
    }

SimpleVector<FamilySkipListRecord> skipListRecords;



void initFamilySkipList() {
    }



void freeFamilySkipList() {
    for( int i=0; i<skipListRecords.size(); i++ ) {
        FamilySkipListRecord *r = skipListRecords.getElement( i );
        freeRecordMembers( r );
        }
    skipListRecords.deleteAll();
    }



// makes new one if one doesn't exist, if requested, 
// or returns NULL if not found
static FamilySkipListRecord *findRecord( char *inBabyEmail, 
                                         char inMakeNew = false ) {
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<skipListRecords.size(); i++ ) {
        FamilySkipListRecord *r = skipListRecords.getElement( i );
        
        if( strcmp( r->babyEmail, inBabyEmail ) == 0 ) {
            return r;
            }
        else if( curTime - r->lastUpdateTime > 7200 ) {
            // record not touched for 2 hours
            freeRecordMembers( r );
            skipListRecords.deleteElement( i );
            i--;
            }
        }

    // no matching email found

    if( ! inMakeNew ) {
        return NULL;
        }

    FamilySkipListRecord newR = 
        { stringDuplicate( inBabyEmail ),
          new SimpleVector<int>(),
          curTime };
    
    skipListRecords.push_back( newR );

    return skipListRecords.getElement( skipListRecords.size() - 1 );
    }



void skipFamily( char *inBabyEmail, int inLineageEveID ) {
    FamilySkipListRecord *r = findRecord( inBabyEmail, true );
    
    r->skippedLineageList->push_back( inLineageEveID );
    }



void clearSkipList( char *inBabyEmail ) {
    FamilySkipListRecord *r = findRecord( inBabyEmail );
    
    if( r != NULL ) {
        r->skippedLineageList->deleteAll();
        }
    }



char isSkipped( char *inBabyEmail, int inLineageEveID ) {

    FamilySkipListRecord *r = findRecord( inBabyEmail );

    if( r == NULL ) {
        return false;
        }
    
    if( r->skippedLineageList->getElementIndex( inLineageEveID ) != -1 ) {
        return true;
        }
    
    return false;
    }


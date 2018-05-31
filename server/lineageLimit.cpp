#include "lineageLimit.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/system/Time.h"


#include <stdint.h>


typedef struct LineageTime {
        int lineageEveID;
        double lastBornTime;
    } LineageTime;
    


typedef struct HashTableEntry {
        char *email;
        
        double freshestTime;
        
        SimpleVector<LineageTime> *times;
    } HashTableEntry;


#define TABLE_SIZE 4096    

static SimpleVector<HashTableEntry> hashTable[ TABLE_SIZE ];


void initLineageLimit() {
    }



void freeLineageLimit() {
    for( int i=0; i<TABLE_SIZE; i++ ) {
        for( int j=0; j<hashTable[i].size(); j++ ) {
            HashTableEntry *e = hashTable[i].getElement( j );
            
            delete [] e->email;
            delete e->times;
            }
        hashTable[i].deleteAll();
        }
    }

    



static char testSkipped = false;

static double staleTime = 0;


void primeLineageTest( int inNumLivePlayers ) {
    double fractionOfMax = ( inNumLivePlayers - 10 ) / 40.0;
    
    if( fractionOfMax > 1 ) {
        fractionOfMax = 1;
        }
    
    float maxHours = 
        SettingsManager::getFloatSetting( "lineageLimitMaxHours", 24 );
    
    double hours = fractionOfMax * maxHours;

    if( hours <= 0 ) {
        testSkipped = true;
        return;
        }

    testSkipped = false;

    double sec = hours * 3600;
    
    staleTime = Time::getCurrentTime() - sec;
    }



static SimpleVector<HashTableEntry> *lookupBin( const char *inPlayerEmail ) {
    
    uint32_t hash = 5381;
    int len = strlen( inPlayerEmail );
    for( int i=0; i<len; i++ ) {
        hash = ((hash << 5) + hash) + 
            (uint32_t)(((const uint8_t *)inPlayerEmail)[i]);
        }

    int binNumber = hash % TABLE_SIZE;
    
    return &( hashTable[ binNumber ] );
    }



// returns NULL if not found
static HashTableEntry *lookup( const char *inPlayerEmail ) {
    SimpleVector<HashTableEntry> *bin = lookupBin( inPlayerEmail );
    
    for( int i=0; i<bin->size(); i++ ) {
        HashTableEntry *e = bin->getElement( i );
        
        if( e->freshestTime < staleTime ) {
            // a completely stale entry, remove it
            delete [] e->email;
            delete e->times;
            bin->deleteElement( i );
            i--;
            }
        else if( strcmp( e->email, inPlayerEmail ) == 0 ) {
            return e;
            }
        }
    return NULL;
    }



static void insert( const char *inPlayerEmail, int inLineageEveID ) {
    // new record saying player born in this line NOW
    
    double curTime = Time::getCurrentTime();
    
    SimpleVector<LineageTime> *tList = new SimpleVector<LineageTime>();
    
    LineageTime tNew = { inLineageEveID, curTime };
    
    tList->push_back( tNew );

    HashTableEntry e = { stringDuplicate( inPlayerEmail ),
                         Time::getCurrentTime(),
                         tList };

    SimpleVector<HashTableEntry> *bin = lookupBin( inPlayerEmail );

    bin->push_back( e );
    }



char isLinePermitted( const char *inPlayerEmail, int inLineageEveID ) {
    if( testSkipped ) {
        return true;
        }
    
    
    HashTableEntry *e = lookup( inPlayerEmail );
    
    if( e == NULL ) {
        return true;
        }
    
    for( int i=0; i<e->times->size(); i++ ) {
        LineageTime *t = e->times->getElement( i );
        
        if( t->lastBornTime < staleTime ) {
            // a stale birth time, remove it
            e->times->deleteElement( i );
            i--;
            }
        else if( t->lineageEveID == inLineageEveID ) {
            // born in this lineage, and time not stale
            return false;
            }
        }
    
    // no matching lineage found
    return true;
    }



void recordLineage( const char *inPlayerEmail, int inLineageEveID ) {
    HashTableEntry *e = lookup( inPlayerEmail );
    
    if( e == NULL ) {
        insert( inPlayerEmail, inLineageEveID );
        return;
        }


    double curTime = Time::getCurrentTime();

    for( int i=0; i<e->times->size(); i++ ) {
        LineageTime *t = e->times->getElement( i );
        
        if( t->lineageEveID == inLineageEveID ) {
            // previously born in this lineage, adjust with new birth time
            t->lastBornTime = curTime;
            return;
            }
        }

    // not found, add new one
    LineageTime t = { inLineageEveID, curTime };
    e->times->push_back( t );
    e->freshestTime = curTime;
    }


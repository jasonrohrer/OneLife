#include "lineageLimit.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/system/Time.h"


#include <stdint.h>


extern double secondsPerYear;



typedef struct LineageTime {
        int lineageEveID;
        double lastBornTime;
        double totalLivedThisLine;
        double totalLivedOtherLines;
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

// after 24 hours, drop records, to keep them from building up forever
static double staleTimeout = 24 * 3600;
static double staleTime = 0;


static double otherLineRequiredYears = 0;
static double oneLineMaxYears = 0;


void primeLineageTest( int inNumLivePlayers ) {

    staleTime = Time::getCurrentTime() - staleTimeout;

    double fractionOfMax = ( inNumLivePlayers - 10 ) / 40.0;
    
    if( fractionOfMax > 1 ) {
        fractionOfMax = 1;
        }
    
    float maxHours = 
        SettingsManager::getFloatSetting( "lineageLimitOtherLineHours", 1.5 );
    
    double hours = fractionOfMax * maxHours;

    if( hours <= 0 ) {
        testSkipped = true;
        return;
        }

    testSkipped = false;

    otherLineRequiredYears = ( hours * 3600 ) / secondsPerYear;


    float maxHoursOneLine = 
        SettingsManager::getFloatSetting( "lineageLimitOneLineHours", 0.5 );

    oneLineMaxYears = ( maxHoursOneLine * 3600 ) / secondsPerYear;
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



static void insert( const char *inPlayerEmail, int inLineageEveID,
                    double inLivedYears ) {
    // new record saying player born in this line NOW
    
    double curTime = Time::getCurrentTime();
    
    SimpleVector<LineageTime> *tList = new SimpleVector<LineageTime>();
    
    LineageTime tNew = { inLineageEveID, curTime, inLivedYears, 0 };
    
    tList->push_back( tNew );

    HashTableEntry e = { stringDuplicate( inPlayerEmail ),
                         curTime,
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
            // born in this lineage before, and time not stale

            if( t->totalLivedThisLine < oneLineMaxYears ) {
                // we're allowed to live more in this line
                return true;
                }
            else if( t->totalLivedOtherLines >= otherLineRequiredYears ) {
                // we've lived long enough in other lines
                // clear our count in this line
                t->totalLivedThisLine = 0;
                return true;
                }

            // else we've lived too long in this line, and not long enough
            // in other lines
            return false;
            }
        }
    
    // no matching lineage found
    return true;
    }



void recordLineage( const char *inPlayerEmail, int inLineageEveID,
                    double inLivedYears, char inMurdered, 
                    char inCommittedMurder ) {

    if( inMurdered || inCommittedMurder ) {
        // push up over the limit no matter how long they have lived
        inLivedYears += oneLineMaxYears;
        }

    HashTableEntry *e = lookup( inPlayerEmail );
    
    if( e == NULL ) {
        insert( inPlayerEmail, inLineageEveID, inLivedYears );
        return;
        }
    
    

    double curTime = Time::getCurrentTime();

    char found = false;
    for( int i=0; i<e->times->size(); i++ ) {
        LineageTime *t = e->times->getElement( i );
        
        if( t->lineageEveID == inLineageEveID ) {
            // previously born in this lineage, adjust with new birth time
            t->lastBornTime = curTime;
            t->totalLivedThisLine += inLivedYears;
            
            // clear out count in other lines
            // that count only starts happening after we're done
            // living in this line
            t->totalLivedOtherLines = 0;
            
            found = true;
            }
        else {
            t->totalLivedOtherLines += inLivedYears;
            }
        }

    if( !found ) {
        // not found, add new one
        LineageTime t = { inLineageEveID, curTime, inLivedYears, 0 };
        e->times->push_back( t );
        e->freshestTime = curTime;
        }
    }


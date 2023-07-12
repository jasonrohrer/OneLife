#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"

#include "minorGems/util/SimpleVector.h"


typedef struct CountRecord {
        char email[40];
        int recordedCount;
        int recount;
    } CountRecord;


SimpleVector<CountRecord> records;


// returns a newly allocated record with 0 counts if one doesn't exist
CountRecord *findRecord( char inEmail[40] ) {
    for( int i=0; i<records.size(); i++ ) {
        CountRecord *r = records.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            return r;
            }
        }
    
    // not found, insert new
    CountRecord newRec;
    memcpy( newRec.email, inEmail, 40 );
    newRec.recordedCount = 0;
    newRec.recount = 0;
    
    records.push_back( newRec );
    
    return records.getElement( records.size() - 1 );
    }





// inIndex = 0 for sender, 1 for receiver
// result NOT destroyed by caller (pointer to internal buffer)

char senderBuffer[40];
char receiverBuffer[40];

static char *getEmailFromKey( unsigned char *inKey, int inIndex ) {
    
    sscanf( (char*)inKey, "%39[^,],%39s", senderBuffer, receiverBuffer );
    
    if( inIndex == 0 ) {
        return senderBuffer;
        }
    else {
        return receiverBuffer;
        }
    }

    

int main() {
    LINEARDB3 db;
    
    int error = LINEARDB3_open( &db, 
                                "curses.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                // sender email (truncated to 39 chars max)
                                // with receiver email (trucated to 39) 
                                // with comma separating them
                                // terminated by a NULL character
                                // append spaces to the end if needed
                                // (after the NULL character) to fill
                                // the full 80 characters consistently
                                80, 
                                // one 64-bit double, representing the time
                                // the cursed was placed
                                // in whatever binary format and byte order
                                // "double" on the server platform uses
                                8 );

    if( error ) {
        printf( "Error %d opening curses LinearDB3", error );
        return 1;
        }
    

    LINEARDB3 dbCount;
    
    error = LINEARDB3_open( &dbCount, 
                                "curseCount.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                40, 
                                12 );

    if( error ) {
        printf( "Error %d opening curseCount LinearDB3", error );
        
        LINEARDB3_close( &db );
        return 1;
        }
    

    LINEARDB3_Iterator dbCountI;
    
    
    LINEARDB3_Iterator_init( &dbCount, &dbCountI );
    
    unsigned char key[40];
    
    unsigned char value[12];


    while( LINEARDB3_Iterator_next( &dbCountI, key, value ) > 0 ) {
        int count = valueToInt( value );
        
        CountRecord *r = findRecord( (char*) key );
        
        r->recordedCount = count;
        }




    LINEARDB3_Iterator dbI;
    
    
    LINEARDB3_Iterator_init( &db, &dbI );
    
    unsigned char keyB[80];
    
    unsigned char valueB[8];
    
    
    

    while( LINEARDB3_Iterator_next( &dbI, keyB, valueB ) > 0 ) {
        
        char *receiverEmail = getEmailFromKey( keyB, 1 );
        
        CountRecord *r = findRecord( receiverEmail );
        
        r->recount ++;

        if( strcmp( receiverEmail, "" ) == 0 ) {
            char *senderEmail = getEmailFromKey( keyB, 0 );
            
            timeSec_t curseTime = valueToTime( value );
            timeSec_t curTimeSec = Time::timeSec();
            
            int daysAgo = ( curTimeSec - curseTime ) /
                ( 3600 * 24 );
            
            printf( "%d days ago BLANK RECEIVER EMAIL found for %s -> %s\n",
                    daysAgo, senderEmail, receiverEmail );
            }
        }

    
    for( int i=0; i<records.size(); i++ ) {
        CountRecord *r = records.getElement( i );

        printf( "%d %d %s\n", r->recount, r->recordedCount, r->email );
        }


    LINEARDB3_close( &db );
    LINEARDB3_close( &dbCount );
        
    return 0;
    }

        
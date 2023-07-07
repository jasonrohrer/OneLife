#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"


int main() {
    
    LINEARDB3 dbCount;
    
    int error = LINEARDB3_open( &dbCount, 
                                "curseCount.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                40, 
                                12 );

    if( error ) {
        printf( "Error %d opening curseCount LinearDB3", error );
        return 1;
        }
    

    LINEARDB3_Iterator dbi;
    
    
    LINEARDB3_Iterator_init( &dbCount, &dbi );
    
    unsigned char key[40];
    
    unsigned char value[12];
    
    int total = 0;
    int stale = 0;
    int nonStale = 0;
    
    char forceStale = false;
    
    
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;
        
        int count = valueToInt( value );

        key[39] = '\0';
        
        printf( "%d %s\n", count, key );
        }
    
    return 0;
    }

        
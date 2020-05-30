#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"



// process dump like this:
//
// ./curseDBDump | sed -e "s/[^\@]*\@[^\@]*\.com//" | sort | uniq -c | sort -nr | more

int main() {
    
    LINEARDB3 db;

    int error = LINEARDB3_open( &db, 
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
    
    
    LINEARDB3_Iterator_init( &db, &dbi );

    
    unsigned char key[40];
    
    unsigned char value[12];
    
    
    // first, just count
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {

        key[39] = '\0';
        
        int count = valueToInt( value );

        timeSec_t t = valueToTime( &( value[4] ) );

        time_t tt = (time_t)t;
        
        char *humanTime = ctime( &tt );

        printf( "%s | %d | %s\n", (char*)key, count, humanTime );
        }


    LINEARDB3_close( &db );

    return 1;
    }

#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"



// process dump like this:
//
// ./curseDBDump | sed -e "s/[^\@]*\@[^\@]*\.com//" | sort | uniq -c | sort -nr | more

int main() {
    
    LINEARDB3 db;

    int error = LINEARDB3_open( &db, 
                                "curses.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                // sender email (truncated to 40 chars max)
                                // with receiver email (trucated to 40) 
                                // appended.
                                // append spaces to the end if needed 
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


    LINEARDB3_Iterator dbi;
    
    
    LINEARDB3_Iterator_init( &db, &dbi );

    
    unsigned char key[80];
    
    unsigned char value[8];
    
    int total = 0;
    int stale = 0;
    int nonStale = 0;
    
    // first, just count
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {

        key[79] = '\0';
        
        timeSec_t t = valueToTime( value );

        time_t tt = (time_t)t;
        
        char *humanTime = ctime( &tt );

        printf( "%s | %s\n", (char*)key, humanTime );
        }


    LINEARDB3_close( &db );

    return 1;
    }

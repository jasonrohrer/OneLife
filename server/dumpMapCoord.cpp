#include "lineardb3.h"
#include "kissdb.h"
#include "dbCommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void usage() {
    printf( "Usage:\n\n"
            "dumpMapCoord x y\n\n" );
    printf( "Pipe usage:\n\n"
            "genCoordsList | dumpMapCoord -\n\n" );
    exit( 1 );
    }


int main( int inNumArgs, char **inArgs ) {
    char pipeRead = false;
    
    if( inNumArgs == 2 &&
        strcmp( inArgs[1], "-" ) == 0 ) {
        pipeRead = true;
        }
    else if( inNumArgs != 3 ) {
        usage();
        }
    
    int x = 0;
    int y = 0;

    if( ! pipeRead ) {
        int numRead = sscanf( inArgs[1], "%d", &x );
        numRead += sscanf( inArgs[2], "%d", &y );
        
        if( numRead != 2 ) {
            usage();
            }
        }
    else {
        int numRead = scanf( "%d", &x );
        numRead += scanf( "%d", &y );
        
        if( numRead != 2 ) {
            usage();
            }
        }
    
    
    LINEARDB3 db;

    int error = LINEARDB3_open( &db, 
                                "map.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                80000,
                                16, // four 32-bit ints, xysb
                                // s is the slot number 
                                // s=0 for base object
                                // s=1 decay ETA seconds (wall clock time)
                                // s=2 for count of contained objects
                                // s=3 first contained object
                                // s=4 second contained object
                                // s=... remaining contained objects
                                // Then decay ETA for each slot, in order,
                                //   after that.
                                // s = -1
                                //  is a special flag slot set to 0 if NONE
                                //  of the contained items have ETA decay
                                //  or 1 if some of the contained items might 
                                //  have ETA decay.
                                //  (this saves us from having to check each
                                //   one)
                                // If a contained object id is negative,
                                // that indicates that it sub-contains
                                // other objects in its corresponding b slot
                                //
                                // b is for indexing sub-container slots
                                // b=0 is the main object 
                                // b=1 is the first sub-slot, etc.
                                4 // one int, object ID at x,y in slot (s-3)
                                // OR contained count if s=2
                                );
    
    if( error ) {
        printf( "failed to open map.db\n\n" );
        usage();
        }
    
    char inputDone = false;

    while( !inputDone ) {
        unsigned char key[16];
        unsigned char value[4];
        
        intQuadToKey( x, y, 0, 0, key );
        
        int result = LINEARDB3_get( &db, key, value );
        
        if( result == 0 ) {
            // found
            int val = valueToInt( value );
            printf( "%d\n", val );
            }
        else {
            printf( "0\n" );
            }
        
        if( pipeRead ) {
            int numRead = scanf( "%d", &x );
            numRead += scanf( "%d", &y );
        
            if( numRead != 2 ) {
                inputDone = true;
                }
            }
        else {
            inputDone = true;
            }
        }
    
    
    LINEARDB3_close( &db );

    return 0;
    }

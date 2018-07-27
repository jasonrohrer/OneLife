#include <stdlib.h>


#include "minorGems/util/stringUtils.h"

#include "kissdb.h"
#include "stackdb.h"


void usage() {
    printf( "Usage:\n" );
    printf( "dbCount stackdb_file table_size key_size value_size\n\n" );
    
    printf( "Example:\n" );
    printf( "dbConvert map.db 80000 16 4\n\n" );
    
    exit( 1 );
    }



int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 5 ) {
        usage();
        }
    int tableSize = 0;
    int keySize = 0;
    int valueSize = 0;
    
    sscanf( inArgs[2], "%d", &tableSize );
    
    if( tableSize == 0 ) {
        usage();
        }

    sscanf( inArgs[3], "%d", &keySize );
    
    if( keySize == 0 ) {
        usage();
        }

    sscanf( inArgs[4], "%d", &valueSize );
    
    if( valueSize == 0 ) {
        usage();
        }


    char *oldFileName = inArgs[1];
    
    STACKDB db;

    int error = STACKDB_open( &db,
                              oldFileName,
                              KISSDB_OPEN_MODE_RDONLY,
                              tableSize,
                              keySize,
                              valueSize );
    if( error ) {
        printf( "dbCount Failed to open stackdb file %s\n",
                oldFileName );
        exit( 1 );
        }
    

    // insert all keys from old to new
    unsigned char *keyBuff = new unsigned char[ keySize ];
    unsigned char *valueBuff = new unsigned char[ valueSize ];

    STACKDB_Iterator dbi;
    
    STACKDB_Iterator_init( &db, &dbi );
    
    int count = 0;
    while( STACKDB_Iterator_next( &dbi, keyBuff, valueBuff ) > 0 ) {
        count ++;
        }

    printf( "%s contains %d records\n", oldFileName, count );
    

    delete [] keyBuff;
    delete [] valueBuff;
    
    STACKDB_close( &db );
    

    
    return 0;
    }

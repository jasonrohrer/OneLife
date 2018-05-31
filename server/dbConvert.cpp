#include <stdlib.h>


#include "minorGems/util/stringUtils.h"

#include "kissdb.h"
#include "stackdb.h"


void usage() {
    printf( "Usage:\n" );
    printf( "dbConvert kiss_db_file new_table_size\n\n" );
    
    printf( "Example:\n" );
    printf( "dbConvert map.db 80000\n\n" );
    
    exit( 1 );
    }



int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 3 ) {
        usage();
        }
    int newTableSize = 0;
    
    sscanf( inArgs[2], "%d", &newTableSize );
    
    if( newTableSize == 0 ) {
        usage();
        }

    char *oldFileName = inArgs[1];
    
    KISSDB db;

    int error = KISSDB_open( &db,
                             oldFileName,
                             KISSDB_OPEN_MODE_RDONLY,
                             0,
                             0,
                             0 );
    if( error ) {
        printf( "dbConvert Failed to open kissdb file %s\n",
                oldFileName );
        exit( 1 );
        }
    
    int keySize = db.key_size;
    int valueSize = db.value_size;
    
    


    char tempFileName[1000];
    
    sprintf( tempFileName, "%s.temp", oldFileName );
    

    STACKDB dbNew;

    error = STACKDB_open( &dbNew,
                          tempFileName,
                          0,
                          newTableSize,
                          keySize,
                          valueSize );
    if( error ) {
        printf( "dbConvert: Failed to open temp stackdb file %s\n", 
                tempFileName );
        KISSDB_close( &db );
        exit( 1 );
        }

    // insert all keys from old to new
    unsigned char *keyBuff = new unsigned char[ keySize ];
    unsigned char *valueBuff = new unsigned char[ valueSize ];

    KISSDB_Iterator dbi;
    
    KISSDB_Iterator_init( &db, &dbi );
    
    while( KISSDB_Iterator_next( &dbi, keyBuff, valueBuff ) > 0 ) {
        STACKDB_put_new( &dbNew, keyBuff, valueBuff );
        }

    delete [] keyBuff;
    delete [] valueBuff;
    
    KISSDB_close( &db );
    STACKDB_close( &dbNew );
        
    if( rename ( tempFileName, oldFileName ) != 0 ) {
        printf( "dbConvert: Failed to move temp file %s to "
                "overwrite old kissdb file %s\n",
                tempFileName, oldFileName );
        }
    

    
    return 0;
    }

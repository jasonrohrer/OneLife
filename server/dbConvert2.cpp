#include <stdlib.h>


#include "minorGems/util/stringUtils.h"

#include "kissdb.h"
#include "stackdb.h"
#include "lineardb.h"


void usage() {
    printf( "Usage:\n" );
    printf( "dbConvert stack_db_file old_table_size key_size value_size\n\n" );
    
    printf( "Example:\n" );
    printf( "dbConvert map.db 80000 16 4 1000000\n\n" );
    
    exit( 1 );
    }



int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 5 ) {
        usage();
        }

    int oldTableSize = 0;
    int keySize = 0;
    int valueSize = 0;
    int newTableSize = 0;


    sscanf( inArgs[2], "%d", &oldTableSize );
    
    if( oldTableSize == 0 ) {
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
                              0,
                              oldTableSize,
                              keySize,
                              valueSize );
    if( error ) {
        printf( "dbConvert Failed to open stackdb file %s\n",
                oldFileName );
        exit( 1 );
        }
    
    


    char tempFileName[1000];
    
    sprintf( tempFileName, "%s.temp", oldFileName );
    

    // insert all keys from old to new
    unsigned char *keyBuff = new unsigned char[ keySize ];
    unsigned char *valueBuff = new unsigned char[ valueSize ];

    STACKDB_Iterator dbi;
    
    STACKDB_Iterator_init( &db, &dbi );

    
    printf( "Counting records in %s stackdb...\n", oldFileName );
    
    int count = 0;
    while( STACKDB_Iterator_next( &dbi, keyBuff, valueBuff ) > 0 ) {
        count ++;
        }
    
    printf( "...%d records found\n", count );
    
    
    // anticipate maxLoad of 0.5
    newTableSize = count * 2;

    printf( "Converting %s from stackdb to lineardb with %d "
            "starting size...\n", oldFileName, newTableSize );

    LINEARDB dbNew;

    error = LINEARDB_open( &dbNew,
                          tempFileName,
                          0,
                          newTableSize,
                          keySize,
                          valueSize );
    if( error ) {
        printf( "dbConvert: Failed to open temp linear file %s\n", 
                tempFileName );
        STACKDB_close( &db );
        exit( 1 );
        }



    STACKDB_Iterator_init( &db, &dbi );

    count = 0;
    while( STACKDB_Iterator_next( &dbi, keyBuff, valueBuff ) > 0 ) {
        LINEARDB_put( &dbNew, keyBuff, valueBuff );
        count++;
        }
    printf( "...converted %d records\n\n", count );
    
    delete [] keyBuff;
    delete [] valueBuff;
    
    STACKDB_close( &db );
    LINEARDB_close( &dbNew );
        
    if( rename ( tempFileName, oldFileName ) != 0 ) {
        printf( "dbConvert: Failed to move temp file %s to "
                "overwrite old stackdb file %s\n",
                tempFileName, oldFileName );
        }
    

    
    return 0;
    }

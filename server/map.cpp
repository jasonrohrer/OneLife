#include "map.h"


#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include <mysql/mysql.h>

#include <stdarg.h>




static int fullMapDimension = 1000;
static int halfD = fullMapDimension/2;


static int chunkDimension = 32;

static int *map;


static int startingObjectID = 26;

static char *dbUser = NULL;
static char *dbPassword = NULL;
static char *dbServerAddress = NULL;
static char *dbDatabaseName = NULL;
static char *dbMapTableName = NULL;


static MYSQL *dbCon = NULL;




char doesTableExist( const char *inTableName ) {
    mysql_query( dbCon, "SHOW TABLES" );
    
    MYSQL_RES *result = mysql_store_result( dbCon );
            
    MYSQL_ROW row;
    while( ( row = mysql_fetch_row( result ) ) ) {
        if( strcmp( row[0], inTableName ) == 0 ) {
            return true;
            }
        }
    return false;
    }


int queryDatabase( const char *inQueryFormatString, ... ) {
    
    va_list argList;
    va_start( argList, inQueryFormatString );
    
    char *query = vautoSprintf( inQueryFormatString, argList );
    
    va_end( argList );

    
    printf( "Query =\n%s\n", query );
    
    int result = mysql_query( dbCon, query );
    
    if( result ) {
        printf( "MySQL query failed, query = %s;  error = %s\n", 
                query, mysql_error( dbCon ) );
        }
    

    delete [] query;
    
    return result;
    }




static int *getMapSpot( int inX, int inY ) {
    inX += halfD;
    inY += halfD;

    return &( map[ inY * fullMapDimension + inX ] );
    }



void initMap() {
    
    JenkinsRandomSource randSource( 10 );
    
    char *dbConnection = SettingsManager::getStringSetting( "dbConnection" );

    
    if( dbConnection != NULL ) {
        dbUser = new char[100];
        dbPassword = new char[100];
        dbServerAddress = new char[100];
        
        int numRead = sscanf( dbConnection, 
                              "%99[^:]:%99[^@]@%99s", 
                              dbUser, dbPassword, dbServerAddress );
        
        dbDatabaseName = SettingsManager::getStringSetting( "dbDatabaseName" );

        if( numRead != 3 || dbDatabaseName == NULL ) {
            printf( "Failed to read mysql connection "
                    "settings:  %s\n", dbConnection );

            delete [] dbConnection;
            }
        else {
            delete [] dbConnection;


            dbCon = mysql_init( NULL );
        
            

            if( dbCon == NULL ) {
                printf( "MySQL Init Error: %s\n", mysql_error( dbCon ) );
                return;
                }
            
            if( mysql_real_connect( dbCon, 
                                    dbServerAddress, dbUser, dbPassword,
                                    dbDatabaseName, 0, NULL, 0 ) == NULL ) {
                
                printf( "MySQL Connect Error: %s\n", mysql_error( dbCon ) );
                mysql_close( dbCon );
                return;
                }
            
            printf( "Connected to MySQL database %s\n", dbDatabaseName );
            
            // stay connected forever, until we close the connection
            mysql_query( dbCon, "SET wait_timeout = 2000000" );


            dbMapTableName = 
                SettingsManager::getStringSetting( "dbMapTableName" );

            if( dbMapTableName == NULL ) {
                dbMapTableName = stringDuplicate( "mapTriplets" );
                }

            if( !doesTableExist( dbMapTableName ) ) {
                printf( "Table %s does not exist, creating it\n",
                        dbMapTableName );
                
                queryDatabase( 
                    "CREATE TABLE %s( "
                    " x INT NOT NULL, "
                    " y INT NOT NULL, "
                    " PRIMARY KEY(x,y), "
                    " value TEXT NOT NULL ) ENGINE = MYISAM",
                    dbMapTableName );
                }
                
            }
        }
    


    int cells = fullMapDimension*fullMapDimension;
    
    map = new int[cells];

    for( int i=0; i<cells; i++ ) {
        if( randSource.getRandomBoundedInt( 0, 100 ) < 10 ) {
            map[i] = startingObjectID;
            }
        else {
            map[i] = 0;
            }
        }
    }



void freeAndNullString( char **inStringPointer ) {
    if( *inStringPointer != NULL ) {
        delete [] *inStringPointer;
        *inStringPointer = NULL;
        }
    }



void freeMap() {
    delete [] map;
    
    freeAndNullString( &dbUser );
    freeAndNullString( &dbPassword );
    freeAndNullString( &dbServerAddress );
    freeAndNullString( &dbDatabaseName );
    freeAndNullString( &dbMapTableName );    

    if( dbCon != NULL ) {
        mysql_close( dbCon );
        printf( "Closed MySQL database connection\n" );
        }
    }




// returns properly formatted chunk message for chunk centered
// around x,y
char *getChunkMessage( int inCenterX, int inCenterY ) {
    
    int chunkCells = chunkDimension * chunkDimension;
    
    int *chunk = new int[chunkCells];
    

    // 0,0 is center of map
    
    int halfChunk = chunkDimension /2;

    int worldStartX = inCenterX - halfChunk;
    int worldStartY = inCenterY - halfChunk;
    

    int startY = inCenterY + halfD - halfChunk;
    int startX = inCenterX + halfD - halfChunk;
    
    int endY = startY + chunkDimension;
    int endX = startX + chunkDimension;

    
    
    for( int y=startY; y<endY; y++ ) {
        int chunkY = y - startY;
        
        char forceZeroY = false;
        if( y < 0 || y >= fullMapDimension ) {
            forceZeroY = true;
            }

        for( int x=startX; x<endX; x++ ) {
            int chunkX = x - startX;
            
            char forceZeroX = false;
            if( x < 0 || x >= fullMapDimension ) {
                forceZeroX = true;
                }

            int i = y * fullMapDimension + x;
            int cI = chunkY * chunkDimension + chunkX;
            
            if( forceZeroY || forceZeroX ) {
                chunk[cI] = 0;
                }
            else {
                chunk[cI] = map[i];
                }
            }
        
        }
    

    // now apply updates to default from db

    int dbResult = queryDatabase( 
        "SELECT x, y, value FROM %s "
        "WHERE x >= %d AND x < %d  "
        "AND y >= %d AND y < %d",
        dbMapTableName, 
        worldStartX, worldStartX + chunkDimension, 
        worldStartY, worldStartY + chunkDimension );
    
    if( !dbResult ) {
        MYSQL_RES *result = mysql_store_result( dbCon );
        
        MYSQL_ROW row;
        
        int numRows = mysql_num_rows( result );
        printf( "Chunk contains %d db updates\n", numRows );
        
        while( ( row = mysql_fetch_row( result ) ) ) {
            
            int x, y, id;
            
            sscanf( row[0], "%d", &x );
            sscanf( row[1], "%d", &y );
            sscanf( row[2], "%d", &id );
            
            int chunkY = (y + halfD) - startY;
            int chunkX = (x + halfD) - startX;
            
            int cI = chunkY * chunkDimension + chunkX;
            chunk[cI] = id;
            }
        
        mysql_free_result( result );
        }
    


    char *header = autoSprintf( "MAP_CHUNK\n%d %d %d\n", chunkDimension,
                                worldStartX, worldStartY );
    
    SimpleVector<char> buffer;
    buffer.appendElementString( header );
    delete [] header;
    
    
    for( int i=0; i<chunkCells; i++ ) {
        
        if( i > 0 ) {
            buffer.appendElementString( " " );
            }
        

        char *cell = autoSprintf( "%d", chunk[i] );
        
        buffer.appendElementString( cell );
        delete [] cell;
        }
    buffer.appendElementString( "#" );
    
    
    delete [] chunk;

    return buffer.getElementString();
    }









int getMapObject( int inX, int inY ) {
    
    int dbResult = queryDatabase( "SELECT value FROM %s WHERE x=%d AND y=%d",
                                  dbMapTableName, inX, inY );
    
    int id = -1;

    if( !dbResult ) {
        MYSQL_RES *result = mysql_store_result( dbCon );
        
        
        if( mysql_num_rows( result ) == 1 ) {
            MYSQL_ROW row = mysql_fetch_row( result );
            
            sscanf( row[0], "%d", &id );
            }
        
        mysql_free_result( result );
        }
    
    if( id != -1 ) {
        return id;
        }
    else {
        return *getMapSpot( inX, inY );
        }
    }




void setMapObject( int inX, int inY, int inID ) {
    *getMapSpot( inX, inY ) = inID;

    
    queryDatabase( "INSERT INTO %s "
                   "  SET x=%d, y=%d, value='%d' "
                   "ON DUPLICATE KEY UPDATE "
                   "  value='%d' ",
                   dbMapTableName, inX, inY, inID, inID );
    }



#include "map.h"


#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"




static int fullMapDimension = 1000;
static int halfD = fullMapDimension/2;


static int chunkDimension = 32;

static int *map;


static int startingObjectID = 26;




static int *getMapSpot( int inX, int inY ) {
    inX += halfD;
    inY += halfD;

    return &( map[ inY * fullMapDimension + inX ] );
    }



void initMap() {
    
    JenkinsRandomSource randSource( 10 );


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



void freeMap() {
    delete [] map;
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

    char forceZero = false;
    
    for( int y=startY; y<endY; y++ ) {
        int chunkY = y - startY;
        
        if( y < 0 || y >= fullMapDimension ) {
            forceZero = true;
            }

        for( int x=startX; x<endX; x++ ) {
            int chunkX = x - startX;
            
            if( x < 0 || x >= fullMapDimension ) {
                forceZero = true;
                }

            int i = y * fullMapDimension + x;
            int cI = chunkY * chunkDimension + chunkX;
            
            if( forceZero ) {
                chunk[cI] = 0;
                }
            else {
                chunk[cI] = map[i];
                }
            }
        
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
    return *getMapSpot( inX, inY );
    }




void setMapObject( int inX, int inY, int inID ) {
    *getMapSpot( inX, inY ) = inID;
    }



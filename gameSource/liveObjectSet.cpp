#include "spriteBank.h"

#include "objectBank.h"

#include "transitionBank.h"


#include "minorGems/util/SimpleVector.h"




static SimpleVector<int> liveObjectSet;
static SimpleVector<int> liveSpriteSet;


static int objectMapSize = 0;
static int spriteMapSize = 0;

static char *liveObjectIDMap = NULL;

static char *liveSpriteIDMap = NULL;




void initLiveObjectSet() {
    objectMapSize = getMaxObjectID() + 1;
    spriteMapSize = getMaxSpriteID() + 1;

    liveObjectIDMap = new char[ objectMapSize ];
    liveSpriteIDMap = new char[ spriteMapSize ];
    }



void freeLiveObjectSet() {
    if( liveObjectIDMap != NULL ) {
        delete [] liveObjectIDMap;
        liveObjectIDMap = NULL;
        }
    if( liveSpriteIDMap != NULL ) {
        delete [] liveSpriteIDMap;
        liveSpriteIDMap = NULL;
        }
    }



void clearLiveObjectSet() {
    memset( liveObjectIDMap, false, objectMapSize );
    memset( liveSpriteIDMap, false, spriteMapSize );
    
    liveObjectSet.deleteAll();
    liveSpriteSet.deleteAll();
    }



// adds an object to the base set of live objects
// objects one transition step away will be auto-added as well  
void addBaseObjectToLiveObjectSet( int inID ) {
    if( ! liveObjectIDMap[ inID ] ) {
        
        liveObjectIDMap[ inID ] = true;
        
        liveObjectSet.push_back( inID );
        
        ObjectRecord *o = getObject( inID );

        for( int j=0; j< o->numSprites; j++ ) {
            int spriteID = o->sprites[j];
            
            if( ! liveSpriteIDMap[ spriteID ] ) {

                liveSpriteIDMap[ spriteID ] = true;
                liveSpriteSet.push_back( spriteID );
                }
            }

        }
    }




void finalizeLiveObjectSet() {
    for( int i=0; i<liveObjectSet.size(); i++ ) {
        
        
        }
    

    int numSprites = liveSpriteSet.size();
    
    for( int i=0; i<numSprites; i++ ) {
        int id = liveSpriteSet.getElementDirect(i);
        
        markSpriteLive( id );
        }
    
    }





char isLiveObjectSetFullyLoaded( float *outProgress ) {

    
    int numDone = 0;

    int num = liveSpriteSet.size();
    for( int i=0; i<num; i++ ) {
        
        int id = liveSpriteSet.getElementDirect( i );
        
        if( markSpriteLive( id ) ) {
            numDone ++;
            }
        }

    *outProgress = (float)numDone / (float)num;

    return ( numDone == num );
    }



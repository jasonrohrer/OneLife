#include "spriteBank.h"

#include "objectBank.h"

#include "transitionBank.h"

#include "liveObjectSet.h"


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

    clearLiveObjectSet();
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
    
    // follow transitions one step and add any new objects at end of our
    // list.

    // But DON'T keep going through them in list and processing THEIR 
    // transitions (that would hit all reachable objects).

    // so process up to the base set size and then stop, even though
    // our list is growing
    int baseSetSize = liveObjectSet.size();
    
    for( int i=0; i<baseSetSize; i++ ) {
        int id = liveObjectSet.getElementDirect( i );
        
        SimpleVector<TransRecord*> *list = getAllUses( id );
        
        if( list != NULL ) {
            
            int num = list->size();
            
            for( int j=0; j<num; j++ ) {
                TransRecord *t = list->getElementDirect( j );
                
                addBaseObjectToLiveObjectSet( t->newActor );
                addBaseObjectToLiveObjectSet( t->newTarget );            
                }
            }
        
        }
    

    int numSprites = liveSpriteSet.size();

    //printf( "Finalizing an object set resulting in %d live sprites\n",
    //        numSprites );
    
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



#include "spriteBank.h"

#include "objectBank.h"

#include "soundBank.h"

#include "transitionBank.h"

#include "animationBank.h"

#include "liveObjectSet.h"


#include "minorGems/util/SimpleVector.h"




static SimpleVector<int> liveObjectSet;
static SimpleVector<int> liveSpriteSet;
static SimpleVector<int> liveSoundSet;


static int objectMapSize = 0;
static int spriteMapSize = 0;
static int soundMapSize = 0;

static char *liveObjectIDMap = NULL;

static char *liveSpriteIDMap = NULL;

static char *liveSoundIDMap = NULL;




void initLiveObjectSet() {
    objectMapSize = getMaxObjectID() + 1;
    spriteMapSize = getMaxSpriteID() + 1;
    soundMapSize = getMaxSoundID() + 1;

    liveObjectIDMap = new char[ objectMapSize ];
    liveSpriteIDMap = new char[ spriteMapSize ];
    liveSoundIDMap = new char[ soundMapSize ];

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
    if( liveSoundIDMap != NULL ) {
        delete [] liveSoundIDMap;
        liveSoundIDMap = NULL;
        }
    }



void clearLiveObjectSet() {
    memset( liveObjectIDMap, false, objectMapSize );
    memset( liveSpriteIDMap, false, spriteMapSize );
    memset( liveSoundIDMap, false, soundMapSize );
    
    liveObjectSet.deleteAll();
    liveSpriteSet.deleteAll();
    liveSoundSet.deleteAll();
    }



static void markSoundUsageLiveInternal( SoundUsage inUsage ) {
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        liveSoundIDMap[ inUsage.ids[i] ] = true;
        liveSoundSet.push_back( inUsage.ids[i] );
        }
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
        

        markSoundUsageLiveInternal( o->creationSound );
        markSoundUsageLiveInternal( o->usingSound );
        markSoundUsageLiveInternal( o->eatingSound );
        markSoundUsageLiveInternal( o->decaySound );
        

        for( int t=ground; t<endAnimType; t += 1 ) {
            AnimationRecord *r = getAnimation( inID, (AnimType)t );
            
            if( r != NULL ) {
                for( int s=0; s<r->numSounds; s++ ) {                    
                    markSoundUsageLiveInternal( r->soundAnim[s].sound );
                    }
                }
            }
        
        }
    }




void finalizeLiveObjectSet() {
    
    // monument call object sounds always part of set
    SimpleVector<int> *monumentCallIDs = getMonumentCallObjects();
    int numMon = monumentCallIDs->size();
    for( int i=0; i<numMon; i++ ) {
        int id = monumentCallIDs->getElementDirect(i);
        markSoundUsageLiveInternal( getObject( id )->creationSound  );
        }
    


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
                
                if( t->newActor > 0 ) {
                    addBaseObjectToLiveObjectSet( t->newActor );
                    }
                
                if( t->newTarget > 0 ) {
                    addBaseObjectToLiveObjectSet( t->newTarget );
                    }
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


    int numSounds = liveSoundSet.size();

    //printf( "Finalizing an object set resulting in %d live sounds\n",
    //        numSounds );
    
    for( int i=0; i<numSounds; i++ ) {
        int id = liveSoundSet.getElementDirect(i);
        
        markSoundLive( id );
        }
    
    }





char isLiveObjectSetFullyLoaded( float *outProgress ) {
    
    int numDone = 0;

    int numSprites = liveSpriteSet.size();
    for( int i=0; i<numSprites; i++ ) {
        
        int id = liveSpriteSet.getElementDirect( i );
        
        if( markSpriteLive( id ) ) {
            numDone ++;
            }
        }

    int numSounds = liveSoundSet.size();
    for( int i=0; i<numSounds; i++ ) {
        
        int id = liveSoundSet.getElementDirect( i );
        
        if( markSoundLive( id ) ) {
            numDone ++;
            }
        }

    int numTotal = numSprites + numSounds;

    *outProgress = (float)numDone / (float)numTotal;

    return ( numDone == numTotal );
    }



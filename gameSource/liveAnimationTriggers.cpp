#include "liveAnimationTriggers.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include <stdio.h>


static char enabled = false;

static char triggerKey = '@';


static const char *triggerFileName = "liveTriggers.txt";


typedef struct Trigger {
        AnimType anim;
        int extraAnimIndex;
        int stepDuration;
        int numStepsPlayed;
    } Trigger;


static SimpleVector<Trigger> triggers;


static Trigger nullTrigger = { endAnimType, -1, 0, 0 };

static Trigger currentTrigger = nullTrigger;

    

void initLiveTriggers() {
    enabled = SettingsManager::getIntSetting( "enableLiveTriggers", 0 );

    if( !enabled ) return;
    

    FILE *f = fopen( triggerFileName, "r" );
    
    if( f == NULL ) {
        printf( "Failed to read live anim trigger file %s\n", triggerFileName );
        enabled = false;
        return;
        }
    
    int numRead = 2;
    
    // switch back and forth between these
    // so that triggered animations automatically blend between each other
    AnimType anim1 = extra;
    AnimType anim2 = extraB;

    while( numRead == 2 ) {
        Trigger t;
        

        numRead = fscanf( f, "%d %d", 
                          &( t.extraAnimIndex ), &( t.stepDuration ) );
        
        if( numRead == 2 ) {
            t.numStepsPlayed = 0;
            t.anim = anim1;
            
            // swap
            AnimType temp = anim1;
            anim1 = anim2;
            anim2 = temp;

            triggers.push_back( t );
            }
        }
    
    printf( "Read %d triggers from anim trigger file %s\n",
            triggers.size(), triggerFileName );

    fclose( f );    
    }

    


void freeLiveTriggers() {
    if( !enabled ) return;

    }



// send in a key command received from user that make trigger an animation
void registerTriggerKeyCommand( unsigned char inASCII ) {
    if( !enabled ) return;
    
    if( inASCII == triggerKey && triggers.size() > 0 ) {
        
        currentTrigger = triggers.getElementDirect( 0 );
        
        triggers.deleteElement( 0 );
        }
    }




AnimType stepLiveTriggers( char *outNew ) {
    if( !enabled ) return endAnimType;
    
    if( currentTrigger.extraAnimIndex > -1 ) {

        if( currentTrigger.numStepsPlayed == 0 ) {
            *outNew = true;
            }
        else {
            *outNew = false;
            }
        
        currentTrigger.numStepsPlayed ++;
        
        if( currentTrigger.numStepsPlayed > currentTrigger.stepDuration ) {
            currentTrigger = nullTrigger;
            }


        if( currentTrigger.anim == extra ) {
            setExtraIndex( currentTrigger.extraAnimIndex );
            }
        else if( currentTrigger.anim == extraB ) {
            setExtraIndexB( currentTrigger.extraAnimIndex );
            }        
        }

    return currentTrigger.anim;    
    }


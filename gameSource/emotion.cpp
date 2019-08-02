#include "emotion.h"
#include "liveObjectSet.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"


SimpleVector<Emotion> emotions;


void initEmotion() {
    char *cont = SettingsManager::getSettingContents( "emotionWords", "" );
    
    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        return;    
        }

    int numParts;
    char **parts = split( cont, "\n", &numParts );
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            
            Emotion e = { stringToUpperCase( parts[i] ), 0, 0, 0 };

            emotions.push_back( e );
            }
        delete [] parts[i];
        }
    delete [] parts;


    // now read emotion objects

    cont = SettingsManager::getSettingContents( "emotionObjects", "" );
    
    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        return;    
        }

    parts = split( cont, "\n", &numParts );
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            
            Emotion *e;
            
            if( i < emotions.size() ) {
                e = emotions.getElement( i );
                }
            else {
                // the list extends beyond emotion words
                // put dummy trigger in place for these
                // * is a character that the end user cannot type
                Emotion eNew = { stringDuplicate( "DUMMY*TRIGGER" ), 0, 0, 0 };

                emotions.push_back( eNew );
                e = emotions.getElement( emotions.size() - 1 );
                }
            e->eyeEmot = 0;
            e->mouthEmot = 0;
            e->otherEmot = 0;
            e->faceEmot = 0;
            e->bodyEmot = 0;
            e->headEmot = 0;
            

            sscanf( parts[i], "%d %d %d %d %d %d", 
                    &( e->eyeEmot ), 
                    &( e->mouthEmot ), 
                    &( e->otherEmot ),
                    &( e->faceEmot ),
                    &( e->bodyEmot ),
                    &( e->headEmot ) );
            }
        delete [] parts[i];
        }
    delete [] parts;
    
    }



void freeEmotion() {
    for( int i=0; i<emotions.size(); i++ ) {
        delete [] emotions.getElementDirect(i).triggerWord;
        }
    emotions.deleteAll();
    }



// -1 if no emotion triggered
int getEmotionIndex( const char *inSpeech ) {
    char *upperSpeech = stringToUpperCase( inSpeech );
    
    for( int i=0; i<emotions.size(); i++ ) {
        
        if( strstr( upperSpeech, emotions.getElement(i)->triggerWord ) ==
            upperSpeech ) {
            
            // starts with trigger
            delete [] upperSpeech;
            return i;
            }
        }
    
    delete [] upperSpeech;
    return -1;
    }



// returns NULL if index out of range
Emotion *getEmotion( int inIndex ) {
    if( inIndex < 0 || inIndex >= emotions.size() ) {
        return NULL;
        }
    return emotions.getElement( inIndex );
    }



void markEmotionsLive() {
    for( int i=0; i<emotions.size(); i++ ) {

        Emotion *e = emotions.getElement( i );

        if( e->eyeEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->eyeEmot );
            }
        if( e->mouthEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->mouthEmot );
            }
        if( e->otherEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->otherEmot );
            }
        if( e->faceEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->faceEmot );
            }
        if( e->bodyEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->bodyEmot );
            }
        if( e->headEmot > 0 ) {    
            addBaseObjectToLiveObjectSet( e->headEmot );
            }
        }
    }



int getEmotionNumObjectSlots() {
    return 6;
    }


int getEmotionObjectByIndex( Emotion *inEmote, int inIndex ) {
    if( inIndex < 0 || inIndex >= 6 ) {
        return 0;
        }
    switch( inIndex ) {
        case 0:
            return inEmote->eyeEmot;
        case 1:
            return inEmote->mouthEmot;
        case 2:
            return inEmote->otherEmot;
        case 3:
            return inEmote->faceEmot;
        case 4:
            return inEmote->bodyEmot;
        case 5:
            return inEmote->headEmot;
        default:
            return 0;
        }    
    }


        

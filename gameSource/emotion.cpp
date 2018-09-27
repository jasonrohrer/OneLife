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
        if( i < emotions.size() && strcmp( parts[i], "" ) != 0 ) {
            
            Emotion *e = emotions.getElement( i );
            
            sscanf( parts[i], "%d %d %d", 
                    &( e->eyeEmot ), 
                    &( e->mouthEmot ), 
                    &( e->otherEmot ) );
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
        }
    }

        

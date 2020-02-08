#include "message.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"

#include "minorGems/util/stringUtils.h"



extern Font *mainFont;

static TextAlignment messageAlign = alignCenter;

void setMessageAlign( TextAlignment inAlign ) {
    messageAlign = inAlign;
    }

TextAlignment getMessageAlign() {
    return messageAlign;
    }



void drawMessage( const char *inTranslationKey, doublePair inCenter,
                  char inRed, double inFade ) {

    const char *inMessage = translate( inTranslationKey );
    
    

    if( inRed ) {
        setDrawColor( 1, 0, 0, inFade );
        }
    else {
        setDrawColor( 1, 1, 1, inFade );
        }
    

    if( strstr( inMessage, "##" ) != NULL ) {
        // multi-line message
            
        int numSubMessages;
        
        char **subMessages = split( inMessage, "##", &numSubMessages );
            
        
        doublePair thisMessagePos = inCenter;
        
        for( int i=0; i < numSubMessages; i++ ) {
            
            if( strcmp( subMessages[i], "~~" ) == 0 ) {
                // half-skip
                thisMessagePos.y += 15;
                }
            else {
                mainFont->drawString( subMessages[i], 
                                      thisMessagePos, messageAlign );
                }
            delete [] subMessages[i];

            thisMessagePos.y -= 30;
            }

        delete [] subMessages;
        }
    else {
        // just a single message

        mainFont->drawString( inMessage, inCenter, messageAlign );
        }

    }


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
            
            
        for( int i=numSubMessages-1; i>= 0; i-- ) {
                
            doublePair thisMessagePos = inCenter;
                
            thisMessagePos.y -= i * 30;
                
            mainFont->drawString( subMessages[i], 
                                  thisMessagePos, messageAlign );
                
            delete [] subMessages[i];
            }

        delete [] subMessages;
        }
    else {
        // just a single message

        mainFont->drawString( inMessage, inCenter, messageAlign );
        }

    }


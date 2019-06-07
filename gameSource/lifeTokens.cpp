#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "message.h"

extern Font *mainFont;
extern Font *numbersFontFixed;



void initLifeTokens() {
    }


void freeLifeTokens() {
    }



void triggerLifeTokenUpdate() {
    }



int getNumLifeTokens() {
    return -1;
    }


char *getLifeTokenString() {
    return autoSprintf( translate( "tokenMessage" ), 3, 3600/60, 12 );
    }


void getLifeTokenTime( int *outHours, int *outMinutes, int *outSeconds ) {
    *outHours = 0;
    *outMinutes = 2;
    *outSeconds = 3;
    }





void drawTokenMessage( doublePair inPos ) {
    int numTokens = getNumLifeTokens();

    if( numTokens != -1 ) {
        
        if( numTokens > 0 ) {
            char *message = getLifeTokenString();
            
            drawMessage( message, inPos );
            
            delete [] message;
            }
        else if( numTokens == 0 ) {
            int h, m, s;
            
            getLifeTokenTime( &h, &m, &s );

            setDrawColor( 1, 1, 1, 1.0 );
            mainFont->drawString( translate( "tokenTimeMessage" ), 
                                  inPos, alignRight );

            char *timeString = autoSprintf( "%d:%02d:%02d", h, m, s );
            
            numbersFontFixed->drawString( timeString, 
                                          inPos, alignLeft );

            delete [] timeString;
            }
        }
    
    }



#include "keyLegend.h"

#include "minorGems/game/Font.h"


#include "minorGems/util/stringUtils.h"




extern Font *smallFont;


void addKeyDescription( KeyLegend *inLegend,
                        char inKey,
                        const char *inDescription ) {

    inLegend->keys.push_back( inKey );
    inLegend->keyClassNames.push_back( NULL );
    inLegend->descriptions.push_back( inDescription );
    }



void addKeyClassDescription( KeyLegend *inLegend,
                             const char *inKeyClassName,
                             const char *inDescription ) {
    
    inLegend->keys.push_back( '\0' );
    inLegend->keyClassNames.push_back( inKeyClassName );
    inLegend->descriptions.push_back( inDescription );
    }



void drawKeyLegend( KeyLegend *inLegend, doublePair inPos,
                    TextAlignment inAlign ) {
    setDrawColor( 1, 1, 1, 1 );
    
    int numKeys = inLegend->keys.size();
    

    for( int i=0; i<numKeys; i++ ) {
        
        char *string;
        
        char key = inLegend->keys.getElementDirect( i );
        const char *keyClassName = 
            inLegend->keyClassNames.getElementDirect( i );
        const char *description = inLegend->descriptions.getElementDirect( i );
        
        if( key == '\0' ) {
            string = autoSprintf( "%s = %s", keyClassName, description );
            }
        else {
            string = autoSprintf( "%c = %s", key, description );
            }

        smallFont->drawString( string, inPos, inAlign );

        delete [] string;

        inPos.y -= 16;
        }
    }

    
    

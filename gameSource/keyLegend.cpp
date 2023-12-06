#include "keyLegend.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"


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

    double spacing = 16;    

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

        double width = smallFont->measureString( string );
        
        // darker transparent background box under each line
        setDrawColor( 0, 0, 0, 0.6 );
        
        int margin = 4;

        if( inAlign == alignCenter ) {
            drawRect( inPos, width/2 + margin, spacing / 2 );
            }
        else if( inAlign == alignLeft ) {
            drawRect( inPos.x - margin, inPos.y + spacing/2, 
                      inPos.x + width + margin, inPos.y - spacing/2 );
            }
        else if( inAlign == alignRight ) {
            drawRect( inPos.x - width - margin, inPos.y + spacing/2,
                      inPos.x + margin, inPos.y - spacing/2 );
            }
        
            
        setDrawColor( 1, 1, 1, 1 );
        smallFont->drawString( string, inPos, inAlign );

        delete [] string;

        inPos.y -= spacing;
        }
    }

    
    

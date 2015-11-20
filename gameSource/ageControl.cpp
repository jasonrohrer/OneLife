#include "ageControl.h"

doublePair getAgeHeadOffset( double inAge, doublePair inHeadSpritePos ) {
    if( inAge == -1 ) {
        return (doublePair){ 0, 0 };
        }
    
    if( inAge < 20 ) {
        
        double maxHead = inHeadSpritePos.y;
        
        double yOffset = ( ( 20 - inAge ) / 20 ) * .5 * maxHead;
        
        
        return (doublePair){ 0, -yOffset };
        }
    

    if( inAge >= 40 ) {
        
        if( inAge > 60 ) {
            // no worse after 60
            inAge = 60;
            }

        double maxHead = inHeadSpritePos.y;
        
        double offset = ( ( inAge - 40 ) / 20 ) * .5 * maxHead;
        
        
        return (doublePair){ offset, -offset };
        }

    return (doublePair){ 0, 0 };
    }



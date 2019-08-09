#include "ageControl.h"

#include <math.h>


static double babyHeadDownFactor = 0.6;

static double babyBodyDownFactor = 0.75;

static double oldHeadDownFactor = 0.35;

static double oldHeadForwardFactor = 2;

//UncleGus Changes
static double deathAge = 120.0;

static double adultAge = 20.0;

static double oldAge = 100.0;

//End UncleGus Changes

#include "minorGems/util/SettingsManager.h"


void initAgeControl() {
    babyHeadDownFactor = 
        SettingsManager::getFloatSetting( "babyHeadDownFactor", 0.6 );
    
    babyBodyDownFactor = 
        SettingsManager::getFloatSetting( "babyBodyDownFactor", 0.75 );

    oldHeadDownFactor = 
        SettingsManager::getFloatSetting( "oldHeadDownFactor", 0.35 );

    oldHeadForwardFactor = 
        SettingsManager::getFloatSetting( "oldHeadForwardFactor", 2 );
    
    }




doublePair getAgeHeadOffset( double inAge, doublePair inHeadSpritePos,
                             doublePair inBodySpritePos,
                             doublePair inFrontFootSpritePos ) {
    if( inAge == -1 ) {
        return (doublePair){ 0, 0 };
        }
    
    // UncleGus change
      if( inAge < adultAge ) {
    // End UncleGus change
        
        double maxHead = inHeadSpritePos.y - inBodySpritePos.y;
        
        //UncleGus change
                double yOffset = ( ( adultAge - inAge ) / adultAge ) * babyHeadDownFactor * maxHead;
        // End UncleGus change
        
        
        return (doublePair){ 0, round( -yOffset ) };
        }
    

		// UncleGus Change
		 if( inAge >= oldAge ) {

				if( inAge > deathAge ) {
					// no worse after 120
					inAge = deathAge;
					}
		// End UncleGus change

        double maxHead = inHeadSpritePos.y - inBodySpritePos.y;
        
		// UncleGus Change
				double vertOffset =
					( ( inAge - oldAge ) / adultAge ) * oldHeadDownFactor * maxHead;
		// End UncleGus change
        
        double footOffset = inFrontFootSpritePos.x - inHeadSpritePos.x;
        
		// UncleGus Change	
				double forwardOffset =
				   ( ( inAge - oldAge ) / adultAge ) * oldHeadForwardFactor * footOffset;
		// End UncleGus Change

        return (doublePair){ round( forwardOffset ), round( -vertOffset ) };
        }

    return (doublePair){ 0, 0 };
    }



doublePair getAgeBodyOffset( double inAge, doublePair inBodySpritePos ) {
    if( inAge == -1 ) {
        return (doublePair){ 0, 0 };
        }
    
	// UncleGus Change
		if( inAge < adultAge ) {
	// End UncleGus Change
        
        double maxBody = inBodySpritePos.y;
    
	// UncleGus Change
        double yOffset = ( ( adultAge - inAge ) / adultAge ) * babyBodyDownFactor * maxBody;
	// End UncleGus Change
        
        
        return (doublePair){ 0, round( -yOffset ) };
        }

    return (doublePair){ 0, 0 };
    }


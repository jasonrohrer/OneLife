#include "ageControl.h"

#include <math.h>


static double babyHeadDownFactor = 0.6;

static double babyBodyDownFactor = 0.75;

static double oldHeadDownFactor = 0.35;

static double oldHeadForwardFactor = 2;


#include "minorGems/util/SettingsManager.h"

float lifespan_multiplier = 1.0f;
int age_baby = 5;
int age_fertile = 14;
int age_mature = 20;
int age_old = (int)( 40 * lifespan_multiplier );
int age_death = (int)( 60 * lifespan_multiplier );


void sanityCheckSettings() {
    FILE *fp = SettingsManager::getSettingsFile( "lifespanMultiplier", "r" );
	if( fp == NULL ) {
		fp = SettingsManager::getSettingsFile( "lifespanMultiplier", "w" );
        SettingsManager::setSetting( "lifespanMultiplier", 1.0f );
	}
    fclose( fp );
}


void initAgeControl() {
    babyHeadDownFactor = 
        SettingsManager::getFloatSetting( "babyHeadDownFactor", 0.6 );
    
    babyBodyDownFactor = 
        SettingsManager::getFloatSetting( "babyBodyDownFactor", 0.75 );

    oldHeadDownFactor = 
        SettingsManager::getFloatSetting( "oldHeadDownFactor", 0.35 );

    oldHeadForwardFactor = 
        SettingsManager::getFloatSetting( "oldHeadForwardFactor", 2 );
    
    sanityCheckSettings();
    lifespan_multiplier = SettingsManager::getFloatSetting( "lifespanMultiplier", 1.0f );
    age_old = (int)( 40 * lifespan_multiplier );
    age_death = (int)( 60 * lifespan_multiplier );
    }




doublePair getAgeHeadOffset( double inAge, doublePair inHeadSpritePos,
                             doublePair inBodySpritePos,
                             doublePair inFrontFootSpritePos ) {
    if( inAge == -1 ) {
        return (doublePair){ 0, 0 };
        }
    
    if( inAge < age_mature ) {
        
        double maxHead = inHeadSpritePos.y - inBodySpritePos.y;
        
        double yOffset = ( ( age_mature - inAge ) / age_mature ) * babyHeadDownFactor * maxHead;
        
        
        return (doublePair){ 0, round( -yOffset ) };
        }
    

    if( inAge >= age_old ) {
        
        if( inAge > age_death ) {
            // no worse after age_death
            inAge = age_death;
            }

        double maxHead = inHeadSpritePos.y - inBodySpritePos.y;
        
        double vertOffset = 
            ( ( inAge - age_old ) / age_mature ) * oldHeadDownFactor * maxHead;
        
        double footOffset = inFrontFootSpritePos.x - inHeadSpritePos.x;
        
        double forwardOffset = 
            ( ( inAge - age_old ) / age_mature ) * oldHeadForwardFactor * footOffset;

        return (doublePair){ round( forwardOffset ), round( -vertOffset ) };
        }

    return (doublePair){ 0, 0 };
    }



doublePair getAgeBodyOffset( double inAge, doublePair inBodySpritePos ) {
    if( inAge == -1 ) {
        return (doublePair){ 0, 0 };
        }
    
    if( inAge < age_mature ) {
        
        double maxBody = inBodySpritePos.y;
        
        double yOffset = ( ( age_mature - inAge ) / age_mature ) * babyBodyDownFactor * maxBody;
        
        
        return (doublePair){ 0, round( -yOffset ) };
        }

    return (doublePair){ 0, 0 };
    }


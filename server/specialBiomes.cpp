#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "../gameSource/objectBank.h"

#include "specialBiomes.h"
#include "map.h"


typedef struct SpecialBiome {
        // -1 if not set
        int specialistRace;
        // -1 if not set
        int sicknessObjectID;
    } SpecialBiome;


// assume not more than 100 biomes;

// mapping biome numbers 
#define MAX_BIOME_NUMBER 100
static SpecialBiome specialBiomes[ MAX_BIOME_NUMBER ];

static int curNumPlayers;
static int minNumPlayers;


void initSpecialBiomes() {
    updateSpecialBiomes( 0 );
    }


void freeSpecialBiomes() {
    }



// reload settings
void updateSpecialBiomes( int inNumPlayers ) {
    curNumPlayers = inNumPlayers;

    minNumPlayers = 
        SettingsManager::getIntSetting( 
            "minActivePlayersForSpecialBiomes", 15 );
    
    for( int i=0; i < MAX_BIOME_NUMBER; i++ ) {
        specialBiomes[i].specialistRace = -1;
        specialBiomes[i].sicknessObjectID = -1;
        }
    
    SimpleVector<char *> *parts = 
        SettingsManager::getSetting( "raceSpecialBiomes" );

    if( parts->size() % 2 == 0 ) {
        
        for( int i=0; i<parts->size() - 1; i += 2 ) {
            char *letter = parts->getElementDirect( i );
            char *number = parts->getElementDirect( i + 1 );

            int raceNumber = letter[0] - 'A' + 1;
            
            int biomeNumber = 0;
            sscanf( number, "%d", &biomeNumber );
            
            if( biomeNumber >= 0 && biomeNumber < MAX_BIOME_NUMBER ) {
                specialBiomes[biomeNumber].specialistRace = raceNumber;
                }
            }
        }
    parts->deallocateStringElements();
    
    delete parts;
    
    
    parts = 
        SettingsManager::getSetting( "specialBiomeSicknesses" );

    if( parts->size() % 2 == 0 ) {
        
        for( int i=0; i<parts->size() - 1; i += 2 ) {
            char *numberA = parts->getElementDirect( i );
            char *numberB = parts->getElementDirect( i + 1 );
            
            int biomeNumber = 0;
            sscanf( numberA, "%d", &biomeNumber );

            int sickID = 0;
            sscanf( numberB, "%d", &sickID );
            
            if( biomeNumber >= 0 && biomeNumber < MAX_BIOME_NUMBER &&
                getObject( sickID ) != NULL ) {
                specialBiomes[biomeNumber].sicknessObjectID = sickID;
                }
            }
        }
    parts->deallocateStringElements();
    }



static SpecialBiome defaultRecord = { -1, -1 };


static SpecialBiome getBiomeRecord( int inX, int inY ) {
    int b = getMapBiome( inX, inY );

    if( b < 0 || b >= MAX_BIOME_NUMBER ) {
        return defaultRecord;
        }
    
    return specialBiomes[ b ];
    }


char isBiomeAllowed( int inDisplayID, int inX, int inY ) {
    if( curNumPlayers < minNumPlayers ) {
        return true;
        }
    
    ObjectRecord *o = getObject( inDisplayID );
    
    if( o->race == 0 ) {
        return true;
        }

    SpecialBiome r = getBiomeRecord( inX, inY );

    if( r.specialistRace == -1 || r.specialistRace == o->race ) {
        return true;
        }

    // floor blocks
    if( getMapFloor( inX, inY ) != 0 ) {
        return true;
        }
    
    return false;
    }



int getBiomeSickness( int inDisplayID, int inX, int inY ) {
    if( curNumPlayers < minNumPlayers ) {
        return -1;
        }
    
    ObjectRecord *o = getObject( inDisplayID );
    
    if( o->race == 0 ) {
        return -1;
        }

    
    SpecialBiome r = getBiomeRecord( inX, inY );

    if( r.specialistRace == -1 || r.specialistRace == o->race ) {
        return -1;
        }

    // floor blocks
    if( getMapFloor( inX, inY ) != 0 ) {
        return -1;
        }
    
    return r.sicknessObjectID;
    }


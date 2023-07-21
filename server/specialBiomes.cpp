#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "../gameSource/objectBank.h"

#include "specialBiomes.h"
#include "map.h"


typedef struct SpecialBiome {
        // -1 if not set
        int specialistRace;
        // -1 if not set
        int sicknessObjectID;
        
        // -1 if not set
        int reliefEmotIndex;
        
        // NULL if not set
        char *badBiomeName;
    } SpecialBiome;


SimpleVector<int> polylingualRaces;


// assume not more than 100 biomes;

// mapping biome numbers 
#define MAX_BIOME_NUMBER 100
static SpecialBiome specialBiomes[ MAX_BIOME_NUMBER ];

static int curNumPlayers;
static int minNumPlayers;

static int minNumPlayersForLanguages;



void initSpecialBiomes() {
    for( int i=0; i < MAX_BIOME_NUMBER; i++ ) {
        specialBiomes[i].badBiomeName = NULL;
        }

    updateSpecialBiomes( 0 );
    }


void freeSpecialBiomes() {
    for( int i=0; i < MAX_BIOME_NUMBER; i++ ) {
        if( specialBiomes[i].badBiomeName != NULL ) {
            delete [] specialBiomes[i].badBiomeName;
            specialBiomes[i].badBiomeName = NULL;
            }
        }
    }



// reload settings
char updateSpecialBiomes( int inNumPlayers ) {
    char wasActive = specialBiomesActive();
    
    
    curNumPlayers = inNumPlayers;

    minNumPlayers = 
        SettingsManager::getIntSetting( 
            "minActivePlayersForSpecialBiomes", 15 );

    minNumPlayersForLanguages = 
        SettingsManager::getIntSetting( 
            "minActivePlayersForLanguages", 15 );

    polylingualRaces.deleteAll();
    
    for( int i=0; i < MAX_BIOME_NUMBER; i++ ) {
        specialBiomes[i].specialistRace = -1;
        specialBiomes[i].sicknessObjectID = -1;
        specialBiomes[i].reliefEmotIndex = -1;
        if( specialBiomes[i].badBiomeName != NULL ) {
            delete [] specialBiomes[i].badBiomeName;
            specialBiomes[i].badBiomeName = NULL;
            }
        }
    
    SimpleVector<char *> *parts = 
        SettingsManager::getSetting( "raceSpecialBiomes" );

    if( parts->size() % 3 == 0 ) {
        
        for( int i=0; i<parts->size() - 1; i += 3 ) {
            char *letter = parts->getElementDirect( i );
            char *number = parts->getElementDirect( i + 1 );
            char *name = parts->getElementDirect( i + 2 );

            int raceNumber = letter[0] - 'A' + 1;
            
            int biomeNumber = 0;
            sscanf( number, "%d", &biomeNumber );
            
            if( biomeNumber >= 0 && biomeNumber < MAX_BIOME_NUMBER ) {
                specialBiomes[biomeNumber].specialistRace = raceNumber;
                specialBiomes[biomeNumber].badBiomeName = 
                    stringDuplicate( name );
                }
            else if( biomeNumber == -1 ) {
                polylingualRaces.push_back( raceNumber );
                }
            }
        }
    parts->deallocateStringElements();
    
    delete parts;
    
    
    parts = 
        SettingsManager::getSetting( "specialBiomeSicknesses" );

    if( parts->size() % 3 == 0 ) {
        
        for( int i=0; i<parts->size() - 1; i += 3 ) {
            char *numberA = parts->getElementDirect( i );
            char *numberB = parts->getElementDirect( i + 1 );
            char *numberC = parts->getElementDirect( i + 2 );
            
            int biomeNumber = 0;
            sscanf( numberA, "%d", &biomeNumber );

            int sickID = 0;
            sscanf( numberB, "%d", &sickID );

            int reliefIndex = -1;
            sscanf( numberC, "%d", &reliefIndex );
            
            if( biomeNumber >= 0 && biomeNumber < MAX_BIOME_NUMBER &&
                getObject( sickID ) != NULL ) {
                specialBiomes[biomeNumber].sicknessObjectID = sickID;
                specialBiomes[biomeNumber].reliefEmotIndex = reliefIndex;
                }
            }
        }
    parts->deallocateStringElements();
    
    delete parts;

    if( specialBiomesActive() != wasActive ) {
        // change in active status
        return true;
        }
    return false;
    }



static SpecialBiome defaultRecord = { -1, -1 };


static SpecialBiome getBiomeRecord( int inX, int inY ) {
    int b = getMapBiome( inX, inY );

    if( b < 0 || b >= MAX_BIOME_NUMBER ) {
        return defaultRecord;
        }
    
    return specialBiomes[ b ];
    }



char specialBiomesActive() {
    if( curNumPlayers < minNumPlayers ) {
        return false;
        }
    return true;
    }



char isBiomeAllowed( int inDisplayID, int inX, int inY, char inIgnoreFloor ) {
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
    if( ! inIgnoreFloor && getMapFloor( inX, inY ) != 0 ) {
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


int getBiomeReliefEmot( int inSicknessObjectID ) {
    for( int i=0; i < MAX_BIOME_NUMBER; i++ ) {
        if( specialBiomes[i].sicknessObjectID == inSicknessObjectID ) {
            return specialBiomes[i].reliefEmotIndex;
            }
        }
    return -1;
    }



char isPolylingual( int inDisplayID ) {
    if( curNumPlayers < minNumPlayers &&
        curNumPlayers < minNumPlayersForLanguages ) {
        // if languages are still active, even if special biomes are not active
        // let polylingual races remain polylingual
        return false;
        }
    
    ObjectRecord *o = getObject( inDisplayID );
    
    if( o->race == 0 ) {
        return false;
        }
    
    if( polylingualRaces.getElementIndex( o->race ) != -1 ) {
        return true;
        }
    
    return false;
    }


int getPolylingualRace( char inIgnorePopulationSize ) {
    if( ! inIgnorePopulationSize && 
        curNumPlayers < minNumPlayers ) {
        return -1;
        }
    
    
    if( polylingualRaces.size() > 0 ) {
        return polylingualRaces.getElementDirect( 0 );
        }
    
    return -1;
    }



char *getBadBiomeMessage( int inDisplayID ) {
    if( curNumPlayers < minNumPlayers ) {
        return stringDuplicate( "BB\n#" );
        }
    
    ObjectRecord *o = getObject( inDisplayID );
    
    if( o->race == 0 ) {
        return stringDuplicate( "BB\n#" );
        }

    SimpleVector<char> messageWorking;

    messageWorking.appendElementString( "BB\n" );

    for( int i=0; i<MAX_BIOME_NUMBER; i++ ) {
        int r = specialBiomes[ i ].specialistRace;
        
        if( r != -1 && r != o->race ) {
            char *line = 
                autoSprintf( "%d %s\n", i, specialBiomes[i].badBiomeName );
            messageWorking.appendElementString( line );
            delete [] line;
            }
        }
    messageWorking.push_back( '#' );
    
    return messageWorking.getElementString();
    }



static SpecialBiome *getSpecialBiome( ObjectRecord *inObject ) {
    if( inObject->numBiomes == 0 ) {
        return NULL;
        }
    
    int b = inObject->biomes[ 0 ];
    
    if( b < MAX_BIOME_NUMBER ) {
        if( specialBiomes[b].badBiomeName == NULL ) {
            return NULL;
            }
        
        return &( specialBiomes[b] );
        }
    else {
        return NULL;
        }
    }
    


int getSpecialistRace( ObjectRecord *inObject ) {
    
    SpecialBiome *b = getSpecialBiome( inObject );
    
    if( b == NULL ) {
        return -1;
        }
    
    return b->specialistRace;
    }



const char *getBadBiomeName( ObjectRecord *inObject ) {
    SpecialBiome *b = getSpecialBiome( inObject );
    
    if( b == NULL ) {
        return NULL;
        }
    
    return b->badBiomeName;
    }



int getSpecialistRace( int inBiomeNumber ) {
    if( inBiomeNumber >= MAX_BIOME_NUMBER || inBiomeNumber < 0 ) {
        return -1;
        }
    
    return specialBiomes[inBiomeNumber].specialistRace;
    }


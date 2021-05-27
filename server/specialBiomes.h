

void initSpecialBiomes();


void freeSpecialBiomes();


// reload settings
void updateSpecialBiomes( int inNumPlayers );


// returns true if special biomes are currently active on server
char specialBiomesActive();



char isBiomeAllowed( int inDisplayID, int inX, int inY, 
                     char inIgnoreFloor = false );


// get sickness they should hold when stepping into biome
// -1 if no sickness or biome allowed
int getBiomeSickness( int inDisplayID, int inX, int inY );


// get emotion index of relief from biome sickness
// returns -1 if not set
int getBiomeReliefEmot( int inSicknessObjectID );


char isPolylingual( int inDisplayID );


// returns race number of first race that is polylingual
// returns -1 if there's no polylingual race
// Defaults to saying there's no polylingual race if population size is
// too small for specialty biomes.
int getPolylingualRace( char inIgnorePopulationSize = false );



// message to tell a player which biomes are bad for them
// and what their display names are
// destroyed by caller
char *getBadBiomeMessage( int inDisplayID );


// gets race number for specialist for inObject
// returns -1 if there's no specialist
int getSpecialistRace( ObjectRecord *inObject );

// returns -1 if there's no specialist
int getSpecialistRace( int inBiomeNumber );


const char *getBadBiomeName( ObjectRecord *inObject );

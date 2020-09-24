

void initSpecialBiomes();


void freeSpecialBiomes();


// reload settings
void updateSpecialBiomes( int inNumPlayers );



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
int getPolylingualRace();



// message to tell a player which biomes are bad for them
// and what their display names are
// destroyed by caller
char *getBadBiomeMessage( int inDisplayID );


// gets race number for specialist for inObject
// returns -1 if there's no specialist
int getSpecialistRace( ObjectRecord *inObject );


const char *getBadBiomeName( ObjectRecord *inObject );

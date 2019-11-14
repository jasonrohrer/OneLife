

void initSpecialBiomes();


void freeSpecialBiomes();


// reload settings
void updateSpecialBiomes( int inNumPlayers );



char isBiomeAllowed( int inDisplayID, int inX, int inY );


// get sickness they should hold when stepping into biome
// -1 if no sickness or biome allowed
int getBiomeSickness( int inDisplayID, int inX, int inY );

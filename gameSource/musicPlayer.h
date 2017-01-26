


void initMusicPlayer();

void freeMusicPlayer();


// starts music over again for player who is already inAge
// age rate is in years per second
// inForceNow true to start it now instead of waiting for coming of age
void restartMusic( double inAge, double inAgeRate, char inForceNow=false );

void instantStopMusic();


void stepMusicPlayer();


void setMusicLoudness( double inLoudness, char inForce=false );


double getMusicLoudness();




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



// adds to the list of actions currently suppressing music
// Upon first suppression, music fades from it's current loudness down to 0
// Each additional suppression simply keeps it there.
// Volume only fades back to the original level after all suppressions
// have been removed
//
// This fixes the problem of different areas of the code that were
// fading music out and then bringing it back later.
// If their requests cross, then one might fade the music back in with
// the other still needs it to be silent.
//
// Each bit of code should identify itself with a unique action name.
void addMusicSuppression( const char *inActionName );



// removes one from the list of actions currently suppressing music.
// If no suppression actions remain, music fades back up to its original volume
// where it was before the first call to addMusicSuppression
void removeMusicSuppression( const char *inActionName );
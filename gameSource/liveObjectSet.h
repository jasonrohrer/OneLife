

// must be called after initing object bank
void initLiveObjectSet();

void freeLiveObjectSet();




// call before filling next set of live objects
void clearLiveObjectSet();



// adds an object to the base set of live objects
// objects one transition step away will be auto-added as well  
void addBaseObjectToLiveObjectSet( int inID );


// call this after all base objects have been added
// this sends list of needed sprites to sprite bank for loading
void finalizeLiveObjectSet();


// checks if the set of needed sprites for the live object set
// has been loaded yet.
char isLiveObjectSetFullyLoaded( float *outProgress );


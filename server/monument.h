


// action is 1, 2, or 3
// for step, done, or call
void monumentAction( int inX, int inY, int inObjectID, int inPlayerID,
                     int inAction );


// call when map is wiped to put a file in place that will indicate
// that monument logs are stale
void monumentMapWipe();


// true if monuments have been wiped recently, and no new monument
// construction has started since then
char haveMonumentsBeenWiped();




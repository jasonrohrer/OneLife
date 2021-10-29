#include "../gameSource/GridPos.h"
#include "minorGems/network/web/WebRequest.h"


GridPos killPlayer( const char *inEmail );


void forcePlayerAge( const char *inEmail, double inAge );


WebRequest *defaultTimeoutWebRequest( const char *inURL );

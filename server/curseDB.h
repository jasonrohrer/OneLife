#include "../gameSource/GridPos.h"


void initCurseDB();


void freeCurseDB();


void setDBCurse( const char *inSenderEmail, const char *inReceiverEmail );

void clearDBCurse( const char *inSenderEmail, const char *inReceiverEmail );


char isCursed( const char *inSenderEmail, const char *inReceiverEmail );


void initPersonalCurseTest( const char *inTargetEmail );

void addPersonToPersonalCurseTest( const char *inEmail,
                                   const char *inTargetEmail,
                                   GridPos inPos );

char isBirthLocationCurseBlocked( const char *inTargetEmail, GridPos inPos );


// checks personal curse blocking without relying on or storing
// results in cache
// This results in potentially hitting the curse database for all players in 
// range
// (the cached version, above, avoids this for subsequent calls) 
// However, in some cases, like when checking twins for curses, 
// this extra work cannot be avoided.
char isBirthLocationCurseBlockedNoCache( const char *inTargetEmail, 
                                         GridPos inPos );

#include "../gameSource/GridPos.h"


void initCurseDB();


void freeCurseDB();


void setDBCurse( int inSenderID, 
                 const char *inSenderEmail, const char *inReceiverEmail );

void clearDBCurse( int inSenderID,
                   const char *inSenderEmail, const char *inReceiverEmail );


// one player forgiving everyone in one move
// this doesn't happen instantly, but instead is spread over our
// incremental culling operation
void clearAllDBCurse( int inSenderID, const char *inSenderEmail );



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

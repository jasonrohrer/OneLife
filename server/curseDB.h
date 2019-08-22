#include "../gameSource/GridPos.h"


void initCurseDB();


void freeCurseDB();


void setDBCurse( const char *inSenderEmail, const char *inReceiverEmail );


char isCursed( const char *inSenderEmail, const char *inReceiverEmail );


void initPersonalCurseTest();

void addPersonToPersonalCurseTest( const char *inEmail,
                                   GridPos inPos );

char isBirthLocationCurseBlocked( const char *inTargetEmail, GridPos inPos );

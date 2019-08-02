

void initLifeTokens();


void freeLifeTokens();

void stepLifeTokens();


// return value:
// 0 still pending
// -1 DENIED
// 1 spent  (or not using life token server)
int spendLifeToken( char *inEmail );

void refundLifeToken( char *inEmail );

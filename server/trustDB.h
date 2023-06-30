void initTrustDB();


void freeTrustDB();


void setDBTrust( int inSenderID, 
                 const char *inSenderEmail, const char *inReceiverEmail );

void clearDBTrust( const char *inSenderEmail, const char *inReceiverEmail );


char isTrusted( const char *inSenderEmail, const char *inReceiverEmail );



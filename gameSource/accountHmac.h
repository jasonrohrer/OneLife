


// gets URL parameters for correct account hmac based on download code
// and current sequence number.
//
// Example result:  
// sequence_number=36&account_hmac=8C87B150ACFBFE18B9790285986F169F67D7E82D
char *getAccountHmac();


// replaces an account hmac in inString with a fresh one
// deletes inString, if not NULL, and returns newly allocated string
// returns NULL if inString NULL
char *replaceAccountHmac( char *inString );


// get account key without hyphens
// destroyed by caller
char *getPureAccountKey();

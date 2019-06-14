

void initNames();


void freeNames();


// results destroyed internally when freeNames called
const char *findCloseFirstName( char *inString );

const char *findCloseLastName( char *inString );


// for names returned by findClose calls
int getFirstNameIndex( char *inFirstName );

int getLastNameIndex( char *inLastName );


const char *getFirstName( int inIndex, int *outNextIndex );

const char *getLastName( int inIndex, int *outNextIndex );

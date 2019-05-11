

void initLanguage();


void freeLanguage();


// each subsequent inPlayerID should be larger than any previous Eve added
void addEveLanguage( int inPlayerID );


void incrementLanguageCount( int inEveID );

void decrementLanguageCount( int inEveID );

void stepLanguage();


// handles punctuation, etc.
char *mapLanguagePhrase( char *inPhrase, int inEveIDA, int inEveIDB );

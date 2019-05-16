

void initLanguage();


void freeLanguage();


// each subsequent inPlayerID should be larger than any previous Eve added
void addEveLanguage( int inPlayerID );


void incrementLanguageCount( int inEveID );

void decrementLanguageCount( int inEveID );

void stepLanguage();


void removePlayerLanguageMaps( int inPlayerID );


// handles punctuation, etc.
char *mapLanguagePhrase( char *inPhrase, int inEveIDA, int inEveIDB,
                         int inPlayerIDA, int inPlayerIDB,
                         // ages in years
                         double inAgeA, double inAgeB,
                         int inParentIDA, int inParentIDB );



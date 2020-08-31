

void initLanguage();


void freeLanguage();


// each subsequent inPlayerID should be larger than any previous Eve added
void addEveLanguage( int inPlayerID );


void incrementLanguageCount( int inEveID );

// returns number of speakers of the language remaining
int decrementLanguageCount( int inEveID );

void stepLanguage();


void removePlayerLanguageMaps( int inPlayerID );


// handles punctuation, etc.
char *mapLanguagePhrase( char *inPhrase, int inEveIDA, int inEveIDB,
                         int inPlayerIDA, int inPlayerIDB,
                         // ages in years
                         double inAgeA, double inAgeB,
                         int inParentIDA, int inParentIDB,
                         // if > 0, some clusters that would be in other
                         // language come straight through
                         double inFractionToPassThrough );



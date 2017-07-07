#ifndef CATEGORY_BANK_INCLUDED
#define CATEGORY_BANK_INCLUDED

#include "minorGems/util/SimpleVector.h"


typedef struct CategoryRecord {
        int id;
        
        char *description;
        
        SimpleVector<int> objectIDSet;
                
    } CategoryRecord;



// backwards mapping
typedef struct ReverseCategoryRecord {
        int objectID;
        
        SimpleVector<int> categoryIDSet;
                
    } ReverseCategoryRecord;



// loads from categories folder
// returns number of categories that need to be loaded
int initCategoryBankStart( char *outRebuildingCache );

// returns progress... ready for Finish when progress == 1.0
float initCategoryBankStep();
void initCategoryBankFinish();


void freeCategoryBank();


CategoryRecord *getCategory( int inID );
ReverseCategoryRecord *getReverseCategory( int inObjecID );


// return array destroyed by caller, NULL if none found
CategoryRecord **searchCategories( const char *inSearch, 
                                   int inNumToSkip, 
                                   int inNumToGet, 
                                   int *outNumResults, int *outNumRemaining );


int addCategory( const char *inDescription );

void addCategoryToObject( int inObjectID, int inCategoryID );

void removeCategoryFromObject( int inObjectID, int inCategoryID );

void removeObjectFromAllCategories( int inObjectID );

void moveCategoryUp( int inObjectID, int inCategoryID );
void moveCategoryDown( int inObjectID, int inCategoryID );

int getNumCategoriesForObject( int inObjectID );

// return -1 if index does not exist
int getCategoryForObject( int inObjectID, int inCategoryIndex );



void deleteCategoryFromBank( int inID );



#endif

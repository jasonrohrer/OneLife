#ifndef CATEGORY_BANK_INCLUDED
#define CATEGORY_BANK_INCLUDED

#include "minorGems/util/SimpleVector.h"


typedef struct CategoryRecord {
        // object ID of parent object
        int parentID;
        
        // child objects that are in this category
        // none of these should themselves be parent objects
        SimpleVector<int> objectIDSet;
                
    } CategoryRecord;



// backwards mapping
typedef struct ReverseCategoryRecord {
        // child object ID
        int childID;
        
        // parent objects whose category this child object is in
        SimpleVector<int> categoryIDSet;
                
    } ReverseCategoryRecord;



// loads from categories folder
// returns number of categories that need to be loaded
int initCategoryBankStart( char *outRebuildingCache );

// returns progress... ready for Finish when progress == 1.0
float initCategoryBankStep();
void initCategoryBankFinish();


void freeCategoryBank();


CategoryRecord *getCategory( int inParentID );
ReverseCategoryRecord *getReverseCategory( int inChildID );


void addCategoryToObject( int inObjectID, int inParentID );

void removeCategoryFromObject( int inObjectID, int inParentID );

void removeObjectFromAllCategories( int inObjectID );

void moveCategoryUp( int inObjectID, int inParentID );
void moveCategoryDown( int inObjectID, int inParentID );

int getNumCategoriesForObject( int inObjectID );

// return -1 if index does not exist
int getCategoryForObject( int inObjectID, int inCategoryIndex );



void deleteCategoryFromBank( int inID );


// used as either parent or child
char isObjectUsedInCategories( int inObjectID );


#endif

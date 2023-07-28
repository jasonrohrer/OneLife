#ifndef CATEGORY_BANK_INCLUDED
#define CATEGORY_BANK_INCLUDED

#include "minorGems/util/SimpleVector.h"


typedef struct CategoryRecord {
        // object ID of parent object
        int parentID;
        
        // true if this category is a pattern
        // means parentID can be a real gameplay object, and that we add
        // extra transitions only if another pattern with the same number
        // of members  matches for other parts of a transition
        // For example, if actor and target are both patterns with the same
        // number of elements, extra transitions will be generated
        // for all the element pairs in the two patterns.
        //
        // Not that parentID can also be an abstract object that isn't
        // a real gameplay object.  The parent object should have a
        // description that starts with @ in that case.
        char isPattern;
        
        // true if this category is a set of probability-weighted objects
        char isProbabilitySet;
        
        // child objects that are in this category
        // none of these should themselves be parent objects
        SimpleVector<int> objectIDSet;
        
        // for probability sets
        SimpleVector<float> objectWeights;

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


void removeObjectFromCategory( int inParentID, int inObjectID, 
                               int inListIndex );



void setCategoryIsPattern( int inParentID, char inIsPattern );
void setCategoryIsProbabilitySet( int inParentID, char inIsProbabilitySet );



void removeObjectFromAllCategories( int inObjectID );

/**
NOTE:
These functions are currently not implemented in a useful way.
They result in a change to RAM records only, and the result is not
preserved on disk in the category folder.

// move category up/down on object's category list (which categories object
// is part of, and which take precedence)
void moveCategoryUp( int inObjectID, int inParentID );
void moveCategoryDown( int inObjectID, int inParentID );
*/

// move member object up/down in category's member list
void moveCategoryMemberUp( int inParentID, int inObjectID, int inListIndex );
void moveCategoryMemberDown( int inParentID, int inObjectID, int inListIndex );

void setMemberWeight( int inParentID, int inObjectID, float inWeight );

// only works on prob sets
void makeWeightUniform( int inParentID );



int getNumCategoriesForObject( int inObjectID );

// return -1 if index does not exist
int getCategoryForObject( int inObjectID, int inCategoryIndex );



void deleteCategoryFromBank( int inID );


// used as either parent or child
char isObjectUsedInCategories( int inObjectID );



int pickFromProbSet( int inParentID );


char isProbabilitySet( int inParentID );


#endif

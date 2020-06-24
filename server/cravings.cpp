
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"



typedef struct CravingList {
        int lineageEveID;
        SimpleVector<int> cravedFoodIDs;
    } CravingList;
    

static SimpleVector<CravingList> list;



// returns NULL if not found
static CravingList *getListForLineage( int inLineageEveID ) {
    for( int i=0; i<list.size(); i++ ) {
        CravingList *l = list.getElement( i );
        if( l->lineageEveID == inLineageEveID ) {
            return l;
            }
        }
    return NULL;
    }



static JenkinsRandomSource randSource;


static int getRandomFood( int inPlayerGenerationNumber, 
                          int inFoodToAvoid = -1 ) {
    
    double e = SettingsManager::getDoubleSetting( "cravingPoolExponent", 1.4 );

    int maxDepth = lrint( pow( inPlayerGenerationNumber, e ) );

    if( maxDepth < 1 ) {
        maxDepth = 1;
        }
    

    SimpleVector<int> *allFoods = getAllPossibleFoodIDs();
    
    SimpleVector<int> possibleFoods;
    
    for( int i=0; i<allFoods->size(); i++ ) {
        int id = allFoods->getElementDirect( i );
        
        if( id != inFoodToAvoid ) {
            
            int d = getObjectDepth( id );

            if( d <= maxDepth ) {
                possibleFoods.push_back( id );
                }
            }
        }
    
    if( possibleFoods.size() > 0 ) {
        int pick = 
            randSource.getRandomBoundedInt( 0, possibleFoods.size() - 1 );
    
        return possibleFoods.getElementDirect( pick );
        }
    else {
        // no possible foods at or below d depth

        // return first food in main list
        if( allFoods->size() > 0 ) {
            return allFoods->getElementDirect( 0 );
            }
        else {
            return -1;
            }
        }
    }




int getCravedFood( int inLineageEveID, int inPlayerGenerationNumber,
                   int inLastCravedID ) {

    CravingList *l = getListForLineage( inLineageEveID );
   
    if( l == NULL ) {
        // push a new empty list
        CravingList newL;
        newL.lineageEveID = inLineageEveID;
        list.push_back( newL );
        
        l = getListForLineage( inLineageEveID );
        }
    
    int listSize = l->cravedFoodIDs.size();
    for( int i=0; i < listSize; i++ ) {
        int foodID = l->cravedFoodIDs.getElementDirect( i );
        
        if( foodID == inLastCravedID && i < listSize - 1 ) {
            return l->cravedFoodIDs.getElementDirect( i + 1 );
            }
        }
    
    // got here, we went off end of list without finding our last food
    // add a new one and return that.

    // avoid repeating last food we craved
    int newFoodID = getRandomFood( inPlayerGenerationNumber, inLastCravedID );
    
    l->cravedFoodIDs.push_back( newFoodID );
    
    return newFoodID;
    }



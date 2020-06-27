#include "cravings.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"


Craving noCraving = { -1, -1 };




typedef struct CravingList {
        int lineageEveID;
        SimpleVector<Craving> cravedFoods;
    } CravingList;
    

static SimpleVector<CravingList> list;

static int nextUniqueID = 1;



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


static Craving getRandomFood( int inPlayerGenerationNumber, 
                              Craving inFoodToAvoid = noCraving ) {
    
    double e = SettingsManager::getDoubleSetting( "cravingPoolExponent", 1.4 );

    double offset = SettingsManager::getDoubleSetting( "cravingPoolOffset", 5 );

    int maxDepth = lrint( pow( inPlayerGenerationNumber + offset, e ) );

    if( maxDepth < 1 ) {
        maxDepth = 1;
        }
    

    SimpleVector<int> *allFoods = getAllPossibleFoodIDs();
    
    SimpleVector<int> possibleFoods;
    
    for( int i=0; i<allFoods->size(); i++ ) {
        int id = allFoods->getElementDirect( i );
        
        if( id != inFoodToAvoid.foodID ) {
            
            int d = getObjectDepth( id );

            if( d <= maxDepth ) {
                possibleFoods.push_back( id );
                }
            }
        }
    
    if( possibleFoods.size() > 0 ) {
        int pick = 
            randSource.getRandomBoundedInt( 0, possibleFoods.size() - 1 );
    
        printf( "%d possible foods, picking #%d\n", possibleFoods.size(),
                pick );
        
        Craving c = { possibleFoods.getElementDirect( pick ),
                      nextUniqueID };
        nextUniqueID ++;
        
        return c;
        }
    else {
        // no possible foods at or below d depth

        // return first food in main list
        if( allFoods->size() > 0 ) {
            Craving c = { allFoods->getElementDirect( 0 ),
                          nextUniqueID };
            nextUniqueID ++;
        
            return c;
            }
        else {
            return noCraving;
            }
        }
    }




Craving getCravedFood( int inLineageEveID, int inPlayerGenerationNumber,
                       Craving inLastCraved ) {

    CravingList *l = getListForLineage( inLineageEveID );
   
    if( l == NULL ) {
        // push a new empty list
        CravingList newL;
        newL.lineageEveID = inLineageEveID;
        list.push_back( newL );
        
        l = getListForLineage( inLineageEveID );
        }
    
    int listSize = l->cravedFoods.size();
    for( int i=0; i < listSize; i++ ) {
        Craving food = l->cravedFoods.getElementDirect( i );
        
        if( food.foodID == inLastCraved.foodID && 
            food.uniqueID == inLastCraved.uniqueID &&
            i < listSize - 1 ) {
            return l->cravedFoods.getElementDirect( i + 1 );
            }
        }
    
    // got here, we went off end of list without finding our last food
    // add a new one and return that.

    // avoid repeating last food we craved
    Craving newFood = getRandomFood( inPlayerGenerationNumber, inLastCraved );
    
    l->cravedFoods.push_back( newFood );
    
    return newFood;
    }




void purgeStaleCravings( int inLowestUniqueID ) {
    
    int numPurged = 0;
    
    for( int i=0; i<list.size(); i++ ) {
        CravingList *l = list.getElement( i );        

        // walk backwards and find first id that is below inLowestUniqueID
        // then trim all records that come before that
        int start = l->cravedFoods.size() - 1;
        
        for( int f=start; f >= 0; f-- ) {
            
            if( l->cravedFoods.getElementDirect( f ).uniqueID < 
                inLowestUniqueID ) {
                
                l->cravedFoods.deleteStartElements( f + 1 );
                
                numPurged += f + 1;
                break;
                }
            }
        }
    }




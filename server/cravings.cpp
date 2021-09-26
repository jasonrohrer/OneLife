#include "cravings.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"


Craving noCraving = { -1, -1, 0 };




typedef struct CravingList {
        int lineageEveID;
		int lineageMaxFoodDepth;
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


static Craving getRandomFood( int inLineageMaxFoodDepth, 
                              Craving inFoodToAvoid = noCraving ) {

	SimpleVector<int> *allFoods = getAllPossibleFoodIDs();
    
    SimpleVector<int> possibleFoods;
    
    if( inLineageMaxFoodDepth > 0 ) {
        
        // the next craving is drawn with certain probability to "advance the tech"
        // which controls how many difficult food is in the craving pool vs the easy food
        
        double harderCravingProb = SettingsManager::getDoubleSetting( "harderCravingProb", 0.3 );
        
        for( int i=0; i<allFoods->size(); i++ ) {
            int id = allFoods->getElementDirect( i );
            
            if( id != inFoodToAvoid.foodID ) {
                
                int d = getObjectDepth( id );

                if( d <= inLineageMaxFoodDepth ) {
                    possibleFoods.push_back( id );
                    }
                }
            }
            
        int EasyFoodListLength = possibleFoods.size();
        int TotalFoodListLength = ceil( EasyFoodListLength / (1 - harderCravingProb) );

        int nextDepth = inLineageMaxFoodDepth;
        
        while( possibleFoods.size() < TotalFoodListLength && nextDepth < getMaxDepth() ) {
            nextDepth = nextDepth + 1;
            for( int i=0; i<allFoods->size(); i++ ) {
                int id = allFoods->getElementDirect( i );
                
                if( id != inFoodToAvoid.foodID ) {
                    
                    int d = getObjectDepth( id );

                    if( d == nextDepth ) {
                        possibleFoods.push_back( id );
                        if( possibleFoods.size() == TotalFoodListLength ) break;
                        }
                    }
                }
            }
        }
    else {
        // new lineage, crave some wild food
        for( int i=0; i<allFoods->size(); i++ ) {
            int id = allFoods->getElementDirect( i );
            
            if( id != inFoodToAvoid.foodID ) {
                
                int d = getObjectDepth( id );

                if( d <= 1 ) {
                    possibleFoods.push_back( id );
                    }
                }
            }
        }
    
    if( possibleFoods.size() > 0 ) {
        int pick = 
            randSource.getRandomBoundedInt( 0, possibleFoods.size() - 1 );
            
        int pickedFood = possibleFoods.getElementDirect( pick );
        
        int bonus = getObjectDepth( pickedFood ) - inLineageMaxFoodDepth;
        
        if( bonus < 1 ) bonus = 1;
    
        printf( "%d possible foods, picking #%d\n", possibleFoods.size(),
                pick );
        
        Craving c = { pickedFood,
                      nextUniqueID,
                      bonus
                      };
        nextUniqueID ++;
        
        return c;
        }
    else {
        // no possible foods at or below d depth

        // return first food in main list
        if( allFoods->size() > 0 ) {
            int pickedFood = allFoods->getElementDirect( 0 );
            Craving c = { pickedFood,
                          nextUniqueID,
                          getObjectDepth( pickedFood )
                          };
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
		newL.lineageMaxFoodDepth = 0;
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
    Craving newFood = getRandomFood( l->lineageMaxFoodDepth, inLastCraved );
    
    l->cravedFoods.push_back( newFood );
    
    return newFood;
    }


void logFoodDepth( int inLineageEveID, int inEatenID ) {
    
    // the food with max depth ever eaten is
    // the way we gauge how advanced the tech is for a lineage
    
    int d = getObjectDepth( inEatenID );
    
    // do not include uncraftable food in determining cravings
    if( d == UNREACHABLE ) return;

    CravingList *l = getListForLineage( inLineageEveID );
   
    if( l == NULL ) {
        // push a new empty list
        CravingList newL;
        newL.lineageEveID = inLineageEveID;
		newL.lineageMaxFoodDepth = 0;
        list.push_back( newL );
        
        l = getListForLineage( inLineageEveID );
        }

	if( d > l->lineageMaxFoodDepth ) {
		l->lineageMaxFoodDepth = d;
		}

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




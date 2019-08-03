#include "../gameSource/objectBank.h"

#include "minorGems/util/SimpleVector.h"


void testChildGenderPlacement() {
    
    // test child gender placement
    int numObj;
    ObjectRecord **allObj = getAllObjects( &numObj );

    for( int i=0; i<numObj; i++ ) {
        ObjectRecord *o = allObj[ i ];
        
        if( o->person &&
            ! o->male ) {

            int maleCount = 0;
            int femaleCount = 0;
            
            SimpleVector<int> babyIDs;
            SimpleVector<int> counts;
            

            for( int b=0; b<100000; b++ ) {
                
                int babyID = getRandomFamilyMember( o->race, o->id, 2 );
                
                int existingIndex = babyIDs.getElementIndex( babyID );
                
                if( existingIndex != -1 ) {
                    ( *( counts.getElement( existingIndex ) ) ) ++;
                    }
                else {
                    babyIDs.push_back( babyID );
                    counts.push_back( 1 );
                    }

                ObjectRecord *babyO = getObject( babyID );
                
                if( babyO->male ) {
                    maleCount++;
                    }
                else {
                    femaleCount++;
                    }
                }
            printf( "Mother id %d (race=%d) had %f%% male babies\n",
                    o->id,
                    o->race,
                    maleCount / (double)(maleCount + femaleCount ) );

            for( int b=0; b<babyIDs.size(); b++ ) {
                int id = babyIDs.getElementDirect( b );
                printf( "   babyID_%d born %d times (male=%d)\n", id,
                        counts.getElementDirect( b ),
                        getObject( id )->male );
                }
            }    
        }
    delete [] allObj;
    }


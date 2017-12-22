

#include "../serverCalls.h"
#include "../map.h"



static void customInit() {
    ClothingSet clothing = getEmptyClothingSet();
    
    clothing.hat = getObject( 199 );
    clothing.tunic = getObject( 201 );
    clothing.bottom = getObject( 200 );
    clothing.frontShoe = getObject( 203 );
    clothing.backShoe = getObject( 203 );
    
    PlayerMapping narrator = {
        "test@test.com",
        19,
        20,
        { -30, -10 },
        0, clothing };
    
    playerMap.push_back( narrator );
    }




static LiveDummySocket wolfDummy;
static LiveDummySocket firstBaby;





static void customTrigger( int inTriggerNumber ) {
    
    //GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    
    
    if( inTriggerNumber == t++ ) {
        
        GridPos startPos = { -16, -8 };
        
        
        
        wolfDummy = newDummyPlayer( "dummy3@test.com", 418, 0,
                                    startPos,
                                    0,
                                    clothing );
        }
    else if( inTriggerNumber == t++ ) {
        // wolf runs past cammera
        addToMove( 1, -1 );
        addToMove( 2, -2 );
        addToMove( 3, -3 );
        addToMove( 4, -4 );
        addToMove( 5, -5 );
        addToMove( 6, -6 );
        addToMove( 7, -7 );
        addToMove( 8, -8 );
        
        sendDummyMove( &wolfDummy, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {

        GridPos startPos = { -10, -10 };
        
        
        
        firstBaby = newDummyPlayer( "dummy2@test.com", 19, 0,
                                    startPos,
                                    0,
                                    clothing );

        // stick a loincloth to right so we can put it on baby
        setMapObject( -1, -10, 200 );

        // set up phonograph
        setMapObject( 3, 0, 488 );
        setMapObject( 4, 0, 491 );
        }
    else if( inTriggerNumber == t++ ) {
        sendDummySay( &firstBaby, "O" );
        }
    else if( inTriggerNumber == t++ ) {
        sendDummySay( &firstBaby, "K" );
        }
    else if( inTriggerNumber == t++ ) {

        GridPos deathPos = killPlayer( "test@test.com" );
        firstBaby.pos = deathPos;
        }
    else if( inTriggerNumber == t++ ) {        
        // baby walks around at end
        addToMove( 1, 0 );
        addToMove( 2, 0 );
        addToMove( 3, 0 );
        addToMove( 4, 0 );
        
        sendDummyMove( &firstBaby, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {        
        // baby walks around at end
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        
        sendDummyMove( &firstBaby, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {        
        // kill baby
        
        dummySockets.deleteElementEqualTo( firstBaby.sock );

        delete firstBaby.sock;
        }
    
    }


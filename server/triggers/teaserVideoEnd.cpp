

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
        { 280, -0 },
        0, clothing };
    
    playerMap.push_back( narrator );
    }




static LiveDummySocket firstBaby;
static LiveDummySocket oldMan;





static void customTrigger( int inTriggerNumber ) {
    
    GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    
    
    if( inTriggerNumber == t++ ) {
        
        GridPos startPos = { 280, 0 };
        
        clothing.bottom = getObject( 200 );
        
        firstBaby = newDummyPlayer( "dummy2@test.com", 19, 0,
                                    startPos,
                                    0,
                                    clothing );
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
    else if( inTriggerNumber == t++ ) {        
        // old man starts off screen
        GridPos startPos = { 286, 0 };
        
        clothing.bottom = getObject( 200 );
        clothing.tunic = getObject( 201 );

        oldMan = newDummyPlayer( "dummy3@test.com", 355, 55,
                                 startPos,
                                 0,
                                 clothing );
        // walks up to grave
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        addToMove( -4, 0 );
        
        sendDummyMove( &oldMan, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        offset.x = -1;
        
        // grab hat
        sendDummyAction( &oldMan, "REMV", offset, true, 4 );
        }
    else if( inTriggerNumber == t++ ) {
        // put on hat
        sendDummyAction( &oldMan, "SELF", offset, true, 0 );
        }
    else if( inTriggerNumber == t++ ) {        

        // walks up to bush
        addToMove( -1, 1 );
        sendDummyMove( &oldMan, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        offset.x = -1;
        // grab berry
        sendDummyAction( &oldMan, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // eat berry
        sendDummyAction( &oldMan, "SELF", offset, true, 0 );
        }
    else if( inTriggerNumber == t++ ) {        
        // walks off screen
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        addToMove( -4, 0 );
        addToMove( -5, 0 );
        addToMove( -6, 0 );
        addToMove( -7, 0 );
        addToMove( -8, 0 );
        
        sendDummyMove( &oldMan, finishMove() );
        }
    }


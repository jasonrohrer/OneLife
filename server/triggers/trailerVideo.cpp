

#include "../serverCalls.h"
#include "../map.h"



static void customInit() {
    ClothingSet clothing = getEmptyClothingSet();
    
    //clothing.hat = getObject( 199 );
    //clothing.tunic = getObject( 201 );
    //clothing.bottom = getObject( 200 );
    //clothing.frontShoe = getObject( 203 );
    //clothing.backShoe = getObject( 203 );
    
    PlayerMapping narrator = {
        "test@test.com",
        433,
        39,
        { -2, 0 },
        0, clothing };
    
    playerMap.push_back( narrator );
    }




static LiveDummySocket wolfDummy;


static LiveDummySocket eve;
static LiveDummySocket firstBaby;


static LiveDummySocket carDriver;
static LiveDummySocket transportDriver;





static void customTrigger( int inTriggerNumber ) {
    
    //GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    

    if( inTriggerNumber == t++ ) {
        forcePlayerAge( "test@test.com", 40 );
        
        GridPos startPos = { 22, -3 };
        
        clothing.tunic = getObject( 201 );
        clothing.bottom = getObject( 200 );
        
        eve = newDummyPlayer( "dummy4@test.com", 19, 20,
                              startPos,
                              0,
                              clothing );
        }
    else if( inTriggerNumber == t++ ) {
        // eve walks into clearing
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        addToMove( -4, 0 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // eve has baby
        GridPos startPos = { 18, -3 };
        
        firstBaby = newDummyPlayer( "dummy5@test.com", 350, 0,
                                    startPos,
                                    0,
                                    clothing );
        }
    else if( inTriggerNumber == t++ ) {
        // grab baby
        GridPos offset = { 0, 0 };
        sendDummyAction( &eve, "BABY", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // eve walks south with baby
        addToMove( 0, -1 );
        addToMove( 0, -2 );
        addToMove( 0, -3 );
        addToMove( 0, -4 );
        addToMove( 0, -5 );
        addToMove( 0, -6 );
        addToMove( 0, -7 );
        addToMove( 0, -8 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {

        GridPos startPos = { 17, -2 };

        clothing.tunic = getObject( 585 );
        carDriver = newDummyPlayer( "dummy2@test.com", 19, 20,
                                    startPos,
                                    630,
                                    clothing );


        clothing = getEmptyClothingSet();
        
        clothing.hat = getObject( 199 );
        clothing.tunic = getObject( 201 );
        clothing.bottom = getObject( 200 );
        clothing.frontShoe = getObject( 203 );
        clothing.backShoe = getObject( 203 );

        startPos.x = 15;
        startPos.y = -8;
        transportDriver = newDummyPlayer( "dummy3@test.com", 19, 20,
                                          startPos,
                                          //0,
                                          632,
                                          clothing );
        }
    else if( inTriggerNumber == t++ ) {
        // car drives past
        
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        addToMove( -4, 0 );
        addToMove( -5, 0 );
        addToMove( -6, 0 );
        addToMove( -7, 0 );
        addToMove( -8, 0 );
        addToMove( -9, 0 );
        addToMove( -10, 0 );
        addToMove( -11, 0 );
        addToMove( -12, 0 );
        addToMove( -13, 0 );
        addToMove( -14, 0 );
        addToMove( -15, 0 );
        addToMove( -16, 0 );
        addToMove( -17, 0 );
        
        sendDummyMove( &carDriver, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // horn honks
        
        // plays on decay of this place-holder object
        setMapObject( 8, -3, 654 );
        }
    else if( inTriggerNumber == t++ ) {
        // transport goes overhead
        
        addToMove( 0, 1 );
        addToMove( 0, 2 );
        addToMove( 0, 3 );
        addToMove( 0, 4 );
        addToMove( 0, 5 );
        addToMove( 0, 6 );
        addToMove( 0, 7 );
        addToMove( 0, 8 );
        addToMove( 0, 9 );
        addToMove( 0, 10 );
        addToMove( 0, 11 );
        addToMove( 0, 12 );
        addToMove( 0, 13 );
        addToMove( 0, 14 );
        addToMove( 0, 15 );
        addToMove( 0, 16 );
        addToMove( 0, 17 );
        addToMove( 0, 18 );
        addToMove( 0, 19 );
        
        sendDummyMove( &transportDriver, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // robot switch to firing state
        setMapObject( 14, 3, 648 );
        }
    else if( inTriggerNumber == t++ ) {
        GridPos deathPos = killPlayer( "test@test.com" );
        }
    else if( inTriggerNumber == t++ ) {
        // robot switch to greeting state
        setMapObject( 14, 3, 649 );
        }
    else if( inTriggerNumber == t++ ) {
        // robot switch to leaving state
        setMapObject( 14, 3, 650 );
        }
    else if( inTriggerNumber == t++ ) {
        
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


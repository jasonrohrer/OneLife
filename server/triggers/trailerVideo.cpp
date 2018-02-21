

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

static LiveDummySocket lostBaby;


static LiveDummySocket momA;
static LiveDummySocket dadA;
static LiveDummySocket grandpaA;
static LiveDummySocket babyA;
static LiveDummySocket kidA1;
static LiveDummySocket kidA2;



static LiveDummySocket manB1;
static LiveDummySocket manB2;
static LiveDummySocket womanB1;
static LiveDummySocket womanB2;
static LiveDummySocket kidB1;




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
        

        // use this opportunity to spawn all NPCs for video

        GridPos startPos = { 25, -3 };
        
        clothing.tunic = getObject( 201 );
        clothing.bottom = getObject( 200 );
        
        eve = newDummyPlayer( "dummy4@test.com", 19, 20,
                              startPos,
                              0,
                              clothing );

        startPos.x = 21;
        startPos.y = 2;
        clothing = getEmptyClothingSet();
        lostBaby = newDummyPlayer( "lostBaby@test.com", 347, 1,
                              startPos,
                              0,
                              clothing );
        
        
        clothing.tunic = getObject( 201 );
        clothing.bottom = getObject( 200 );
        
        startPos.x = 24;
        startPos.y = 4;
        momA = newDummyPlayer( "momA@test.com", 350, 27,
                               startPos,
                               0,
                               clothing );
        startPos.x = 25;
        clothing = getEmptyClothingSet();
        babyA = newDummyPlayer( "babyA@test.com", 351, 1,
                               startPos,
                               0,
                               clothing );


        clothing.bottom = getObject( 200 );
        
        startPos.x = 26;
        startPos.y = 5;
        dadA = newDummyPlayer( "dadA@test.com", 355, 27,
                               startPos,
                               0,
                               clothing );

        startPos.x = 27;
        startPos.y = 5;
        kidA1 = newDummyPlayer( "kidA1@test.com", 353, 3,
                                startPos,
                                0,
                                clothing );
        startPos.x = 30;
        startPos.y = 9;
        kidA2 = newDummyPlayer( "kidA2@test.com", 350, 5,
                                startPos,
                                40,
                                clothing );
        
        clothing.hat = getObject( 199 );
        clothing.frontShoe = getObject( 203 );
        clothing.backShoe = getObject( 203 );

        startPos.x = 27;
        startPos.y = 6;
        grandpaA = newDummyPlayer( "grandpaA@test.com", 347, 55,
                               startPos,
                               0,
                               clothing );


        clothing = getEmptyClothingSet();
        clothing.bottom = getObject( 200 );
        
        startPos.x = 40;
        startPos.y = 1;
        manB1 = newDummyPlayer( "manB1@test.com", 347, 28,
                                startPos,
                                292,
                                clothing );

        startPos.x = 47;
        startPos.y = 3;
        manB2 = newDummyPlayer( "manB2@test.com", 354, 57,
                                startPos,
                                0,
                                clothing );
        
        startPos.x = 42;
        startPos.y = 2;
        kidB1 = newDummyPlayer( "kidB1@test.com", 352, 6,
                                startPos,
                                135,
                                clothing );



        clothing.tunic = getObject( 201 );
        
        startPos.x = 43;
        startPos.y = -1;
        womanB1 = newDummyPlayer( "womanB1@test.com", 353, 29,
                                startPos,
                                139,
                                clothing );

        startPos.x = 45;
        startPos.y = 2;
        womanB2 = newDummyPlayer( "womanB2@test.com", 351, 49,
                                startPos,
                                71,
                                clothing );

        }
    else if( inTriggerNumber == t++ ) {
        // eve walks into clearing
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        addToMove( -3, 0 );
        addToMove( -4, 0 );
        addToMove( -5, 0 );
        addToMove( -6, 0 );
        addToMove( -7, 0 );
        
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
        // lost baby runs past
        forcePlayerAge( "lostBaby@test.com", 0 );
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
        
        sendDummyMove( &lostBaby, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // grab hat
        GridPos offset = { 0, 1 };
        sendDummyAction( &momA, "USE", offset );        
        }
    else if( inTriggerNumber == t++ ) {
         // put on baby
        GridPos offset = { 1, 0 };
        sendDummyAction( &momA, "UBABY", offset, true, -1 );
        
        // pick berry
        setNextActionDelay( 0.5 );
        offset.x = -1;
        sendDummyAction( &dadA, "USE", offset );

        // kid comes walking in
        addToMove( -1, -1 );
        addToMove( -1, -2 );
        addToMove( -1, -3 );
        addToMove( -1, -4 );
        addToMove( -2, -5 );
        addToMove( -3, -5 );
        addToMove( -4, -5 );
        
        sendDummyMove( &kidA2, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // feed berry
        GridPos offset = { 1, 0 };
        sendDummyAction( &dadA, "UBABY", offset, true, -1 );

        // pick up baby
        setNextActionDelay( 0.25 );
        sendDummyAction( &momA, "BABY", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // skin rabbit
        GridPos offset = { 1, 0 };
        sendDummyAction( &kidB1, "USE", offset );
        }    
    else if( inTriggerNumber == t++ ) {
        // chop wood
        GridPos offset = { -1, 0 };
        sendDummyAction( &womanB2, "USE", offset );

        // set down stone
        setNextActionDelay( 0.5 );
        offset.x = 0;
        sendDummyAction( &kidB1, "DROP", offset, true, -1 );

        // walk into scene
        addToMove( -1, -1 );
        addToMove( -2, -2 );
        
        sendDummyMove( &manB2, finishMove() );
        }    
    else if( inTriggerNumber == t++ ) {
        // pick up rabbit fur
        GridPos offset = { 1, 0 };
        sendDummyAction( &kidB1, "USE", offset );

        // drop hatchet
        setNextActionDelay( 0.25 );
        sendDummyAction( &womanB2, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // pick drop fur
        GridPos offset = { 0, 1 };
        sendDummyAction( &kidB1, "DROP", offset, true, -1 );

        // grab wood
        offset.x = -1;
        offset.y = 0;
        setNextActionDelay( 0.5 );
        sendDummyAction( &womanB2, "USE", offset );
        }    
    else if( inTriggerNumber == t++ ) {
        // pick up skewer
        GridPos offset = { -1, 0 };
        sendDummyAction( &kidB1, "USE", offset );
        
        // grab sharp stone
        setNextActionDelay( 0.25 );
        offset.x = 0;
        sendDummyAction( &manB2, "USE", offset );
        }
    else  if( inTriggerNumber == t++ ) {
        // walk to fire
        addToMove( -1, -1 );
        addToMove( -2, -1 );
        
        sendDummyMove( &womanB2, finishMove() );

        // skewer rabbit
        setNextActionDelay( 0.25 );
        GridPos offset = { 1, 0 };
        sendDummyAction( &kidB1, "USE", offset );
        

        // use stone on wood
        setNextActionDelay( 0.5 );
        offset.x = -1;
        sendDummyAction( &manB2, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // feed fire
        GridPos offset = { -1, 0 };
        sendDummyAction( &womanB2, "USE", offset );
        
        // use stone on wood
        setNextActionDelay( 0.5 );
        sendDummyAction( &manB2, "USE", offset );
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


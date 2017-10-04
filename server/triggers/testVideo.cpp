


static void customInit() {
    ClothingSet clothing = getEmptyClothingSet();
    
    clothing.bottom = getObject( 200 );
    
    PlayerMapping narrator = {
        "test@test.com",
        433,
        20,
        { 0, 0 },
        0, clothing };
    
    playerMap.push_back( narrator );
    }



// eve scene
static LiveDummySocket eve;
static LiveDummySocket firstBaby;

// hunter-gatherer camp
static LiveDummySocket bob;
static LiveDummySocket alice;
static LiveDummySocket bobby;




static void customTrigger( int inTriggerNumber ) {
    
    GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    
    
    if( inTriggerNumber == t++ ) {
        GridPos startPos = { 12, -11 };
        
        
        
        eve = newDummyPlayer( "dummy1@test.com", 19, 14,
                                            startPos,
                                            0,
                                            clothing );        
        }
    else if( inTriggerNumber == t++ ) {
        // grab stone
        offset.x = -1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( -1, -1 );
        addToMove( -1, -2 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // sharpen stone
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( 1, -1 );
        addToMove( 2, -2 );
        addToMove( 2, -3 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // cut reeds
        offset.x = -1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop stone
        offset.y = -1;
        sendDummyAction( &eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab reeds
        offset.x = -1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( &eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // walk to milkweed patch A
        addToMove( 0, 1 );
        addToMove( 0, 2 );
        addToMove( -1, 2 );
        addToMove( -2, 2 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // grab stalk
        offset.x = -1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( &eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other stalk
        offset.x = 1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make strand at feet
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // walk to milkweed patch B
        addToMove( 0, -1 );

        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // grab stalk
        offset.x = -1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( &eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other stalk
        offset.x = 1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make strand at feet
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other strand
        offset.y = 1;
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make rope at feet
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab rope
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // walk back to reeds
        addToMove( 1, 0 );
        addToMove( 2, -1 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // make skirt
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab skirt
        sendDummyAction( &eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // put on skirt
        sendDummyAction( &eve, "SELF", offset, true, 4 );
        }
    else if( inTriggerNumber == t++ ) {

        GridPos startPos = eve.pos;
        
        
        
        firstBaby = newDummyPlayer( "dummy2@test.com", 19, 0,
                                    startPos,
                                    0,
                                    clothing );
        }
    else if( inTriggerNumber == t++ ) {        
        // grab baby
        sendDummyAction( &eve, "BABY", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // run away
        addToMove( 1, -1 );
        addToMove( 2, -2 );
        addToMove( 3, -3 );
        addToMove( 4, -4 );
        addToMove( 5, -5 );
        addToMove( 6, -6 );
        
        sendDummyMove( &eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        
        GridPos startPos = { 26, -17 };
        
        clothing.bottom = getObject( 200 );
        
        bob = newDummyPlayer( "dummy3@test.com", 352, 50,
                              startPos,
                              0,
                              clothing );

        

        GridPos startPosB = { 25, -19 };
        
        clothing.tunic = getObject( 201 );
        
        alice = newDummyPlayer( "dummy4@test.com", 350, 20,
                                startPosB,
                                67,
                                clothing );
        

        GridPos startPosC = { 28, -18 };
        
        clothing.tunic = NULL;
        
        bobby = newDummyPlayer( "dummy5@test.com", 347, 6,
                                startPosC,
                                0,
                                clothing );
        }
    else if( inTriggerNumber == t++ ) {
        // grab bow
        offset.x = 1;
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make ember
        offset.y = 1;
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop bow
        offset.x = 1;
        sendDummyAction( &bob, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab leaf
        offset.x = -1;
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab ember
        offset.y = 1;
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // walk nw
        addToMove( -1, 1 );
        
        sendDummyMove( &bob, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // start tinder
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // walk to kindling
        addToMove( 1, -1 );
        addToMove( 1, -2 );
        
        sendDummyMove( &bob, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // drop leaf
        offset.y = 1;
        sendDummyAction( &bob, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab kindling
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // walk back up
        addToMove( 0, 1 );
        addToMove( 0, 2 );
        
        sendDummyMove( &bob, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // start fire
        offset.x = -1;
        sendDummyAction( &bob, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // now Alice
        // walk up and over
        addToMove( 0, 1 );
        addToMove( 0, 2 );
        addToMove( -1, 3 );
        
        sendDummyMove( &alice, finishMove() );
        

        // now kid
        // walk up
        addToMove( 0, 1 );
        
        sendDummyMove( &bobby, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // light shaft
        offset.x = 1;
        sendDummyAction( &alice, "USE", offset );

        
        // grab skewer
        offset.x = 0;
        offset.y = 1;
        sendDummyAction( &bobby, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // light other kindling
        offset.x = -1;
        sendDummyAction( &alice, "USE", offset );
        

        // walk nw
        addToMove( -1, 1 );
        addToMove( -2, 2 );
        
        sendDummyMove( &bobby, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // skewer rabbit
        offset.x = -1;
        sendDummyAction( &bobby, "USE", offset );
        }


    }


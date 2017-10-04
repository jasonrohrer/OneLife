


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



static LiveDummySocket *eve = NULL;

static LiveDummySocket *firstBaby = NULL;



static void customTrigger( int inTriggerNumber ) {
    
    GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    
    
    if( inTriggerNumber == t++ ) {
        GridPos startPos = { 12, -11 };
        
        
        
        LiveDummySocket s = newDummyPlayer( "dummy1@test.com", 19, 14,
                                            startPos,
                                            0,
                                            clothing );
        
        dummySockets.push_back( s );

        
        eve = 
            dummySockets.getElement( dummySockets.size() - 1 );        
        }
    else if( inTriggerNumber == t++ ) {
        // grab stone
        offset.x = -1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( -1, -1 );
        addToMove( -1, -2 );
        
        sendDummyMove( eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // sharpen stone
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( 1, -1 );
        addToMove( 2, -2 );
        addToMove( 2, -3 );
        
        sendDummyMove( eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // cut reeds
        offset.x = -1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop stone
        offset.y = -1;
        sendDummyAction( eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab reeds
        offset.x = -1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // walk to milkweed patch A
        addToMove( 0, 1 );
        addToMove( 0, 2 );
        addToMove( -1, 2 );
        addToMove( -2, 2 );
        
        sendDummyMove( eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // grab stalk
        offset.x = -1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other stalk
        offset.x = 1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make strand at feet
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // walk to milkweed patch B
        addToMove( 0, -1 );

        sendDummyMove( eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // grab stalk
        offset.x = -1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // drop at feet
        sendDummyAction( eve, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other stalk
        offset.x = 1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make strand at feet
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab other strand
        offset.y = 1;
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // make rope at feet
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab rope
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // walk back to reeds
        addToMove( 1, 0 );
        addToMove( 2, -1 );
        
        sendDummyMove( eve, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        // make skirt
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // grab skirt
        sendDummyAction( eve, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {
        // put on skirt
        sendDummyAction( eve, "SELF", offset, true, 4 );
        }
    else if( inTriggerNumber == t++ ) {

        GridPos startPos = eve->pos;
        
        
        
        LiveDummySocket s = newDummyPlayer( "dummy2@test.com", 19, 0,
                                            startPos,
                                            0,
                                            clothing );
        
        dummySockets.push_back( s );

        
        firstBaby = 
            dummySockets.getElement( dummySockets.size() - 1 );        
        }
    else if( inTriggerNumber == t++ ) {        
        // grab baby
        sendDummyAction( eve, "BABY", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        // run away
        addToMove( 1, -1 );
        addToMove( 2, -2 );
        addToMove( 3, -3 );
        addToMove( 4, -4 );
        addToMove( 5, -5 );
        addToMove( 6, -6 );
        
        sendDummyMove( eve, finishMove() );
        }
    
    }


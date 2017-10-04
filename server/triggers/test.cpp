


static void customInit() {
    ClothingSet clothing = getEmptyClothingSet();
    
    clothing.hat = getObject( 199 );
    clothing.bottom = getObject( 200 );
    
    PlayerMapping narrator = {
        "test@test.com",
        433,
        20,
        { 0, 0 },
        0, clothing };
    
    playerMap.push_back( narrator );
    }



static LiveDummySocket alice;



static void customTrigger( int inTriggerNumber ) {
    
    GridPos offset = { 0, 0 };
    ClothingSet clothing = getEmptyClothingSet();

    
    // increment this as we check for matches
    // thus, we can rearrange trigger order below
    // without having to update all the trigger numbers
    int t = 0;
    
    
    if( inTriggerNumber == t++ ) {
        setMapObject( 2, 0, 418 );
        }
    else if( inTriggerNumber == t++ ) {
        setMapObject( 2, 0, 153 );
        }
    else if( inTriggerNumber == t++ ) {
        GridPos startPos = { 1, 1 };
        
        clothing.hat = getObject( 199 );
        clothing.bottom = getObject( 200 );
        
        
        alice = newDummyPlayer( "dummy1@test.com", 350, 20,
                                startPos,
                                71,
                                clothing );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( 1, 0 );
        addToMove( 2, 0 );
        
        sendDummyMove( &alice, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        offset.y = 1;
        sendDummyAction( &alice, "DROP", offset, true, -1 );
        }
    else if( inTriggerNumber == t++ ) {
        addToMove( -1, 0 );
        addToMove( -2, 0 );
        
        sendDummyMove( &alice, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        offset.y = 1;
        sendDummyAction( &alice, "USE", offset );
        }
    else if( inTriggerNumber == t++ ) {        
        addToMove( -1, 0 );

        sendDummyMove( &alice, finishMove() );
        }
    else if( inTriggerNumber == t++ ) {
        offset.y = 1;
        sendDummyAction( &alice, "USE", offset );
        }
    
    }


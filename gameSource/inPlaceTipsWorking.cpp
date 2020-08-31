// add these to LivingLifePage.h
        int mCurTipID;
        doublePair mCurTipPos;



// add to LivingLifePage.cpp
// insert right after loop to draw location speech

    if( mCurMouseOverID > 0 &&
        mCurTipID != mCurMouseOverID ) {
        
        mCurTipID = mCurMouseOverID;
        mCurTipPos.x = lastMouseX;
        mCurTipPos.y = lastMouseY;
        }
    else if( mCurMouseOverID == 0 ) {
        mCurTipID = 0;
        }
    
    
    if( ! takingPhoto )
    if( mCurTipID > 0 && 
        ourLiveObject != NULL && 
        ourLiveObject->holdingID >= 0 ) {

        int holdingID = ourLiveObject->holdingID;
        
        if( holdingID > 0 && getObject( holdingID )->isUseDummy ) {
            holdingID = getObject( holdingID )->useDummyParent;
            }

        char *des = NULL;

        const char *actionKey = "take";
        

        TransRecord *trans = getTrans( holdingID, mCurTipID );
        if( trans != NULL ) {
            
            int targetObject = trans->newActor;

            if( targetObject > 0 && 
                getObject( targetObject )->isUseDummy ) {
                targetObject = getObject( targetObject )->useDummyParent;
                }
            

            if( targetObject == 0 ||
                targetObject == holdingID ) {
                // actor doesn't change
                
                if( trans->newTarget > 0 ) {
                    targetObject = trans->newTarget;
                    actionKey = "make";
                    }
                }
            else if( ourLiveObject->holdingID != 0 ) {
                actionKey = "make";
                }
            
            if( targetObject > 0 ) {
                des = 
                    stringToUpperCase( getObject( targetObject )->description );
                }
            }
        else if( ! getObject( mCurTipID )->permanent ) {
            // keep take key
            des = stringToUpperCase( getObject( mCurTipID )->description );
            }
        else {
            // mousing over a permanent with no transition
            // show no tip at all
            }
        
        if( des != NULL ) {
            
        
            stripDescriptionComment( des );

            char *verbDes = autoSprintf( "%s %s", translate( actionKey ),
                                         des );
            delete [] des;
            

            double fullWidth = handwritingFont->measureString( verbDes );
            
            doublePair drawPos = mCurTipPos;
            
            drawPos.x -= fullWidth / 2;
            drawPos.y += 65;
            
            drawChalkBackgroundString( drawPos, verbDes, 
                                       1.0f, 99999 );
            delete [] verbDes;
            }
        }

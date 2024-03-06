#include "AutoUpdatePage.h"


#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "minorGems/game/game.h"
#include "minorGems/game/drawUtils.h"

#include "message.h"



AutoUpdatePage::AutoUpdatePage()
        : mUseAHAPMessaging( false ),
          mAutoRelaunch( true ) {
    }



void AutoUpdatePage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    
    float progress = getUpdateProgress();

    const char *messageKey;
    if( progress < 1 ) {
        if( mUseAHAPMessaging ) {
            messageKey = "downloadingUpdateAHAP";
            }
        else {
            messageKey = "downloadingUpdate";
            }
        }
    else {
        if( mUseAHAPMessaging ) {
            messageKey = "applyingUpdateAHAP";
            }
        else {
            messageKey = "applyingUpdate";
            }
        }

    doublePair labelPos = { 0, 100 };

    
    drawMessage( translate( messageKey ), labelPos, false );

    // border
    setDrawColor( 1, 1, 1, 1 );
    
    drawRect( -100, -10, 
               100, 10 );

    // inner black
    setDrawColor( 0, 0, 0, 1 );
    
    drawRect( -98, -8, 
               98, 8 );
    
    
    // progress
    setDrawColor( .8, .8, .8, 1 );
    drawRect( -98, -8, 
               -98 + progress * ( 98 * 2 ), 8 );

    }

        
void AutoUpdatePage::step() {
    int result = stepUpdate();

    if( result == -1 ) {
        clearUpdate();
        
        if( wasUpdateWriteError() ) {
            setSignal( "writeError" );
            }
        else {    
            setSignal( "failed" );
            }
        
        return;
        }
    
    if( result == 1 ) {
        clearUpdate();
        
        if( ! mAutoRelaunch ) {
            printf( "AutoUpdatePage NOT re-launching game post-update.\n" );
            
            setSignal( "updateDone" );
            }
        else {
            printf( "AutoUpdatePage re-launching game post-update.\n" );
            
            char relaunched = relaunchGame();
        
            if( !relaunched ) {
                printf( "Relaunch failed\n" );
                setSignal( "relaunchFailed" );
                }
            else {
                printf( "Relaunched... but did not exit?\n" );
                setSignal( "relaunchFailed" );
                }
            }
        }
    
    }



void AutoUpdatePage::setUseAHAPMessaging( char inAHAP ) {
    mUseAHAPMessaging = inAHAP;
    }


        
void AutoUpdatePage::setAutoRelaunch( char inRelaunch ) {
    mAutoRelaunch = inRelaunch;
    }


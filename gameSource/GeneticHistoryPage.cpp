#include "GeneticHistoryPage.h"

#include "fitnessScore.h"


#include "buttonStyle.h"

#include "message.h"


#include "minorGems/game/Font.h"

#include "minorGems/game/game.h"


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"



extern Font *mainFont;


GeneticHistoryPage::GeneticHistoryPage() 
        : mBackButton( mainFont, -542, 300, translate( "backButton" ) ),
          mRefreshButton( mainFont, 542, 300, translate( "refreshButton" ) ),
          mLeaderboardButton( mainFont, 322, 300, translate( "leaderboard" ) ),
          mRefreshTime( 0 ) {

    setButtonStyle( &mBackButton );
    setButtonStyle( &mRefreshButton );
    setButtonStyle( &mLeaderboardButton );
    
    mBackButton.setVisible( true );
    mRefreshButton.setVisible( false );
    mLeaderboardButton.setVisible( false );
    
    mBackButton.addActionListener( this );
    mRefreshButton.addActionListener( this );
    mLeaderboardButton.addActionListener( this );
    
    addComponent( &mBackButton );
    addComponent( &mRefreshButton );
    addComponent( &mLeaderboardButton );
    }


GeneticHistoryPage::~GeneticHistoryPage() {
    }

        

void GeneticHistoryPage::actionPerformed( GUIComponent *inTarget ) {

    if( inTarget == &mBackButton ) {
        setSignal( "done" );
        }
    else if( inTarget == &mRefreshButton ) {
        triggerFitnessScoreDetailsUpdate();
        mRefreshButton.setVisible( false );
        mRefreshTime = game_getCurrentTime();
        }
    else if( inTarget == &mLeaderboardButton ) {
        char *url = SettingsManager::getStringSetting( "fitnessServerURL", "" );

        if( strcmp( url, "" ) != 0 ) {
            
            char *fullURL = autoSprintf( "%s?action=show_leaderboard", url );
            
            launchURL( fullURL );
            delete [] fullURL;
            }
        }
    }



void GeneticHistoryPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    if( ! mRefreshButton.isVisible() &&
        game_getCurrentTime() - mRefreshTime > 10 ) {
        // enforce 10 seconds between refreshes
        mRefreshButton.setVisible( true );
        }

    if( isFitnessScoreReady() ){
        mLeaderboardButton.setVisible( true );
        }
    

    doublePair pos = { 0, 300 };
    drawFitnessScoreDetails( pos );
    }




void GeneticHistoryPage::makeActive( char inFresh ) {
    if( ! inFresh ) {
        return;
        }

    
    if( game_getCurrentTime() - mRefreshTime > 10 ) {
        triggerFitnessScoreDetailsUpdate();
        mRefreshButton.setVisible( false );
        mLeaderboardButton.setVisible( false );
        mRefreshTime = game_getCurrentTime();
        }
    }


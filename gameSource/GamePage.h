#ifndef GAME_PAGE_INCLUDED
#define GAME_PAGE_INCLUDED


#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"

#include "PageComponent.h"


class GamePage : public PageComponent {
        

    public:
        

        virtual ~GamePage();
        
        void setStatus( const char *inStatusMessageKey, char inError );

        // inStatusMessage destroyed by caller
        void setStatusDirect( const char *inStatusMessage, char inError );

        char isStatusShowing();
        

        // overrides default tip position
        // tip defaults to bottom of screen
        void setTipPosition( char inTop );

        // overrides default status position
        // status messages default to bottom of screen
        void setStatusPositiion( char inTop );
        
        
        // override these from PageComponent to actually SHOW
        // the tool tip, instead of passing it further up the parent chain
        // inTip can either be a translation key or a raw tip
        // copied internally
        virtual void setToolTip( const char *inTip );
        virtual void clearToolTip( const char *inTipToClear );
        

        
        // inFresh set to true when returning to this page
        // after visiting other pages
        // set to false after returning from pause.
        // (if inFresh is true, any signal is cleared)
        void base_makeActive( char inFresh );
        void base_makeNotActive();


        // override to draw status message
        virtual void base_draw( doublePair inViewCenter, 
                                double inViewSize );


        // override to step waiting display
        virtual void base_step();

        
        // override to catch keypress to dismiss shutdown warning
        // overlay
        virtual void base_keyDown( unsigned char inASCII );
        


        // signals are string flags that can be set by a page
        // so that the state of the page can be checked cleanly from outside
        // For example, a "nextPage" signal could be set when a given
        // page is ready to switch to the next page in a sequence,
        // or a "menu" signal could be set when the user wants to return
        // back to the menu page.

        // at most one signal can be set at a time per page

        // this mechanism eliminates multiple checking methods, while
        // also limiting exposure of private variables (that seem
        // unnatural to overload for both internal operations and
        // external state checking).

        // also, implementing signals as separate boolean variables
        // (like an mNextPage and an mReturnToMenu variable) means
        // that they need to be declared, initialized, and maintained.

        virtual char checkSignal( const char *inSignalName );
        
        
    protected:
        

        // set to true to skip drawing sub-components
        // (only results of drawUnderComponents and draw will be shown)
        // Used to "hide the UI" cleanly.
        // defaults to false
        void skipDrawingSubComponents( char inSkip );
        

        // subclasses override these to provide custom functionality

        virtual void step() {
            };
        
        // called before added sub-components have been drawn
        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize ) {
            };
        // called after added sub-components have been drawn
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize ) {
            };
        
        // inFresh set to true when returning to this page
        // after visiting other pages
        // set to false after returning from pause.
        virtual void makeActive( char inFresh ) {
            };
        virtual void makeNotActive() {
            };


        virtual void pointerMove( float inX, float inY );
        

        virtual void pointerDown( float inX, float inY ) {
            };

        virtual void pointerDrag( float inX, float inY ) {
            pointerMove( inX, inY );
            };

        virtual void pointerUp( float inX, float inY ) {
            };

        virtual void keyDown( unsigned char inASCII ) {
            };
        
        virtual void keyUp( unsigned char inASCII ) {
            };

        virtual void specialKeyDown( int inKeyCode ) {
            };

        virtual void specialKeyUp( int inKeyCode ) {
            };
        

        // subclasses can override this to selectively permit
        // the waiting icon to be drawn at particular times
        virtual char canShowWaitingIcon() {
            return true;
            }
        
        // subclasses can override this to control size of waiting icon
        // displayed on their pages
        virtual char makeWaitingIconSmall() {
            return false;
            }
        
        // subclasses can override this to block orange warning color
        // on status icon when we're past the 1/2 retry time point
        // (if this call returns true, only the red retry color will be shown)
        virtual char noWarningColor() {
            return false;
            }
        

        // override this from PageComponent to show waiting status
        virtual void setWaiting( char inWaiting,
                                 char inWarningOnly = false );
        


        // shows an overlay message warning the user that
        // a server shutdown is pending
        virtual void showShutdownPendingWarning();
        

        // sets the named signal and clears previous signal
        virtual void setSignal( const char *inSignalName );
        
        // clears any signal
        // signal is also auto-cleared by base_makeActive( true ) 
        virtual void clearSignal();
        


        GamePage();
        

        char mStatusError;
        const char *mStatusMessageKey;
        char *mStatusMessage;
        
        char mSkipDrawingSubComponents;
        

        char *mTip;
        char *mLastTip;
        double mLastTipFade;
        
        char mTipAtTopOfScreen;
        char mStatusAtTopOfScreen;

        char *mSignal;
        

        static int sPageCount;

        static SpriteHandle sWaitingSprites[3];
        static SpriteHandle sResponseWarningSprite;
        
        static int sCurrentWaitingSprite;
        static int sLastWaitingSprite;
        static int sWaitingSpriteDirection;
        static double sCurrentWaitingSpriteFade;
        
        static char sResponseWarningShowing;
        static doublePair sResponseWarningPosition;

        char mResponseWarningTipShowing;
        
        static double sWaitingFade;
        static char sWaiting;
        static char sShowWaitingWarningOnly;
        
        static char sShutdownPendingWarning;
    };


#endif


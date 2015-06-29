#ifndef PAGE_COMPONENT_INCLUDED
#define PAGE_COMPONENT_INCLUDED


#include "minorGems/ui/GUIComponent.h"

#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/doublePair.h"


class PageComponent : public GUIComponent {
    public:
        
        virtual ~PageComponent() {
            };

        
        virtual doublePair getCenter();
        
        
        virtual void setParent( PageComponent *inParent );

        
        
        
        virtual void setVisible( char inIsVible ) {
            mVisible = inIsVible;
            }
        
        
        virtual char isVisible() {
            return mVisible;
            }
        
        
        virtual char isActive() {
            return true;
            }
        
        
        virtual void base_step();
        
        virtual void base_draw( doublePair inViewCenter, 
                                double inViewSize );
        
        void setPosition( double inX, double inY );

        doublePair getPosition();

        // set to true to ignore events and NOT
        // pass them on to sub-components
        // still handles and passes UP events (so that
        // down states that were invoked before the ignore started
        // do not get stuck)
        void setIgnoreEvents( char inIgnoreEvents );
        

        void base_pointerMove( float inX, float inY );

        void base_pointerDown( float inX, float inY );

        void base_pointerDrag( float inX, float inY );

        void base_pointerUp( float inX, float inY );

        virtual void base_keyDown( unsigned char inASCII );
        
        void base_keyUp( unsigned char inASCII );

        void base_specialKeyDown( int inKeyCode );

        void base_specialKeyUp( int inKeyCode );

        
        void base_clearState();
        
        
    protected:
        
        // inX, inY specify center position of this component
        // Positions of sub components are relative to this center position
        PageComponent( double inX, double inY );
        
        
        // add for default event handling
        void addComponent( PageComponent *inComponent );

        // default implementations pass tool tip up to parent


        // inTip destroyed by caller, or NULL to clear
        virtual void setToolTip( const char *inTip );

        // clears a specific tool tip---only if this tip is still showing
        virtual void clearToolTip( const char *inTipToClear );
        

        // clears hover or partially-pressed status
        virtual void clearState() {
            };


        virtual void step() {
            };
        
        virtual void draw() {
            };

        
        // these calls receive pointer coordinates relative to component center
        
        virtual void pointerMove( float inX, float inY ) {
            };

        virtual void pointerDown( float inX, float inY ) {
            };

        virtual void pointerDrag( float inX, float inY ) {
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
        


        // passes a waiting status from this component, up through parents,
        // to containing GamePage
        // Indicates whether we're in a waiting state (like waiting on 
        // a server response) so that an appropriate indicator can be 
        // displayed.
        // inWaiting is true if waiting, false if done waiting.
        // inSmallIcon is true if there's not room on page for larger warning
        // icon
        // inWarningOnly is true if the waiting icon should be hidden
        //               during normal, non-delayed network operations.
        virtual void setWaiting( char inWaiting,
                                 char inWarningOnly = false );
        


        // this component blocks all other sibling sub-components from
        // receiving events
        void setHogMouseEvents( char inHogMouseEvents );
        

        // sets the event hog amoung the sub-components of this component
        // or NULL to set no event hog
        // auto-cleared with clearState called
        void setMouseEventHog( PageComponent *inHog );
        


        SimpleVector<PageComponent*> mComponents;
        
        double mX, mY;

        PageComponent *mParent;
        
        char mVisible;

        char mIgnoreEvents;

        PageComponent *mMouseEventHog;

    };



#endif

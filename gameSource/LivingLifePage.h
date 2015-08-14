#ifndef LIVING_LIFE_PAGE_INCLUDED
#define LIVING_LIFE_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"



class LivingLifePage : public GamePage {
        
    public:

        LivingLifePage();
        ~LivingLifePage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

        virtual void keyDown( unsigned char inASCII );
        
        virtual void keyUp( unsigned char inASCII );

        
    protected:
        
        char *mServerAddress;
        int mServerPort;

        int mServerSocket;

        int mFirstServerMessagesReceived;
        
        


        int mMapD;

        int *mMap;

        int mMapOffsetX;
        int mMapOffsetY;


        char mEKeyDown;

        
    };



#endif

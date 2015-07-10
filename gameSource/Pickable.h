#ifndef PICKABLE_INCLUDED
#define PICKABLE_INCLUDED


#include "minorGems/game/doublePair.h"



// interface for a Picker plug in

class Pickable {
    public:

        virtual ~Pickable() {
            };

        
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) = 0;

        virtual void draw( void *inObject, doublePair inPos ) = 0;


        virtual int getID( void *inObject ) = 0;
        

        virtual char canDelete( int inID ) = 0;
        
        virtual void deleteID( int inID ) = 0;
        
        
        // not destroyed by caller
        virtual const char *getText( void *inObject ) = 0;
        

        
    };


#endif
        

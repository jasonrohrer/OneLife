#ifndef OBJECT_PICKABLE_INCLUDED
#define OBJECT_PICKABLE_INCLUDED


#include "Pickable.h"


#include "objectBank.h"
#include "transitionBank.h"


#include "EditorTransitionPage.h"

extern EditorTransitionPage *transPage;


class ObjectPickable : public Pickable {
        
    public:
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {

            return (void**)searchObjects( inSearch, inNumToSkip, inNumToGet,
                                          outNumResults, outNumRemaining );
            }
        


        virtual void draw( void *inObject, doublePair inPos ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            drawObject( r, inPos );
            }



        virtual int getID( void *inObject ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            return r->id;
            }
        


        virtual char canDelete( int inID ) {
            return ! isObjectUsed( inID );
            }
        

        
        virtual void deleteID( int inID ) {
            transPage->clearUseOfObject( inID );
            deleteObjectFromBank( inID );
            }



        virtual const char *getText( void *inObject ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            return r->description;
            }
        
    };
        

#endif

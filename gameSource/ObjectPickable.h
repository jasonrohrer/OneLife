#ifndef OBJECT_PICKABLE_INCLUDED
#define OBJECT_PICKABLE_INCLUDED


#include "Pickable.h"


#include "objectBank.h"
#include "spriteBank.h"
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

            int maxD = 0;
            
            for( int i=0; i<r->numSprites; i++ ) {
                doublePair pos = r->spritePos[i];
                
                int rad = getSpriteRecord( r->sprites[i] )->maxD / 2;
                
                int xR = lrint( fabs( pos.x ) + rad );
                int yR = lrint( fabs( pos.y ) + rad );
                
                int xD = 2 * xR;
                int yD = 2 * yR;

                if( xD > maxD ) {
                    maxD = xD;
                    }
                if( yD > maxD ) {
                    maxD = yD;
                    }
                }
            
            double zoom = 64.0 / maxD;
            
            drawObject( r, inPos, false, -1, getEmptyClothingSet(), zoom );
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

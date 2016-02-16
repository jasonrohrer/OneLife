#ifndef SPRITE_PICKABLE_INCLUDED
#define SPRITE_PICKABLE_INCLUDED


#include "Pickable.h"


#include "spriteBank.h"
#include "objectBank.h"


#include "EditorObjectPage.h"

extern EditorObjectPage *objectPage;


class SpritePickable : public Pickable {
        
    public:
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {

            return (void**)searchSprites( inSearch, inNumToSkip, inNumToGet,
                                          outNumResults, outNumRemaining );
            }
        


        virtual void draw( void *inObject, doublePair inPos ) {
            SpriteRecord *r = (SpriteRecord*)inObject;
            
            double zoom = 64.0 / r->maxD;

            drawSprite( r->sprite, inPos, zoom );
            }



        virtual int getID( void *inObject ) {
            SpriteRecord *r = (SpriteRecord*)inObject;
            
            return r->id;
            }
        

        virtual char canDelete( int inID ) {
            return ! isSpriteUsed( inID );
            }
        
        
        virtual void deleteID( int inID ) {
            objectPage->clearUseOfSprite( inID );
            deleteSpriteFromBank( inID );
            }
        
        

        virtual const char *getText( void *inObject ) {
            SpriteRecord *r = (SpriteRecord*)inObject;
            
            return r->tag;
            }
        
    };
        

#endif

#ifndef OVERLAY_PICKABLE_INCLUDED
#define OVERLAY_PICKABLE_INCLUDED


#include "Pickable.h"


#include "overlayBank.h"


#include "EditorImportPage.h"

extern EditorImportPage *importPage;


class OverlayPickable : public Pickable {
        
    public:
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {

            if( strcmp( inSearch, "." ) == 0 ) {
                int *stackResults = 
                    getStackRange( inNumToSkip, inNumToGet,
                                   outNumResults, outNumRemaining );
                void **results = new void *[ *outNumResults ];
                
                for( int i=0; i< *outNumResults; i++ ) {
                    results[i] = (void*)getOverlay( stackResults[i] );
                    }

                delete [] stackResults;
                return results;
                }
            else {
                return (void**)searchOverlays( inSearch, inNumToSkip, 
                                               inNumToGet,
                                               outNumResults, 
                                               outNumRemaining );
                }
            
            }
        


        virtual void draw( void *inItem, doublePair inPos ) {
            OverlayRecord *r = (OverlayRecord*)inItem;
            
            int maxD = r->image->getHeight();
            int w = r->image->getWidth();
            
            if( w > maxD ) {
                maxD = w;
                }

            double scale = 64.0 / maxD;
            
            drawSprite( r->thumbnailSprite, inPos, scale );
            }



        virtual int getID( void *inItem ) {
            OverlayRecord *r = (OverlayRecord*)inItem;
            
            return r->id;
            }
        
        
        
        virtual void *getObjectFromID( int inID ) {
            OverlayRecord *o = getOverlay( inID );
            
            return (void*) o;
            }        
        
        
        
        virtual char canDelete( int inID ) {
            return true;
            }
        
        
        virtual void deleteID( int inID ) {
            importPage->clearUseOfOverlay( inID );
            deleteOverlayFromBank( inID );
            }
        
        

        virtual const char *getText( void *inItem ) {
            OverlayRecord *r = (OverlayRecord*)inItem;
            
            return r->tag;
            }


    protected:

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;

    };
        

#endif

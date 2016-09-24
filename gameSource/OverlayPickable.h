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
        


        virtual void draw( void *inObject, doublePair inPos ) {
            OverlayRecord *r = (OverlayRecord*)inObject;
            
            int maxD = r->image->getHeight();
            int w = r->image->getWidth();
            
            if( w > maxD ) {
                maxD = w;
                }

            double scale = 64.0 / maxD;
            
            drawSprite( r->thumbnailSprite, inPos, scale );
            }



        virtual int getID( void *inObject ) {
            OverlayRecord *r = (OverlayRecord*)inObject;
            
            return r->id;
            }
        

        virtual char canDelete( int inID ) {
            return true;
            }
        
        
        virtual void deleteID( int inID ) {
            importPage->clearUseOfOverlay( inID );
            deleteOverlayFromBank( inID );
            }
        
        

        virtual const char *getText( void *inObject ) {
            OverlayRecord *r = (OverlayRecord*)inObject;
            
            return r->tag;
            }


    protected:

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;

    };
        

#endif

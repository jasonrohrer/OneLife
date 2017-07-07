#ifndef CATEGORY_PICKABLE_INCLUDED
#define CATEGORY_PICKABLE_INCLUDED


#include "Pickable.h"


#include "categoryBank.h"
#include "objectBank.h"
#include "transitionBank.h"


#include "EditorTransitionPage.h"

extern EditorTransitionPage *transPage;



class CategoryPickable : public Pickable {
        
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
                    results[i] = (void*)getCategory( stackResults[i] );
                    }                

                delete [] stackResults;
                return results;
                }
            else {
                return (void**)searchCategories( inSearch, inNumToSkip, 
                                                 inNumToGet,
                                                 outNumResults, 
                                                 outNumRemaining );
                }
            }
        
        


        virtual void draw( void *inCategory, doublePair inPos ) {
            CategoryRecord *r = (CategoryRecord*)inCategory;
            
            if( r->objectIDSet.size() > 0 ) {
                
                ObjectRecord *obR = 
                    getObject( r->objectIDSet.getElementDirect(0) );
                
                if( obR != NULL ) {
                    
            
                    int maxD = getMaxDiameter( obR );
            
                    double zoom = 1;
                    
                    if( maxD > 64 ) {    
                        zoom = 64.0 / maxD;
                        }
                    
                    inPos = sub( inPos, 
                                 mult( getObjectCenterOffset( obR ), zoom ) );

                    drawObject( obR, 2, inPos, 0, false, false, 
                                20, 0, false, false,
                                getEmptyClothingSet(), zoom );
                    }
                }
            }
        



        virtual int getID( void *inCategory ) {
            CategoryRecord *r = (CategoryRecord*)inCategory;
            
            return r->id;
            }
        


        virtual char canDelete( int inID ) {
            return ! isCategoryUsed( inID );
            }
        

        
        virtual void deleteID( int inID ) {
            Pickable::deleteID( inID );
            
            transPage->clearUseOfCategory( inID );
            deleteCategoryFromBank( inID );
            }



        virtual const char *getText( void *inCategory ) {
            CategoryRecord *r = (CategoryRecord*)inCategory;
            
            return r->description;
            }
        
    protected:
        

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;

    };
        

#endif

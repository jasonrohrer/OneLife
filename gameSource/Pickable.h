#ifndef PICKABLE_INCLUDED
#define PICKABLE_INCLUDED


#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"



// interface for a Picker plug in

class Pickable {
    public:

        virtual ~Pickable() {
            };

        
        // search for "." shows stack of recently used
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) = 0;
        
        

        virtual void draw( void *inObject, doublePair inPos ) = 0;


        virtual int getID( void *inObject ) = 0;
        

        virtual char canDelete( int inID ) = 0;

        virtual char isSearchable() {
            return true;
            }
        

        // sub classed must call Pickable::deleteID()
        virtual void deleteID( int inID ) {
            getStack()->deleteElementEqualTo( inID );
            }
        
        
        
        // registers use, putting it on top of stack
        virtual void usePickable( int inID ) {
            SimpleVector<int> *stack = getStack();
            
            stack->deleteElementEqualTo( inID );
            stack->push_front( inID );
            }
        
        
        // not destroyed by caller
        virtual const char *getText( void *inObject ) = 0;
        
    protected:
        
        // sub classes return their static stacks
        // (so that every picker shares the same stack)
        virtual SimpleVector<int> *getStack() = 0;
        
        
        virtual int *getStackRange( int inNumToSkip, int inNumToGet,
                                    int *outNumResults, int *outNumRemaining ) {
            
            SimpleVector<int> *stack = getStack();
            
            int numAvail = stack->size();
            

            if( numAvail - inNumToSkip < inNumToGet ) {
                inNumToGet = numAvail - inNumToSkip;
                *outNumRemaining = 0;
                }
            else {
                *outNumRemaining = numAvail - ( inNumToSkip + inNumToGet );
                }

            *outNumResults = inNumToGet;
            
            int *returnArray = new int[ inNumToGet ];
            
            int arrayI = 0;
            for( int i=inNumToSkip; i<inNumToSkip+inNumToGet; i++ ) {
                returnArray[ arrayI ] = stack->getElementDirect( i );
                arrayI++;
                }
            
            return returnArray;
            }
        
        
    };


#endif
        

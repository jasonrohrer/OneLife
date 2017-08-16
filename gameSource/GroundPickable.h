#ifndef GROUND_PICKABLE_INCLUDED
#define GROUND_PICKABLE_INCLUDED


#include "Pickable.h"


#include "groundSprites.h"
#include "objectBank.h"


#define QUOTE( arg )  #arg
#define STRING_VAL( arg )  QUOTE( arg )
#define NUM_GROUND_STRING_NAMES 30


class GroundPickable : public Pickable {
        
    public:
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {
            
            // ignore search strings

            SimpleVector<int> allBiomes;
            
            getAllBiomes( &allBiomes );

            SimpleVector<GroundSpriteSet*> allGround;
            
            for( int i=0; i<allBiomes.size(); i++ ) {
                
                int b = allBiomes.getElementDirect( i );
                
                if( groundSprites[ b ] != NULL ) {
                    allGround.push_back( groundSprites[ b ] );
                    }
                }

            *outNumResults = 0;
            
            if( inNumToSkip < allGround.size() ) {
                *outNumResults = allGround.size() - inNumToSkip;
                }
            
            *outNumRemaining = 0;
            
            if( *outNumResults > inNumToGet ) {
                *outNumRemaining = *outNumResults - inNumToGet;
                
                *outNumResults = inNumToGet;
                }
            
            void **results = new void *[ *outNumResults ];

            for( int i=0; i<*outNumResults; i++ ) {
                int g = i + inNumToSkip;
                
                results[i] = allGround.getElementDirect( g );
                }
            
            return results;
            }
        


        virtual void draw( void *inObject, doublePair inPos ) {
            GroundSpriteSet *r = (GroundSpriteSet*)inObject;

            // don't access r->sprite directly here
            // getSprite needed to invoke dynamic sprite loading
            SpriteHandle sprite = r->squareTiles[0][0];

            double zoom = 0.4;

            drawSprite( sprite, inPos, zoom );
            }


        virtual char isSearchable() {
            return false;
            }
        

        virtual int getID( void *inObject ) {
            GroundSpriteSet *r = (GroundSpriteSet*)inObject;
            
            return r->biome;
            }
        

        virtual char canDelete( int inID ) {
            return false;
            }
        
        
        virtual void deleteID( int inID ) {
            }
        
        

        virtual const char *getText( void *inObject ) {
            GroundSpriteSet *r = (GroundSpriteSet*)inObject;

            if( r->biome < NUM_GROUND_STRING_NAMES ) {
                return sStringNames[r->biome];
                }
            else {
                return "? >= " STRING_VAL( NUM_GROUND_STRING_NAMES );
                }
            }

    protected:
        

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;


        
        static const char *sStringNames[ NUM_GROUND_STRING_NAMES ];
        
        
    };
        

#endif

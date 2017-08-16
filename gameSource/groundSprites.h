#ifndef GROUND_SPRITES_INCLUDED
#define GROUND_SPRITES_INCLUDED

#include "minorGems/game/gameGraphics.h"

#define CELL_D 128



typedef struct GroundSpriteSet {
        int biome;
        
        int numTilesHigh;
        int numTilesWide;
        
        // indexed as [y][x]
        SpriteHandle **tiles;

        SpriteHandle **squareTiles;

        // all tiles together in one image
        SpriteHandle wholeSheet;
    } GroundSpriteSet;



// array sized for largest biome ID for direct indexing
// sparse, with NULL entries

// One extra place in array for unknown biome sprite
extern int groundSpritesArraySize;
extern GroundSpriteSet **groundSprites;


// object bank must be inited first



// loads from objects folder
// returns number of ground tiles that need to be loaded
int initGroundSpritesStart( char inPrintSteps=true );

// returns progress... ready for Finish when progress == 1.0
float initGroundSpritesStep();
void initGroundSpritesFinish();

void freeGroundSprites();

#endif

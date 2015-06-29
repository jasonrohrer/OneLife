#include "minorGems/game/gameGraphics.h"



// loads an RGB sprite from a TGA file in the graphics folder
// that features a white image on a black background, where
// darkness is interpreted as transparency.
// (Darkness is measured through red channel only, and other channels
//  are ignored)
// 
// outW, outH pointers to where width and height in pixels are returned
// (default to NULL, not returned)
//
// Returns NULL on load error
SpriteHandle loadWhiteSprite( const char *inTGAFileName,
                              int *outW = NULL,
                              int *outH = NULL );


SpriteHandle fillWhiteSprite( Image *inImage );

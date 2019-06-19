#include "minorGems/game/doublePair.h"
#include "minorGems/game/Font.h"



// defaults to alignCenter
void setMessageAlign( TextAlignment inAlign );

TextAlignment getMessageAlign();



// draws multi-line message (lines separated by ##) using a translation key
// top line centered on inCenter
void drawMessage( const char *inTranslationKey, doublePair inCenter,
                  char inRed = false, double inFade = 1.0 );

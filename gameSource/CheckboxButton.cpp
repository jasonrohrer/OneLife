#include "CheckboxButton.h"



CheckboxButton::CheckboxButton( double inX, double inY,
                                double inDrawScale )
        : SpriteToggleButton( "checkMarkOff.tga",
                              "checkMark.tga",
                              inX, inY,
                              inDrawScale ) {

    mWide = mWide / 2;
    mHigh = mHigh / 2;
    }

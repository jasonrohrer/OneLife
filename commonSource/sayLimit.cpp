#include <math.h>

int getSayLimit( double inAge ) {
    int floorAge = floor( inAge );
    
    int sayCap = (int)( floorAge + 1 );
    
    int adultBase = 50;

    if( floorAge >= 16 ) {
        sayCap = 16 + ( floorAge - 16 ) / 2 + adultBase;
        }
    else if ( floorAge >= 8 ) {
        // rise smoothly from 9 to ( 16 + adultBase) characters between
        // age 8 and age 16
        
        int extraAge = floorAge - 8;
        
        if( extraAge > 0 ) {
            int sixteenLimit = adultBase + 16;
            
            int fullIncrease = sixteenLimit - 9;
            
            double fraction = extraAge / 8.0;

            // sigmoid curve with 0,1 output for input values between 0 and 1
            double hardness = 12;
            double curvedFraction = 
                1.0 / ( 1.0 + pow( 2.0, - hardness *( fraction - 0.5 ) ) );
            
            double increase = fullIncrease * curvedFraction;
            
            sayCap = 9 + floor( increase );
            }
        }

    return sayCap;
    }


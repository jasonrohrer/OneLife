// Place in:
// C:\Users\Public\Documents\Lightworks\Effect Templates
//
// And also put a copy in:
// C:\Program Files\Lightworks\Shaders
//
// When adding effects to a clip, right click on list of possible
// effects and Create effect from .fx template.


//--------------------------------------------------------------//
// Lumakey
//
// Copyright (c) EditShare EMEA.  All Rights Reserved
//--------------------------------------------------------------//

int _LwksEffectInfo
<
   string EffectGroup = "GenericPixelShader";
   string Description = "Pixel Perfect 2D DVE";
   string Category    = "DVEs";
> = 0;

//--------------------------------------------------------------//
// Params
//--------------------------------------------------------------//


/*
// these parameters no longer needed
float PixelX
<
   string Description = "X";
   string Group = "Source Pixel Resolution";
   float MinVal = 1;
   float MaxVal = 1920;
> = 640;

float PixelY
<
   string Description = "Y";
   string Group = "Source Pixel Resolution";
   float MinVal = 1;
   float MaxVal = 1080;
> = 360;
*/


float CentreX
<
   string Description = "Position";
   string Flags = "SpecifiesPointX";
   float MinVal = -1.00;
   float MaxVal = 2.00;
> = 0.5;

float CentreY
<
   string Description = "Position";
   string Flags = "SpecifiesPointY";
   float MinVal = -1.00;
   float MaxVal = 2.00;
> = 0.5;

float MasterScale
<
   string Description = "Master";
   string Group = "Scale";
   float MinVal = 0.00;
   float MaxVal = 10.00;
> = 1.0;

float XScale
<
   string Description = "X";
   string Group = "Scale";
   float MinVal = 0.00;
   float MaxVal = 10.00;
> = 1.0;

float YScale
<
   string Description = "Y";
   string Group = "Scale";
   float MinVal = 0.00;
   float MaxVal = 10.00;
> = 1.0;

float CropLeft
<
   string Description = "Left";
   string Group = "Crop";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.0;

float CropTop
<
   string Description = "Top";
   string Group = "Crop";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.0;

float CropRight
<
   string Description = "Right";
   string Group = "Crop";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.0;

float CropBottom
<
   string Description = "Bottom";
   string Group = "Crop";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.0;

float ShadowTransparency
<
   string Description = "Transparency";
   string Group = "Shadow";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.75;

float ShadowXOffset
<
   string Description = "X Offset";
   string Group = "Shadow";
   float MinVal = -1.00;
   float MaxVal = 1.00;
> = 0.0;

float ShadowYOffset
<
   string Description = "Y Offset";
   string Group = "Shadow";
   float MinVal = -1.00;
   float MaxVal = 1.00;
> = 0.0;

float Opacity
<
   string Description = "Opacity";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 1.0;

float _MaxShadowOffset = 0.2;

//--------------------------------------------------------------//
// Inputs
//--------------------------------------------------------------//
texture Fg;
sampler FgSampler = sampler_state
{
   Texture = <Fg>;
   MINFILTER = Point;
   MIPFILTER = Point;
   MAGFILTER = Point;
};

float _FgNormWidth = 1.0;
float _FgWidth  = 10.0;
float _FgHeight = 10.0;

texture Bg;
sampler2D BgSampler = sampler_state { Texture = <Bg>; };


//--------------------------------------------------------------//
// Code
//--------------------------------------------------------------//
float4 ps_main( float2 xyNorm : TEXCOORD0, float2 xy1 : TEXCOORD1, float2 xy2 : TEXCOORD2 ) : COLOR
{
   float4 ret;

   float xScale        = MasterScale * XScale;
   float yScale        = MasterScale * YScale;
   float x             = CentreX;
   float y             = 1.0 - CentreY;
   float l             = x - ( xScale / 2 );
   float t             = y - ( yScale / 2 );
   float r             = l + xScale;
   float b             = t + yScale;
   float croppedL      = l + ( CropLeft * xScale );
   float croppedT      = t + ( CropTop  * yScale );
   float croppedR      = r - ( CropRight * xScale );
   float croppedB      = b - ( CropBottom * yScale );
   float scaledXShadow = ShadowXOffset * _MaxShadowOffset;
   float scaledYShadow = ShadowYOffset * _MaxShadowOffset;
   float shadowL       = croppedL + scaledXShadow;
   float shadowR       = croppedR + scaledXShadow;
   float shadowT       = croppedT + scaledYShadow;
   float shadowB       = croppedB + scaledYShadow;
   float2 texAddressAdjust = float2( 0.5 / _FgWidth, 0.5 / _FgHeight );

   float4 bgPixel = tex2D( BgSampler, xy2 );

   if ( xyNorm.x < croppedL ||  xyNorm.x > croppedR || xyNorm.y < croppedT || xyNorm.y > croppedB )
   {
      if ( ShadowXOffset != 0 || ShadowYOffset != 0 )
      {
         if ( xyNorm.x > shadowL && xyNorm.x < shadowR && xyNorm.y > shadowT && xyNorm.y < shadowB )
         {
            ret = lerp( 0, bgPixel, ShadowTransparency );
            ret = lerp( bgPixel, ret, Opacity );
         }
         else
         {
            ret = bgPixel;
         }
      }
      else
      {
         ret = bgPixel;
      }
   }
   else
   {
      float2 fgPos = float2( ( xyNorm.x - l ) / xScale, ( xyNorm.y - t ) / yScale );

      // Remember that the texCoords for the FG may not be 0 -> 1
      fgPos.x *= _FgNormWidth;
      fgPos += texAddressAdjust;

      if ( Opacity != 1.0 )
      {
         ret = lerp( tex2D( BgSampler, xy2 ), tex2D( FgSampler, fgPos ), Opacity );
      }
      else
      {
         // depend on point filtering above to deal with pixel sampling 
         ret = tex2D( FgSampler, fgPos );

         // round pixel sample position explicitly
         //ret = tex2D( FgSampler, 
         //             float2( floor( PixelX * fgPos.x ) / PixelX, 
         //                     floor( PixelY * fgPos.y ) / PixelY ) );
      }

      ret   = lerp( bgPixel, ret, ret.a );
      ret.a = 1.0;
   }

   return ret;
}

//--------------------------------------------------------------//
// Techniques
//--------------------------------------------------------------//
technique DVE { pass Single_Pass { PixelShader = compile PROFILE ps_main(); } }

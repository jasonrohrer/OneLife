// Place in:
// C:\Users\Public\Documents\Lightworks\Effect Templates
//
// And also put a copy in:
// C:\Program Files\Lightworks\Shaders
//
// On Linux, look in:
// /usr/share/lightworks
//
// When adding effects to a clip, right click on list of possible
// effects and Create effect from .fx template.



//--------------------------------------------------------------//

// Lightworks Pixel Perfect Zoom
// Modified by Jason Rohrer from original idea by Schrauber

//--------------------------------------------------------------//


int _LwksEffectInfo
<
   string EffectGroup = "GenericPixelShader";
   string Description = "Pixel Perfect Zoom";  
   string Category    = "DVE"; 
> = 0;



//--------------------------------------------------------------//
// Inputs und Samplers
//--------------------------------------------------------------//


texture Input;

sampler PointSampler = sampler_state
{
   Texture = <Input>;
   AddressU = Border;
   AddressV = Border;
   MinFilter = Point;
   MagFilter = Point;
   MipFilter = Point;
};






//--------------------------------------------------------------//
// Parameters
//--------------------------------------------------------------//


float Zoom
<
   string Description = "Zoom";
   float MinVal = 0.00;
   float MaxVal = 10.00;
> = 1.0;

float Xpos
<
   string Description = "Zoom center";
   string Flags = "SpecifiesPointX";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.5;

float Ypos
<
   string Description = "Zoom center";
   string Flags = "SpecifiesPointY";
   float MinVal = 0.00;
   float MaxVal = 1.00;
> = 0.5;





//--------------------------------------------------------------
// Shader
//--------------------------------------------------------------


float4 main (float2 uv : TEXCOORD1) : COLOR 
{ 
 float2 XYc = float2 (Xpos, 1.0 - Ypos);
 float2 xy1 = XYc - uv;

 float invZoom = 1.0 - ( 1.0 / Zoom );

 xy1 = invZoom * xy1 + uv;   

 return tex2D (PointSampler, xy1);
} 




//--------------------------------------------------------------
// Technique
//--------------------------------------------------------------
technique
{
   pass
   {
      PixelShader = compile PROFILE main();
   }
}
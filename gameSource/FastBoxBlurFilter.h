/**
 * Blur convolution filter that uses a box for averaging.
 *
 * Faster accumulative implementation, as suggested by Gamasutra.
 *
 * For speed, does NOT handle edge pixels correctly
 *
 * For even more speed, does not support multiple radii (only radius=1)
 *
 *
 * Also, changed to process uchar channels (instead of doubles) for speed
 *
 * @author Jason Rohrer 
 */
class FastBoxBlurFilter { 
	
	public:
		
		/**
		 * Constructs a box filter.
		 */
		FastBoxBlurFilter();
		
		// implements the ChannelFilter interface 
        // (but for uchars and sub-regions, and a subset of pixels in that
        //  region)
		void applySubRegion( unsigned char *inChannel,
                             int *inTouchPixelIndices,
                             int inNumTouchPixels,
                             int inWidth, int inHeight );

	};
	
	
	

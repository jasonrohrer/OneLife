#include "groundSprites.h"
#include "objectBank.h"

#include "../commonSource/fractalNoise.h"


#include "minorGems/graphics/filters/BoxBlurFilter.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"






// array sized for largest biome ID for direct indexing
// sparse, with NULL entries
int groundSpritesArraySize;
GroundSpriteSet **groundSprites;


static int nextStep;

static int blurRadius = 12;
static BoxBlurFilter blur( blurRadius );

static File groundDir( NULL, "ground" );
static File groundTileCacheDir( NULL, "groundTileCache" );

static SimpleVector<int> allBiomes;
    
static char printSteps = false;


int initGroundSpritesStart( char inPrintSteps ) {
    nextStep = 0;
    
    printSteps = inPrintSteps;

    getAllBiomes( &allBiomes );
    
    int maxBiome = -1;
    for( int i=0; i<allBiomes.size(); i++ ) {
        int b = allBiomes.getElementDirect( i );
        if( b > maxBiome ) {
            maxBiome = b;
            }
        }
    
    // extra space for unknown biome sprite
    groundSpritesArraySize = maxBiome + 2;
    groundSprites = new GroundSpriteSet*[ groundSpritesArraySize ];

    
    for( int i=0; i<groundSpritesArraySize; i++ ) {
        groundSprites[i] = NULL;
        }
    
    
    if( !groundTileCacheDir.exists() ) {
        groundTileCacheDir.makeDirectory();
        }

    // -1 to trigger loading of unknown biome image
    allBiomes.push_back( -1 );
    
    return allBiomes.size();
    }


// returns progress... ready for Finish when progress == 1.0
float initGroundSpritesStep() {
    
    if( nextStep < allBiomes.size() ) {
        int i = nextStep;

        int b = allBiomes.getElementDirect( i );
        
        int cacheFileNumber = b;
        
        char *fileName;
        
        char isUnknownBiome = false;

        if( b == -1 ) {
            // special case
            fileName = stringDuplicate( "ground_U.tga" );
            
            // stick it at end
            b = groundSpritesArraySize - 1;
        
            cacheFileNumber = 99999;
            
    
            isUnknownBiome = true;
            }
        else {
            fileName = autoSprintf( "ground_%d.tga", b );
            }
        

        File *groundFile = groundDir.getChildFile( fileName );
        

        char *fullFileName = groundFile->getFullFileName();
        
        RawRGBAImage *rawImage = NULL;
        
        if( groundFile->exists() ) {
            
            rawImage = readTGAFileRawBase( fullFileName );

            }
        
        delete groundFile;

        if( rawImage != NULL ) {
            
            int w = rawImage->mWidth;
            int h = rawImage->mHeight;
            
            if( w % CELL_D != 0 || h % CELL_D != 0 ) {
                printf( 
                    "Ground texture %s with w=%d and h=%d does not "
                    "have dimensions that are even multiples of the cell "
                    "width %d",
                    fileName, w, h, CELL_D );
                }
            else if( rawImage->mNumChannels != 4 ) {
                printf( 
                    "Ground texture %s has %d channels instead of 4",
                    fileName, rawImage->mNumChannels );
                }
            else {    
                groundSprites[b] = new GroundSpriteSet;
                groundSprites[b]->biome = b;
                
                if( isUnknownBiome ) {
                    groundSprites[b]->biome = -1;
                    }
                
                groundSprites[b]->numTilesWide = w / CELL_D;
                groundSprites[b]->numTilesHigh = h / CELL_D;
                
                int tW = groundSprites[b]->numTilesWide;
                int tH = groundSprites[b]->numTilesHigh;

                groundSprites[b]->tiles = new SpriteHandle*[tH];
                groundSprites[b]->squareTiles = new SpriteHandle*[tH];
                
                groundSprites[b]->wholeSheet = fillSprite( rawImage );

                // check if all cache files exist
                // if so, don't need to load double version of whole image
                char allCacheFilesExist = true;
                
                for( int ty=0; ty<tH; ty++ ) {
                    groundSprites[b]->tiles[ty] = new SpriteHandle[tW];
                    groundSprites[b]->squareTiles[ty] = new SpriteHandle[tW];
                    
                    for( int tx=0; tx<tW; tx++ ) {
                        
                        char *cacheFileName = 
                            autoSprintf( 
                                "groundTileCache/biome_%d_x%d_y%d.tga",
                                cacheFileNumber, tx, ty );

                        groundSprites[b]->tiles[ty][tx] = 
                            loadSpriteBase( cacheFileName, false );
                        
                        char *squareCacheFileName = 
                            autoSprintf( 
                                "groundTileCache/biome_%d_x%d_y%d_square.tga",
                                cacheFileNumber, tx, ty );

                        groundSprites[b]->squareTiles[ty][tx] = 
                            loadSpriteBase( squareCacheFileName, false );
                        
                        
                        if( groundSprites[b]->tiles[ty][tx] == NULL ) {
                            // cache miss
                            
                            allCacheFilesExist = false;
                            }
                        if( groundSprites[b]->squareTiles[ty][tx] == NULL ) {
                            // cache miss
                            
                            allCacheFilesExist = false;
                            }
                        delete [] cacheFileName;
                        delete [] squareCacheFileName;
                        }
                    }
                

                if( !allCacheFilesExist ) {
                    // need to regenerate some
                    
                    // spend time to load the double-converted image
                    Image *image = readTGAFileBase( fullFileName );

                    int tileD = CELL_D * 2;
                
                    for( int ty=0; ty<tH; ty++ ) {
                        
                        for( int tx=0; tx<tW; tx++ ) {
                        
                            if( groundSprites[b]->squareTiles[ty][tx] == 
                                NULL ) {
                                
                                // cache miss

                                char *cacheFileName = autoSprintf( 
                                  "groundTileCache/biome_%d_x%d_y%d_square.tga",
                                  cacheFileNumber, tx, ty );

                                if( printSteps ) {    
                                    printf( "Cache file %s does not exist, "
                                            "rebuilding.\n", cacheFileName );
                                    }
                                
                                        
                                Image *tileImage = image->getSubImage( 
                                    tx * CELL_D, ty * CELL_D, CELL_D, CELL_D );
                                
                                writeTGAFile( cacheFileName, tileImage );

                                delete [] cacheFileName;
                                
                                groundSprites[b]->squareTiles[ty][tx] = 
                                    fillSprite( tileImage, false );
                                
                                delete tileImage;
                                }
                            if( groundSprites[b]->tiles[ty][tx] == NULL ) {
                                // cache miss

                                char *cacheFileName = 
                                    autoSprintf( 
                                        "groundTileCache/biome_%d_x%d_y%d.tga",
                                        cacheFileNumber, tx, ty );

                                if( printSteps ) {    
                                    printf( "Cache file %s does not exist, "
                                            "rebuilding.\n", cacheFileName );
                                    }
                                
                                // generate from scratch

                                Image tileImage( tileD, tileD, 4, false );
                        
                                setXYRandomSeed( ty * 237 + tx );
                        
                                // first, copy from source image to 
                                // fill 2x tile
                                // centered on 1x tile of image, wrapping
                                // around in source image as needed
                                int imStartX = 
                                    tx * CELL_D - ( tileD - CELL_D ) / 2;
                                int imStartY = 
                                    ty * CELL_D - ( tileD - CELL_D ) / 2;

                                int imEndX = imStartX + tileD;
                                int imEndY = imStartY + tileD;
                                for( int c=0; c<3; c++ ) {
                                    double *chanSrc = image->getChannel( c );
                                    double *chanDest = 
                                        tileImage.getChannel( c );
                            
                                    int dY = 0;
                                    for( int y = imStartY; y<imEndY; y++ ) {
                                        int wrapY = y;
                                
                                        if( wrapY >= h ) {
                                            wrapY -= h;
                                            }
                                        else if( wrapY < 0 ) {
                                            wrapY += h;
                                            }
                                
                                        int dX = 0;
                                        for( int x = imStartX;  x<imEndX; 
                                             x++ ) {
                                            
                                            int wrapX = x;
                                    
                                            if( wrapX >= w ) {
                                                wrapX -= w;
                                                }
                                            else if( wrapX < 0 ) {
                                                wrapX += w;
                                                }
                                    
                                            chanDest[ dY * tileD + dX ] =
                                                chanSrc[ wrapY * w + wrapX ];
                                            dX++;
                                            }
                                        dY++;
                                        }
                                    }

                                // now set alpha based on radius

                                int cellR = CELL_D / 2;
                        
                                // radius to cornerof map tile
                                int cellCornerR = 
                                    (int)sqrt( 2 * cellR * cellR );

                                int tileR = tileD / 2;

                                                
                                // halfway between
                                int targetR = ( tileR + cellCornerR ) / 2;
                        

                                // better:
                                // grow out from min only
                                targetR = cellCornerR + 1;
                        
                                double wiggleScale = 0.95 * tileR - targetR;
                        
                        
                                double *tileAlpha = tileImage.getChannel( 3 );
                                for( int y=0; y<tileD; y++ ) {
                                    int deltY = y - tileD/2;
                            
                                    for( int x=0; x<tileD; x++ ) {    
                                        int deltX = x - tileD/2;
                                
                                        double r = 
                                            sqrt( deltY * deltY + 
                                                  deltX * deltX );
                                
                                        int p = y * tileD + x;
                                
                                        double wiggle = 
                                            getXYFractal( x, y, 0, .5 );
                                
                                        wiggle *= wiggleScale;
 
                                        if( r > targetR + wiggle ) {
                                            tileAlpha[p] = 0;
                                            }
                                        else {
                                            tileAlpha[p] = 1;
                                            }
                                        }
                                    }

                                // make sure square of cell plus blur
                                // radius is solid, so that corners
                                // are not undercut by blur
                                // this will make some weird square points
                                // sticking out, but they will be blurred
                                // anyway, so that's okay
                                
                                int edgeStartA = CELL_D - 
                                    ( CELL_D/2 + blurRadius );

                                int edgeStartB = CELL_D + 
                                    ( CELL_D/2 + blurRadius + 1 );
                                
                                for( int y=edgeStartA; y<=edgeStartB; y++ ) {
                                    for( int x=edgeStartA; 
                                         x<=edgeStartB; x++ ) {    
                                        
                                        int p = y * tileD + x;
                                        tileAlpha[p] = 1.0;
                                        }
                                    }
                                

                                // trimm off lower right edges
                                for( int y=0; y<tileD; y++ ) {
                                    
                                    for( int x=edgeStartB; x<tileD; x++ ) {    
                                        
                                        int p = y * tileD + x;
                                        tileAlpha[p] = 0;
                                        }
                                    }
                                for( int y=edgeStartB; y<tileD; y++ ) {
                                    
                                    for( int x=0; x<tileD; x++ ) {    
                                        
                                        int p = y * tileD + x;
                                        tileAlpha[p] = 0;
                                        }
                                    }
                        
                                tileImage.filter( &blur, 3 );

                                
                                // cache for next time
                                
                                
                                writeTGAFile( cacheFileName, &tileImage );

                                delete [] cacheFileName;
                                
                                // to test a single tile
                                //exit(0);
                                
                                groundSprites[b]->tiles[ty][tx] = 
                                    fillSprite( &tileImage, false );
                                }
                            }
                        }
                    delete image;
                    }
                }
            
            delete rawImage;
            }
        
        delete [] fullFileName;
            
        delete [] fileName;
        
        nextStep++;
        }
    
    return nextStep / (float)( allBiomes.size() );
    }



void initGroundSpritesFinish() {
    }



void freeGroundSprites() {
    for( int i=0; i<groundSpritesArraySize; i++ ) {
        if( groundSprites[i] != NULL ) {
            
            for( int y=0; y<groundSprites[i]->numTilesHigh; y++ ) {
                for( int x=0; x<groundSprites[i]->numTilesWide; x++ ) {
                    freeSprite( groundSprites[i]->tiles[y][x] );
                    freeSprite( groundSprites[i]->squareTiles[y][x] );
                    }
                delete [] groundSprites[i]->tiles[y];
                delete [] groundSprites[i]->squareTiles[y];
                }
            delete [] groundSprites[i]->tiles;
            delete [] groundSprites[i]->squareTiles;
            

            freeSprite( groundSprites[i]->wholeSheet );
            
            delete groundSprites[i];
            }
        }
    delete [] groundSprites;

    groundSprites = NULL;
    }


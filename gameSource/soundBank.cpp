
#include "soundBank.h"
#include "ogg.h"
#include "objectBank.h"
#include "animationBank.h"

#include <stdlib.h>
#include <math.h>

#include "minorGems/util/SettingsManager.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/game/game.h"

#include "minorGems/sound/formats/aiff.h"

#include "minorGems/system/Time.h"

#include "binFolderCache.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SoundRecord **idMap;


static int maxID;





typedef struct SoundLoadingRecord {
        int soundID;
        int asyncSoundLoadHandle;
        int asyncReverbLoadHandle;
        
    } SoundLoadingRecord;


static SimpleVector<SoundLoadingRecord> loadingSounds;

static SimpleVector<int> loadedSounds;

static char *loadingFailureFileName = NULL;



static int sampleRate = 44100;



static double playedSoundVolumeScale = 1.0;


void setVolumeScaling( int inMaxSimultaneousSoundEffects,
                       double inMusicHeadroom ) {
    double totalVolume = 1.0 - inMusicHeadroom;
    
    playedSoundVolumeScale = totalVolume;

    // compress top 50% of volume range so that those loud sounds have
    // the same volume if played individually, while giving full dynamic
    // range to quieter sounds played individually.
    setMaxTotalSoundSpriteVolume( totalVolume, 0.50 );
    
    // allow a reverb for each
    setMaxSimultaneousSoundSprites( 2 * inMaxSimultaneousSoundEffects );
    }


static double soundEffectsLoudness = 1.0;
static char soundEffectsOff = false;



void setSoundEffectsLoudness( double inLoudness ) {
    soundEffectsLoudness = inLoudness;
    
    SettingsManager::setSetting( "soundEffectsLoudness", 
                                 (float)soundEffectsLoudness );
    }



double getSoundEffectsLoudness() {
    return soundEffectsLoudness;
    }



void setSoundEffectsOff( char inOff ) {
    soundEffectsOff = inOff;
    
    SettingsManager::setSetting( "soundEffectsOff", soundEffectsOff );
    }




int getMaxSoundID() {
    return maxID;
    }



static int16_t *readAIFFFile( File *inFile, int *outNumSamples ) {
    int numBytes;
    unsigned char *data = 
        inFile->readFileContents( &numBytes );
            
    if( data != NULL ) { 
        int16_t *samples = readMono16AIFFData( data, numBytes, outNumSamples );
        
        delete [] data;
        
        return samples;
        }
    
    return NULL;
    }



static void writeAiffFile( File *inFile, int16_t *inSamples, 
                           int inNumSamples ) {
    int headerLength;
    unsigned char *header = 
        getAIFFHeader( 1, 16, sampleRate, inNumSamples, &headerLength );
        
    FileOutputStream stream( inFile );
    
    stream.write( header, headerLength );

    delete [] header;
        
    int numBytes = inNumSamples * 2;
    unsigned char *data = new unsigned char[ numBytes ];
        
    int b = 0;
    for( int s=0; s<inNumSamples; s++ ) {
        // big endian
        data[b] = inSamples[s] >> 8;
        data[b+1] = inSamples[s] & 0xFF;
        b+=2;
        }
        
    stream.write( data, numBytes );
    delete [] data;
    }





static char doesReverbCacheExist( int inID, File *inReverbFolder ) {
    char *cacheFileName = autoSprintf( "%d.aiff", inID );
    
    File *cacheFile = inReverbFolder->getChildFile( cacheFileName );
    
    
    char returnVal = false;
    
    if( cacheFile->exists() ) {
        returnVal = true;
        }
    delete [] cacheFileName;
    delete cacheFile;
    
    return returnVal;
    }




static void clearSoundCacheFile() {
    File soundsFolder( NULL, "sounds" );

    clearAllBinCacheFiles( &soundsFolder );
    }



static void clearReverbCacheFile() {
    File reverbFolder( NULL, "reverbCache" );

    clearAllBinCacheFiles( &reverbFolder );
    }




static void clearCacheFiles() {
    clearSoundCacheFile();
    clearReverbCacheFile();
    }



static char printSteps = false;



#include "convolution.h"


MultiConvolution reverbConvolution = { -1, -1, NULL };
MultiConvolution eqConvolution = { -1, -1, NULL };


static int16_t *generateWetConvolve( MultiConvolution inMulti, int inNumSamples,
                                     int16_t *inSamples, 
                                     int *outNumWetSamples ) {

    if( inMulti.savedNumSamplesB <= 0 ) {
        // no covolution impulse response loaded
        // can't convolve
        // just return copy of dry samples
        int numWetSamples = inNumSamples;
        
        int16_t *wet = new int16_t[ numWetSamples ];
        
        memcpy( wet, inSamples, numWetSamples * sizeof( int16_t ) );
        *outNumWetSamples = numWetSamples;
        
        return wet;
        }
    

    int numWetSamples = inNumSamples + inMulti.savedNumSamplesB;
            
    double *wetSampleFloats = new double[ numWetSamples ];
    
    for( int i=0; i<numWetSamples; i++ ) {
        wetSampleFloats[i] = 0;
        }
            
            

    double *sampleFloats = new double[ inNumSamples ];
    
    for( int i=0; i<inNumSamples; i++ ) {
        sampleFloats[i] = (double) inSamples[i] / 32768.0;
        }
    
    // b data has been pre-generated with startMultiConvolution
    multiConvolve( inMulti, sampleFloats, inNumSamples,
                   wetSampleFloats );

    delete [] sampleFloats;

    double maxWet = 0;
    double minWet = 0;
            
    for( int i=0; i<numWetSamples; i++ ) {
        if( wetSampleFloats[ i ] > maxWet ) {
            maxWet = wetSampleFloats[ i ];
            }
        else if( wetSampleFloats[ i ] < minWet ) {
            minWet = wetSampleFloats[ i ];
            }
        }
    double scale = maxWet;
    if( -minWet > scale ) {
        scale = -minWet;
        }
    double normalizeFactor = 1.0 / scale;
            
    int16_t *wetSamples = new int16_t[ numWetSamples ];
    for( int i=0; i<numWetSamples; i++ ) {
        wetSamples[i] = 
            (int16_t)( 
                lrint( 32767 * normalizeFactor * wetSampleFloats[i] ) );
        }
    delete [] wetSampleFloats;
    
    *outNumWetSamples = numWetSamples;
    return wetSamples;
    }

                                     



static void generateReverb( SoundRecord *inRecord,
                            File *inReverbFolder ) {
    
    char *cacheFileName = autoSprintf( "%d.aiff", inRecord->id );
    
    File *cacheFile = inReverbFolder->getChildFile( cacheFileName );
    
    
    if( ! cacheFile->exists() ) {
        
        if( printSteps ) {
            printf( "Regenerating reverb cache file %s\n", cacheFileName );
            }

        int numSamples;
        int16_t *samples = NULL;
        
        File soundFolder( NULL, "sounds" );
        
        if( soundFolder.exists() && soundFolder.isDirectory() ) {
            File *soundFile = soundFolder.getChildFile( cacheFileName );
            
            if( soundFile->exists() ) {
                samples = readAIFFFile( soundFile, &numSamples );
                }
            delete soundFile;
            }
        
        if( samples != NULL ) {
            
            int numWetSamples;
            
            int16_t *wetSamples = generateWetConvolve( reverbConvolution,
                                                       numSamples,
                                                       samples,
                                                       &numWetSamples );
            delete [] samples;

            writeAiffFile( cacheFile, wetSamples, numWetSamples );
            
            delete [] wetSamples;
            }
        else {
            printf( "Failed to read file from sounds folder %s\n", 
                    cacheFileName );
            }
        }
    delete cacheFile;
    
    delete [] cacheFileName;
    }


static SoundRecord *getSoundRecord( int inID ) {
    if( inID >= 0 && inID < mapSize ) {
        return idMap[inID];
        }
    else {
        return NULL;
        }
    }



static SimpleVector<int> reverbsToRegenerate;
static int nextReverbToRegenerate = 0;
static File *reverbFolder;

static int currentSoundFile = 0;
static int currentReverbFile = 0;


static BinFolderCache soundCache;
static BinFolderCache reverbCache;

SimpleVector<SoundRecord*> records;



int initSoundBankStart( char *outRebuildingCache ) {
    
    //printSteps = inPrintSteps;
    
    
    soundEffectsLoudness = 
        SettingsManager::getFloatSetting( "soundEffectsLoudness", 1.0 );
    
    soundEffectsOff = SettingsManager::getIntSetting( "soundEffectsOff", 0 );
    
    
    SettingsManager::setSetting( "soundEffectsOff", soundEffectsOff );


    sampleRate = SettingsManager::getIntSetting( "soundSampleRate",
                                                 44100 );
    maxID = 0;

    currentSoundFile = 0;
    currentReverbFile = 0;
    
    char rebuildingSounds, rebuildingReverbs;
    
    
    soundCache = initBinFolderCache( "sounds", 
                                     ".aiff|.ogg", &rebuildingSounds );
    
    reverbCache = 
        initBinFolderCache( "reverbCache", ".aiff", &rebuildingReverbs );

    
    File eqFile( NULL, "eqImpulseResponse.aiff" );
    
    if( eqFile.exists() ) {
        
        int numEqSamples = 0;
        int16_t *eqSamples = readAIFFFile( &eqFile, &numEqSamples );
            
        if( eqSamples != NULL ) {        
            double *eqFloats = new double[ numEqSamples ];
            
            for( int j=0; j<numEqSamples; j++ ) {
                eqFloats[j] = (double) eqSamples[j] / 32768.0;
                }
                
            eqConvolution = startMultiConvolution( eqFloats, numEqSamples );
                
            delete [] eqFloats;
            delete [] eqSamples;
            }
        }





    File reverbFile( NULL, "reverbImpulseResponse.aiff" );
    
    if( reverbFile.exists() ) {
    
        reverbFolder = new File( NULL, "reverbCache" );
        
        if( ! reverbFolder->exists() ) {
            reverbFolder->makeDirectory();
            }
    
        if( reverbFolder->exists() && reverbFolder->isDirectory() ) {
            
            int numReverbSamples = 0;
            int16_t *reverbSamples = readAIFFFile( &reverbFile,
                                                   &numReverbSamples );
            
            if( reverbSamples != NULL ) {        
                double *reverbFloats = new double[ numReverbSamples ];
            
                for( int j=0; j<numReverbSamples; j++ ) {
                    reverbFloats[j] = (double) reverbSamples[j] / 32768.0;
                    }
                
                reverbConvolution = 
                    startMultiConvolution( reverbFloats, numReverbSamples );
                
                delete [] reverbFloats;

                File soundFolder( NULL, "sounds" );
                
                int numFiles;
                File **childFiles = soundFolder.getChildFiles( &numFiles );
        
    
                for( int i=0; i<numFiles; i++ ) {
            
                    
                    char *fileName = childFiles[i]->getFileName();
                    delete childFiles[i];
                    
                    // skip all non-AIFF files 
                    // note that we do NOT generate reverbs for OGG files,
                    // since those tend to be longer (snippets of music)
                    // and aren't positioned in the environment in the same
                    // way as one-hit sounds
                    if( strstr( fileName, ".aiff" ) != NULL ) {
                        
                        int id = 0;
                        sscanf( fileName, "%d.aiff", &id );
                        
                
                        if( ! doesReverbCacheExist( id, reverbFolder ) ) {
                            reverbsToRegenerate.push_back( id );
                            }                    
                        
                        }
                    delete [] fileName;
                    }
                
                delete [] childFiles;
                
                
                delete [] reverbSamples;
                }
            
            }
        }
    
    *outRebuildingCache = 
        rebuildingSounds || rebuildingReverbs ||
        ( reverbsToRegenerate.size() > 0 );

    return reverbsToRegenerate.size() + 
        soundCache.numFiles + 
        reverbCache.numFiles;
    }



float initSoundBankStep() {
    
    if( currentSoundFile == soundCache.numFiles &&
        nextReverbToRegenerate == reverbsToRegenerate.size() &&
        currentReverbFile == reverbCache.numFiles ) {
        return 1.0f;
        }
    
    if( currentSoundFile < soundCache.numFiles ) {
        int i = currentSoundFile;
        
        char *fileName = getFileName( soundCache, i );
    
        // skip all non-AIFF, non-OGG files 
        if( strstr( fileName, ".aiff" ) != NULL ||
            strstr( fileName, ".ogg" ) != NULL ) {
            char added = false;
            
            SoundRecord *r = new SoundRecord;

            
            r->sound = NULL;
            r->reverbSound = NULL;
            
            r->loading = false;
            r->numStepsUnused = 0;
            r->liveUseageCount = 0;
            
            r->id = 0;
            
            sscanf( fileName, "%d.", &( r->id ) );
            
            int soundDataLength;
            unsigned char *soundData = 
                getFileContents( soundCache, i, 
                                 fileName, &soundDataLength );
            
            if( soundData != NULL ) {
                
                int numSamples;
                int16_t *samples;
                
                if( strstr( fileName, ".aiff" ) != NULL ) {
                    samples =
                        readMono16AIFFData( soundData, 
                                            soundDataLength, &numSamples );
                    }
                else if( strstr( fileName, ".ogg" ) != NULL ) {
                    OGGHandle o = openOGG( soundData, soundDataLength );

                    int numChan = getOGGChannels( o );
                    if( numChan == 1 ) {
                        numSamples = getOGGTotalSamples( o );
                        samples = new int16_t[ numSamples ];
                        
                        readAllMonoSamplesOGG( o, samples );
                        }
                    // skip non-mono OGG files
                    
                    closeOGG( o );
                    }
                

                if( samples != NULL ) {
                    
                    r->sound = setSoundSprite( samples, numSamples );
                    
                    
                    records.push_back( r );
                    added = true;
                    if( maxID < r->id ) {
                        maxID = r->id;
                        }

                    delete [] samples;                    
                    }
                delete [] soundData;
                }

            if( !added ) {
                delete r;
                }
            }
        
        delete [] fileName;
        
        currentSoundFile ++;

        if( currentSoundFile == soundCache.numFiles ) {
            // done loading all sounds
            mapSize = maxID + 1;
    
            idMap = new SoundRecord*[ mapSize ];
            
            for( int i=0; i<mapSize; i++ ) {
                idMap[i] = NULL;
                }
            
            int numRecords = records.size();
            for( int i=0; i<numRecords; i++ ) {
                SoundRecord *r = records.getElementDirect(i);
                
                idMap[ r->id ] = r;
                }

            printf( "Loaded %d sound IDs from sounds folder\n", numRecords );
            }
        }
    else if( nextReverbToRegenerate < reverbsToRegenerate.size() ) {

        int id = reverbsToRegenerate.getElementDirect( nextReverbToRegenerate );
    
        generateReverb( getSoundRecord( id ), reverbFolder );

        nextReverbToRegenerate++;
        
        if( nextReverbToRegenerate == reverbsToRegenerate.size() ) {
            // done regenning reverbs, and there were some

            // rebuild cache of reverb sounds from scratch            
            clearReverbCacheFile();
            freeBinFolderCache( reverbCache );
            
            char rebuilding;
            
            reverbCache = 
                initBinFolderCache( "reverbCache", ".aiff", &rebuilding );
            }
        }
    else if( currentReverbFile < reverbCache.numFiles ) {
        int i = currentReverbFile;
        
        char *fileName = getFileName( reverbCache, i );
    
        // skip all non-AIFF files 
        if( strstr( fileName, ".aiff" ) != NULL ) {

            int aiffDataLength;
            unsigned char *aiffData = 
                getFileContents( reverbCache, i, 
                                 fileName, &aiffDataLength );
            
            if( aiffData != NULL ) {
                int id = 0;
                
                sscanf( fileName, "%d.aiff", &( id ) );
                
                SoundRecord *r = getSoundRecord( id );
                
                if( r != NULL ) {

                    int numSamples;
                    int16_t *samples =
                        readMono16AIFFData( aiffData, aiffDataLength, 
                                            &numSamples );
                    
                    if( samples != NULL ) {
                        
                        r->reverbSound = setSoundSprite( samples, numSamples );
                        
                        delete [] samples;                    
                        }
                    }
                delete [] aiffData;
                }
            }
        
        delete [] fileName;
        
        currentReverbFile ++;

        if( currentReverbFile == reverbCache.numFiles ) {
            printf( "Loaded %d reverbs from reverbCache folder\n", 
                    currentReverbFile );
            }
        }

    
    return (float)( currentSoundFile + 
                    currentReverbFile + nextReverbToRegenerate ) / 
        (float)( soundCache.numFiles + reverbCache.numFiles + 
                 reverbsToRegenerate.size() );
    }
        


void initSoundBankFinish() {
    endMultiConvolution( &reverbConvolution );
    
    freeBinFolderCache( soundCache );
    freeBinFolderCache( reverbCache );

    delete reverbFolder;
    }







static void loadSound( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r != NULL ) {
        
        if( r->sound == NULL && ! r->loading ) {
                
            File soundsDir( NULL, "sounds" );
            File reverbDir( NULL, "reverbCache" );
            

            const char *printFormatAIFF = "%d.aiff";
        
            char *fileNameAIFF = autoSprintf( printFormatAIFF, inID );
        

            File *soundFile = soundsDir.getChildFile( fileNameAIFF );
            File *reverbFile = reverbDir.getChildFile( fileNameAIFF );
            
            delete [] fileNameAIFF;
            

            char *fullSoundName = soundFile->getFullFileName();
            char *fullReverbName = reverbFile->getFullFileName();
        
            delete soundFile;
            delete reverbFile;
            

            SoundLoadingRecord loadingR;
            
            loadingR.soundID = inID;

            loadingR.asyncSoundLoadHandle = startAsyncFileRead( fullSoundName );
            loadingR.asyncReverbLoadHandle = 
                startAsyncFileRead( fullReverbName );
            
            delete [] fullSoundName;
            delete [] fullReverbName;

            loadingSounds.push_back( loadingR );
            
            r->loading = true;
            }

        }
    }



static void freeSoundRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            if( idMap[inID]->sound != NULL || 
                idMap[inID]->reverbSound != NULL) {                
                
                if( idMap[inID]->sound != NULL ) {
                    freeSoundSprite( idMap[inID]->sound );
                    }
                if( idMap[inID]->reverbSound != NULL ) {
                    freeSoundSprite( idMap[inID]->reverbSound );
                    }
                

                for( int i=0; i<loadedSounds.size(); i++ ) {
                    int id = loadedSounds.getElementDirect( i );
                    
                    if( id == inID ) {
                        loadedSounds.deleteElement( i );
                        break;
                        }
                    }
                }
            
            delete idMap[inID];
            idMap[inID] = NULL;
            
            return;
            }
        }
    }




void freeSoundBank() {

    if( loadingFailureFileName != NULL ) {
        delete [] loadingFailureFileName;
        }

    endMultiConvolution( &eqConvolution );

    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            if( idMap[i]->sound != NULL ) {    
                freeSoundSprite( idMap[i]->sound );
                }
            if( idMap[i]->reverbSound != NULL ) {    
                freeSoundSprite( idMap[i]->reverbSound );
                }

            delete idMap[i];
            }
        }

    delete [] idMap;
    }



static void setLoadingFailureFileName( char *inNewFileName ) {
    if( loadingFailureFileName != NULL ) {
        delete [] loadingFailureFileName;
        }
    loadingFailureFileName = inNewFileName;
    }



char *getSoundBankLoadFailure() {
    return loadingFailureFileName;
    }



void stepSoundBank() {
    // no more dynamic loading or unloading
    // they are all loaded at startup
    return;
    
    // keep this code in case we ever want to go back...

    for( int i=0; i<loadingSounds.size(); i++ ) {
        SoundLoadingRecord *loadingR = loadingSounds.getElement( i );
        
        if( checkAsyncFileReadDone( loadingR->asyncSoundLoadHandle ) &&
            checkAsyncFileReadDone( loadingR->asyncReverbLoadHandle ) ) {
            
            int lengthSound;
            unsigned char *dataSound = getAsyncFileData( 
                loadingR->asyncSoundLoadHandle, &lengthSound );

            int lengthReverb;
            unsigned char *dataReverb = getAsyncFileData( 
                loadingR->asyncReverbLoadHandle, &lengthReverb );
            
            SoundRecord *r = getSoundRecord( loadingR->soundID );
            
            
            if( dataSound == NULL ) {
                printf( "Reading sound data from file failed, sound ID %d\n",
                        loadingR->soundID );
                
                setLoadingFailureFileName(
                        autoSprintf( "sounds/%d.aiff", loadingR->soundID ) );
                }
            else if( dataReverb == NULL ) {
                printf( "Reading reverb data from cache failed, sound ID %d\n",
                        loadingR->soundID );
                
                setLoadingFailureFileName(
                    autoSprintf( "reverbCache/%d.aiff", 
                                     loadingR->soundID ) );
                }
            else {
                
                int numSamples;
                int16_t *samples =
                    readMono16AIFFData( dataSound, lengthSound, &numSamples );
                

                if( samples != NULL ) {
                    
                    r->sound = setSoundSprite( samples, numSamples );
                            
                    delete [] samples;
                    }
                
                samples =
                    readMono16AIFFData( dataReverb, lengthReverb, &numSamples );
                

                if( samples != NULL ) {
                    
                    r->reverbSound = setSoundSprite( samples, numSamples );
                            
                    delete [] samples;
                    }
                }

            if( dataSound != NULL ) {
                delete [] dataSound;
                }
            if( dataReverb != NULL ) {
                delete [] dataReverb;
                }
            
            
            r->numStepsUnused = 0;
            loadedSounds.push_back( loadingR->soundID );

            loadingSounds.deleteElement( i );
            i++;
            }
        }
    

    for( int i=0; i<loadedSounds.size(); i++ ) {
        int id = loadedSounds.getElementDirect( i );
    
        SoundRecord *r = getSoundRecord( id );
        
        if( r->sound != NULL || r->reverbSound != NULL ) {

            r->numStepsUnused ++;

            if( r->numStepsUnused > 600 ) {
                // 10 seconds not played

                if( r->sound != NULL ) {
                    freeSoundSprite( r->sound );
                    r->sound = NULL;
                    }
                if( r->reverbSound != NULL ) {
                    freeSoundSprite( r->reverbSound );
                    r->reverbSound = NULL;
                    }
                }
            
            r->loading = false;

            loadedSounds.deleteElement( i );
            i--;
            }
        }
    }



static char reverbDisabled = false;

void disableReverb( char inDisable ) {
    reverbDisabled = inDisable;
    }




void playSound( int inID, double inVolumeTweak, double inStereoPosition,
                double inReverbMix ) {
    if( soundEffectsOff ) {
        return;
        }
    
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            if( idMap[inID]->sound == NULL ) {
                loadSound( inID );
                return;
                }

            idMap[inID]->numStepsUnused = 0;
            
            if( reverbDisabled || idMap[inID]->reverbSound == NULL ) {
                // play just sound, ignore mix param    
                playSoundSprite( idMap[inID]->sound,
                                 soundEffectsLoudness * 
                                 inVolumeTweak * playedSoundVolumeScale, 
                                 inStereoPosition );
                }
            else {
                //  play both simultaneously
                double volume = 
                    soundEffectsLoudness * 
                    inVolumeTweak * playedSoundVolumeScale;

                SoundSpriteHandle handles[] = { idMap[inID]->sound,
                                                idMap[inID]->reverbSound };
                double volumes[] = { volume * ( 1-inReverbMix),
                                     volume };
                
                double stereo[] = { inStereoPosition, inStereoPosition };
                
                playSoundSprite( 2, handles, volumes, stereo );
                }
            
            markSoundLive( inID );
            }
        }
    }


void playSound( SoundUsage inUsage,
                double inStereoPosition, double inReverbMix ) {
    
    SoundUsagePlay p = playRandom( inUsage );
    
    playSound( p.id, p.volume, inStereoPosition, inReverbMix );
    }



double maxAudibleDistance = 16;

double minFadeStartDistance = 1.5;

double maxAudibleDistanceSquared = maxAudibleDistance * maxAudibleDistance;



// sigmoid functions
// h(x) = .5 + .5 *(-(x-3.5) / (1 + abs(x-3.5)))

// stretch it to span between 1 and 0
// f(x) = ( 1/( h(0) - h(16) ) )*(h(x) - h(16))

static double sigmoidH( double inDstance ) {
    return 0.5 + 0.5 * ( 
        -( inDstance - (2 + minFadeStartDistance) ) /
        ( 1 + fabs( inDstance - (2 + minFadeStartDistance) ) ) );
    }

static double sigmoidF( double inDstance ) {
    return ( 1 / ( sigmoidH( 0 ) - sigmoidH( maxAudibleDistance ) ) ) *
        ( sigmoidH( inDstance ) - sigmoidH( maxAudibleDistance ) );
    }



// returns true if sound should play
static char getVolumeAndPan( doublePair inVectorFromCameraToSoundSource,
                             double *outVolume, double *outPan ) {
        
    double d = length( inVectorFromCameraToSoundSource );
    

    // 1/dist^2
    //double volumeScale = ( 1.0 / ( d * d ) ) - 
    //    ( 1 / maxAudibleDistanceSquared );
    
    // 1/dist (linear)
    double volumeScale = 1;

    
    // everything at center screen same volume
    // doesn't start fading until edge of screen
    //if( d > minFadeStartDistance ) {
    //    volumeScale = ( 1.0 / ( d  - minFadeStartDistance ) ) - 
    //        ( 1.0 / ( maxAudibleDistance - minFadeStartDistance ) );
    //    }

    // the above curve does what we want, but has discontinuities in slope
    // which are audible
    volumeScale = sigmoidF( d );
    
    if( volumeScale <= 0 ) {
        // don't play sound at all
        return false;
        }
    if( volumeScale > 1 ) {
        volumeScale = 1;
        }
        
    // pan is based on X position on screen only
    double xPan = inVectorFromCameraToSoundSource.x;
    
    // stop at edges of screen
    if( xPan > 5 ) {
        xPan = 5;
        }
    if( xPan < -5 ) {
        xPan = -5;
        }
    
    // between 0 and 10
    xPan += 5;
    
    // between 3 and 13
    xPan += 3;


    *outVolume = volumeScale;
    *outPan = xPan / 16.0;

    return true;
    }



void playSound( SoundUsage inUsage,
                doublePair inVectorFromCameraToSoundSource ) {

    double volume, pan;
    
    char shouldPlay = 
        getVolumeAndPan( inVectorFromCameraToSoundSource, &volume, &pan );

    if( ! shouldPlay ) {
        // skip sound
        return;
        }

    // if we treat it as going from 0 to 16, we avoid hard left and right stereo
    // even at the edge of the screen
    
    //printf( "Pan = %f\n", xPan / 16.0 );

    // reverb increases over whole range
    //double reverbMix = d / maxAudibleDistance;

    // reverb increases to max by half range
    //double reverbMix = d / ( maxAudibleDistance / 2 );
    
    // reverb matches volume scaling exactly
    // always a bit of reverb
    double reverbContstant = 0.1;
    
    double reverbMix = 
        ( 1.0 - reverbContstant ) * ( 1.0 - volume ) + 
        reverbContstant;

    SoundUsagePlay p = playRandom( inUsage );

    playSound( p.id, volume * p.volume, pan,
               reverbMix );
    }



void playSound( SoundSpriteHandle inSoundSprite,
                double inVolumeTweak,
                doublePair inVectorFromCameraToSoundSource ) {
    double volume, pan;

    char shouldPlay = 
        getVolumeAndPan( inVectorFromCameraToSoundSource, &volume, &pan );

    
    if( ! shouldPlay ) {
        // skip sound
        return;
        }

    playSoundSprite( inSoundSprite,
                     inVolumeTweak * soundEffectsLoudness * 
                     volume * playedSoundVolumeScale, 
                     pan );
    }


    
char markSoundLive( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r == NULL ) {
        // if sound record doesn't exist, sound will never load
        // count sound as live so that parts of code that are waiting
        // for it to load can move on
        return true;
        }

    r->numStepsUnused = 0;
    
    if( r->sound == NULL && ! r->loading ) {
        loadSound( inID );
        return false;
        }
    

    if( r->sound != NULL ) {
        return true;
        }
    else {
        return false;
        }
    }



char markSoundUsageLive( SoundUsage inUsage ) {
    char allLive = true;
    
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        allLive = allLive && markSoundLive( inUsage.ids[i] );
        }
    return allLive;
    }




void deleteSoundFromBank( int inID ) {
    File soundsDir( NULL, "sounds" );
    File soundsRawDir( NULL, "soundsRaw" );

    for( int i=0; i<loadingSounds.size(); i++ ) {
        SoundLoadingRecord *loadingR = loadingSounds.getElement( i );
    
        if( loadingR->soundID == inID ) {
            // block deletion of sound that hasn't loaded yet

            // this is a rare case of a user's clicks beating the disk
            // but we still need to prevent a crash here.
            return;
            }
        }

    
    
    if( soundsDir.exists() && soundsDir.isDirectory() ) {    

        const char *printFormatAIFF = "%d.aiff";

        char *fileNameAIFF = autoSprintf( printFormatAIFF, inID );
        File *soundFileAIFF = soundsDir.getChildFile( fileNameAIFF );
        
        soundFileAIFF->remove();
        delete soundFileAIFF;
    
        if( soundsRawDir.exists() && soundsRawDir.isDirectory() ) {    
            File *soundRawFileAIFF = soundsRawDir.getChildFile( fileNameAIFF );
            
            soundRawFileAIFF->remove();
            delete soundRawFileAIFF;
            }

        delete [] fileNameAIFF;


        loadedSounds.deleteElementEqualTo( inID );

        
        File reverbFolder( NULL, "reverbCache" );
        
        char *cacheFileName = autoSprintf( "%d.aiff", inID );
    
        File *cacheFile = reverbFolder.getChildFile( cacheFileName );
    
        delete [] cacheFileName;
        
        if( cacheFile->exists() ) {
            cacheFile->remove();
            }

        delete cacheFile;
        }
    
    
    freeSoundRecord( inID );
    }




char startRecordingSound() {
    return startRecording16BitMonoSound( sampleRate );
    }







int stopRecordingSound() {

    int numSamples = 0;
    int16_t *samples = stopRecording16BitMonoSound( &numSamples );

    if( samples == NULL ) {
        return -1;
        }
    
    // normalize it
    int16_t maxAmp = 0;
    
    for( int i=0; i<numSamples; i++ ) {
        int16_t amp = samples[i];
        if( amp < 0 ) {
            amp = -amp;
            }
        if( amp > maxAmp ) {
            maxAmp = amp;
            }
        }
    
    double scale = 32767.0 / (double) maxAmp;
    
    for( int i=0; i<numSamples; i++ ) {
        samples[i] = lrint( scale * samples[i] );
        }
    


    File untrimmedFile( NULL, "untrimmed.aiff" );
    
    writeAiffFile( &untrimmedFile, samples, numSamples );
    
    
    // trim start/end of sound up to first occurrence of 10% amplitude 
    int16_t trimUpToMin = 10 * 327;

    int trimStartPoint =  0;
    int trimEndPoint = numSamples - 1;
    
    if( numSamples > sampleRate ) {
        // have at least a second to work with
        // ignore first and last quarter second (mouse clicks)
        trimEndPoint += sampleRate / 4;
        trimEndPoint -= sampleRate / 4;
        }
    

    for( int i=0; i<numSamples; i++ ) {
        if( samples[i] >= trimUpToMin || samples[i] <= -trimUpToMin ) {
            trimStartPoint = i;
            break;
            }
        }

    for( int i=numSamples-1; i>=0; i-- ) {
        if( samples[i] >= trimUpToMin || samples[i] <= -trimUpToMin ) {
            trimEndPoint = i;
            break;
            }
        }
    
    
    // 1 ms
    int fadeLengthStart = 1  * sampleRate / 1000;
    int fadeLengthEnd = 150  * sampleRate / 1000;
    
    if( trimStartPoint < fadeLengthStart ) {
        trimStartPoint = fadeLengthStart;
        }
    if( trimEndPoint + fadeLengthEnd >= numSamples ) {
        trimEndPoint = ( numSamples - fadeLengthEnd ) - 1;
        }
    
    int finalStartPoint = trimStartPoint - fadeLengthStart;
    int finalEndPoint = trimEndPoint + fadeLengthEnd;
    
    // apply linear fade
    for( int i=finalStartPoint; i<trimStartPoint; i++ ) {
        
        double fade = ( i - finalStartPoint ) / (double)fadeLengthStart;
    
        samples[i] = (int16_t)( lrint( fade * samples[i] ) );
        }
    for( int i=trimEndPoint+1; i<=finalEndPoint; i++ ) {
        
        double fade = ( finalEndPoint - i ) / (double)fadeLengthEnd;
    
        samples[i] = (int16_t)( lrint( fade * samples[i] ) );
        }
    
    int finalNumSamples = 1 + finalEndPoint - finalStartPoint;
    
    
    int newID = -1;
    
    // add it to file structure
    File soundsDir( NULL, "sounds" );
    File soundsRawDir( NULL, "soundsRaw" );
            
    if( !soundsDir.exists() ) {
        soundsDir.makeDirectory();
        }

    if( !soundsRawDir.exists() ) {
        soundsRawDir.makeDirectory();
        }
    
    if( soundsDir.exists() && soundsDir.isDirectory() &&
        soundsRawDir.exists() && soundsRawDir.isDirectory() ) {
                
                
        int nextSoundNumber = 1;
   
        File *nextNumberFile = 
            soundsDir.getChildFile( "nextSoundNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextSoundNumber );
                
                delete [] nextNumberString;
                }
            }

                
        const char *printFormatAIFF = "%d.aiff";
        
        char *fileNameAIFF = autoSprintf( printFormatAIFF, nextSoundNumber );
            
        newID = nextSoundNumber;

        File *rawSoundFile = soundsRawDir.getChildFile( fileNameAIFF );
        
        

        writeAiffFile( rawSoundFile, &( samples[ finalStartPoint ] ),
                       finalNumSamples );
        
        delete rawSoundFile;


        // now apply EQ
        
        int numWet = 0;

        int16_t *wetSamples = 
            generateWetConvolve( eqConvolution, finalNumSamples,
                                 &( samples[ finalStartPoint ] ), 
                                 &numWet );
        
        File *eqSoundFile = soundsDir.getChildFile( fileNameAIFF );

        writeAiffFile( eqSoundFile, wetSamples, numWet );
        
        delete eqSoundFile;

        delete [] fileNameAIFF;
        
        
        delete [] samples;
        
        samples = wetSamples;
        finalStartPoint = 0;
        finalNumSamples = numWet;

            

        
        nextSoundNumber++;
                
        char *nextNumberString = autoSprintf( "%d", nextSoundNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
        
        delete nextNumberFile;

        clearCacheFiles();
        }
    
    if( newID == -1 ) {
        // failed to save it to disk
        delete [] samples;
        return -1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        

        
        SoundRecord **newMap = new SoundRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(SoundRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }


    if( newID > maxID ) {
        maxID = newID;
        }
    
    SoundRecord *r = new SoundRecord;
    
    r->liveUseageCount = 0;
    
    r->id = newID;
    r->sound = setSoundSprite( &( samples[ finalStartPoint ] ),
                               finalNumSamples );
    r->reverbSound = NULL;
    
    delete [] samples;
    
    idMap[newID] = r;
    

    loadedSounds.push_back( r->id );

    r->loading = false;
    r->numStepsUnused = 0;

    return newID;
    }




void countLiveUse( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r != NULL ) {
        r->liveUseageCount ++;
        }
    }



void unCountLiveUse( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r != NULL ) {
        r->liveUseageCount --;

        if( r->liveUseageCount <= 0 ) {
            r->liveUseageCount = 0;
            checkIfSoundStillNeeded( inID );
            }
        }
    }



void countLiveUse( SoundUsage inUsage ) {
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        countLiveUse( inUsage.ids[i] );
        }
    }



void unCountLiveUse( SoundUsage inUsage ) {
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        unCountLiveUse( inUsage.ids[i] );
        }
    }





void checkIfSoundStillNeeded( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r != NULL && r->liveUseageCount == 0 ) {
        
        // ask animation bank and object bank if sound used
     
        if( ! isSoundUsedByObject( inID ) && ! isSoundUsedByAnim( inID ) ) {
            // orphaned, delete it
            deleteSoundFromBank( inID );
            }
        }
    
    }



void printOrphanedSoundReport() {
    int num = 0;
    
    for( int i=0; i<mapSize; i++ ) {
        if( i < mapSize ) {
            if( idMap[i] != NULL ) {
                if( ! isSoundUsedByObject( i ) && 
                    ! isSoundUsedByAnim( i ) ) {
                    
                    printf( "Sound %d orphaned\n", i );
                    num++;
                    }
                }    
            }
        }
    printf( "%d sounds found orphaned\n", num );
    }



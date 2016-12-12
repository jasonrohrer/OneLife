
#include "soundBank.h"
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


static int sampleRate = 44100;



static double playedSoundVolumeScale = 1.0;


void setVolumeScaling( int inMaxSimultaneousSoundEffects,
                       double inMusicHeadroom ) {
    double totalVolume = 1.0 - inMusicHeadroom;
    
    playedSoundVolumeScale = totalVolume / inMaxSimultaneousSoundEffects;

    setMaxTotalSoundSpriteVolume( totalVolume );
    }


static double soundEffectsLoudness = 1.0;
static char soundEffectsOff = false;



void setSoundEffectsLoudness( double inLoudness ) {
    soundEffectsLoudness = inLoudness;
    
    SettingsManager::setSetting( "soundEffectsLoudness", 
                                 (float)soundEffectsLoudness );
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



static void generateReverb( SoundRecord *inRecord,
                            int16_t *inReverbSamples, 
                            int inNumReverbSamples,
                            File *inReverbFolder ) {
    
    char *cacheFileName = autoSprintf( "%d.aiff", inRecord->id );
    
    File *cacheFile = inReverbFolder->getChildFile( cacheFileName );
    
    
    if( ! cacheFile->exists() ) {
         
        printf( "Regenerating reverb cache file %s\n", cacheFileName );
        

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
            
            int numWetSamples = numSamples + inNumReverbSamples;
            
            double *wetSampleFloats = new double[ numWetSamples ];
            
            for( int i=0; i<numWetSamples; i++ ) {
                wetSampleFloats[i] = 0;
                }
            
            double *reverbFloats = new double[ inNumReverbSamples ];
            
            for( int j=0; j<inNumReverbSamples; j++ ) {
                reverbFloats[j] = (double) inReverbSamples[j] / 32768.0;
                }

            for( int i=0; i<numSamples; i++ ) {
                double sampleFloat = (double)( samples[i] / 32768.0);
                
                for( int j=0; j<inNumReverbSamples; j++ ) {
                    wetSampleFloats[ i + j ] +=
                        sampleFloat * reverbFloats[j];
                    }
               
                }
            delete [] reverbFloats;
            
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



void initSoundBank() {
    
    soundEffectsLoudness = 
        SettingsManager::getFloatSetting( "soundEffectsLoudness", 1.0 );
    
    soundEffectsOff = SettingsManager::getIntSetting( "soundEffectsOff", 0 );
    
    
    SettingsManager::setSetting( "soundEffectsOff", soundEffectsOff );


    sampleRate = SettingsManager::getIntSetting( "soundSampleRate",
                                                 44100 );
    maxID = 0;

    File soundFolder( NULL, "sounds" );
    
    SimpleVector<SoundRecord*> records;

    if( soundFolder.exists() && soundFolder.isDirectory() ) {
        
        int numFiles = 0;
        
        File **childFiles = soundFolder.getChildFiles( &numFiles );
        
    
        for( int i=0; i<numFiles; i++ ) {
            

            char *fileName = childFiles[i]->getFileName();
    
            // skip all non-AIFF files 
            if( strstr( fileName, ".aiff" ) != NULL ) {
                            
                SoundRecord *r = new SoundRecord;


                r->sound = NULL;
                r->loading = false;
                r->numStepsUnused = 0;
                r->liveUseageCount = 0;
                
                r->id = 0;
        
                sscanf( fileName, "%d.txt", &( r->id ) );
                

                records.push_back( r );

                if( maxID < r->id ) {
                    maxID = r->id;
                    }
                }
            
            delete [] fileName;
            delete childFiles[i];
            }
        delete [] childFiles;
        }
    
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



    File reverbFile( NULL, "reverbImpulseResponse.aiff" );
    
    if( reverbFile.exists() ) {
    
        File reverbFolder( NULL, "reverbCache" );
        
        if( ! reverbFolder.exists() ) {
            reverbFolder.makeDirectory();
            }
    
        if( reverbFolder.exists() && reverbFolder.isDirectory() ) {
            
            int numReverbSamples;
            int16_t *reverbSamples = readAIFFFile( &reverbFile,
                                                   &numReverbSamples );
            
            if( reverbSamples != NULL ) {        
                
                for( int i=0; i<numRecords; i++ ) {
                    SoundRecord *r = records.getElementDirect(i);
                    
                    generateReverb( r, reverbSamples, numReverbSamples,
                                    &reverbFolder );
                    }

                delete [] reverbSamples;
                }
            
            }
        }
    
    }



static SoundRecord *getSoundRecord( int inID ) {
    if( inID >= 0 && inID < mapSize ) {
        return idMap[inID];
        }
    else {
        return NULL;
        }
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



void stepSoundBank() {
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
                }
            else if( dataReverb == NULL ) {
                printf( "Reading reverb data from cache failed, sound ID %d\n",
                        loadingR->soundID );
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
            
            if( idMap[inID]->reverbSound == NULL ) {
                // play just sound, ignore mix param    
                playSoundSprite( idMap[inID]->sound,
                                 soundEffectsLoudness * 
                                 inVolumeTweak * playedSoundVolumeScale, 
                                 inStereoPosition );
                }
            else {
                //  play both simultaneously
                double volume = inVolumeTweak * playedSoundVolumeScale;

                SoundSpriteHandle handles[] = { idMap[inID]->sound,
                                                idMap[inID]->reverbSound };
                double volumes[] = { volume * ( 1 - inReverbMix ),
                                     volume * inReverbMix };
                
                double stereo[] = { inStereoPosition, inStereoPosition };
                
                playSoundSprite( 2, handles, volumes, stereo );
                }
            
            markSoundLive( inID );
            }
        }
    }


void playSound( SoundUsage inUsage,
                double inStereoPosition ) {
    playSound( inUsage.id, inUsage.volume, inStereoPosition );
    }



double maxAudibleDistance = 16;

double minFadeStartDistance = 1.5;

double maxAudibleDistanceSquared = maxAudibleDistance * maxAudibleDistance;

void playSound( SoundUsage inUsage,
                doublePair inVectorFromCameraToSoundSource ) {


    // FIXME:  reverb, eventually

    
    double d = length( inVectorFromCameraToSoundSource );
    

    // 1/dist^2
    //double volumeScale = ( 1.0 / ( d * d ) ) - 
    //    ( 1 / maxAudibleDistanceSquared );
    
    // 1/dist (linear)
    double volumeScale = 1;

    
    // everything at center screen same volume
    // doesn't start fading until edge of screen
    if( d > minFadeStartDistance ) {
        volumeScale = ( 1.0 / ( d  - minFadeStartDistance ) ) - 
            ( 1.0 / ( maxAudibleDistance - minFadeStartDistance ) );
        }
    
    if( volumeScale < 0 ) {
        // don't play sound at all
        return;
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

    // if we treat it as going from 0 to 16, we avoid hard left and right stereo
    // even at the edge of the screen
    
    //printf( "Pan = %f\n", xPan / 16.0 );

    // reverb increases over whole range
    //double reverbMix = d / maxAudibleDistance;

    // reverb increases to max by half range
    double reverbMix = d / ( maxAudibleDistance / 2 );
    
    playSound( inUsage.id, volumeScale * inUsage.volume, xPan / 16.0,
               reverbMix );
    }



    
char markSoundLive( int inID ) {
    SoundRecord *r = getSoundRecord( inID );
    
    if( r == NULL ) {
        return false;
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




void deleteSoundFromBank( int inID ) {
    File soundsDir( NULL, "sounds" );

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
        
        loadedSounds.deleteElementEqualTo( inID );
        
        soundFileAIFF->remove();
            
        delete [] fileNameAIFF;
        delete soundFileAIFF;
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
            
    if( !soundsDir.exists() ) {
        soundsDir.makeDirectory();
        }
    
    if( soundsDir.exists() && soundsDir.isDirectory() ) {
                
                
        int nextSoundNumber = maxID + 1;
                
        const char *printFormatAIFF = "%d.aiff";
        
        char *fileNameAIFF = autoSprintf( printFormatAIFF, nextSoundNumber );
            
        newID = nextSoundNumber;

        File *soundFile = soundsDir.getChildFile( fileNameAIFF );
        
        delete [] fileNameAIFF;

        writeAiffFile( soundFile, &( samples[ finalStartPoint ] ),
                       finalNumSamples );
        
        delete soundFile;
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



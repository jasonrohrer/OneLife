
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
        int asyncLoadHandle;
        
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




int getMaxSoundID() {
    return maxID;
    }



void initSoundBank() {

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
            

            const char *printFormatAIFF = "%d.aiff";
        
            char *fileNameAIFF = autoSprintf( printFormatAIFF, inID );
        

            File *soundFile = soundsDir.getChildFile( fileNameAIFF );
            
            delete [] fileNameAIFF;
            

            char *fullName = soundFile->getFullFileName();
        
            delete soundFile;
            

            SoundLoadingRecord loadingR;
            
            loadingR.soundID = inID;

            loadingR.asyncLoadHandle = startAsyncFileRead( fullName );
            
            delete [] fullName;

            loadingSounds.push_back( loadingR );
            
            r->loading = true;
            }

        }
    }



static void freeSoundRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            if( idMap[inID]->sound != NULL ) {    
                freeSoundSprite( idMap[inID]->sound );
                
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

            delete idMap[i];
            }
        }

    delete [] idMap;
    }



void stepSoundBank() {
    for( int i=0; i<loadingSounds.size(); i++ ) {
        SoundLoadingRecord *loadingR = loadingSounds.getElement( i );
        
        if( checkAsyncFileReadDone( loadingR->asyncLoadHandle ) ) {
            
            int length;
            unsigned char *data = getAsyncFileData( loadingR->asyncLoadHandle, 
                                                    &length );
            SoundRecord *r = getSoundRecord( loadingR->soundID );

            
            if( data == NULL ) {
                printf( "Reading sound data from file failed, sound ID %d\n",
                        loadingR->soundID );
                }
            else {
                


                // FIXME:  AIFF read here

                if( length < 34 ) {
                    printf( "AIFF not long enough for header, sound ID %d\n",
                            loadingR->soundID );
                    }
                else {
                    

                    // byte 20 and 21 are num channels

                    if( data[20] != 0 || data[21] != 1 ) {
                        printf( "AIFF not mono, sound ID %d\n",
                                loadingR->soundID );
                        }
                    else if( data[26] != 0 || data[27] != 16 ) {
                        printf( "AIFF not 16-bit, sound ID %d\n",
                                loadingR->soundID );
                        }
                    else {
                        int numSamples =
                            data[22] << 24 |
                            data[23] << 16 |
                            data[24] << 8 |
                            data[25];

                        int sampleStartByte = 54;
                        

                        int numBytes = numSamples * 2;
                        
                        if( length < sampleStartByte + numBytes ) {
                            printf( "AIFF not long enough for data, "
                                    "sound ID %d\n",
                                    loadingR->soundID );
                            }
                        else {
                            
                            int16_t *samples = new int16_t[numSamples];
                            

                            int b = sampleStartByte;
                            for( int i=0; i<numSamples; i++ ) {
                                samples[i] = 
                                    ( data[b] << 8 ) |
                                    data[b+1];
                                b += 2;
                                }

                            r->sound = setSoundSprite( samples, numSamples );
                            }
                        }
                    }
                

                delete [] data;
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
        
        r->numStepsUnused ++;

        if( r->numStepsUnused > 600 ) {
            // 10 seconds not played
            
            freeSoundSprite( r->sound );
            r->sound = NULL;

            r->loading = false;

            loadedSounds.deleteElement( i );
            i--;
            }
        }
    }







void playSound( int inID, double inVolumeTweak, double inStereoPosition  ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            if( idMap[inID]->sound == NULL ) {
                loadSound( inID );
                return;
                }
            
            idMap[inID]->numStepsUnused = 0;
            playSoundSprite( idMap[inID]->sound, 
                             inVolumeTweak * playedSoundVolumeScale, 
                             inStereoPosition );
            
            markSoundLive( inID );
            }
        }
    }


void playSound( SoundUsage inUsage,
                double inStereoPosition ) {
    playSound( inUsage.id, inUsage.volume, inStereoPosition );
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



#ifndef EMOTION_INCLUDED
#define EMOTION_INCLUDED


typedef struct Emotion {
        char *triggerWord;

        // any of these can be 0, indicating nothing present
        
        // each is an object ID
        
        // drawn with 0,0 location relative to center of mainEyesPos
        // drawn on top of current eyes
        int eyeEmot;

        // drawn relative to head position
        // drawn in place of mouth
        int mouthEmot;

        // drawn relative to head position
        // drawn on top of head sprite
        int otherEmot;

    } Emotion;



// loads from settings folder
void initEmotion();


void freeEmotion();


// -1 if no emotion triggered
int getEmotionIndex( const char *inSpeech );


// returns NULL if index out of range
Emotion *getEmotion( int inIndex );



// keep emotions in live object set
void markEmotionsLive();



#endif

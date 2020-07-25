#include "language.h"

#include <stdlib.h>


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/MinPriorityQueue.h"



#include "languageIncludes/startingConsonantClusters.cpp"
#include "languageIncludes/endingConsonantClusters.cpp"
#include "languageIncludes/startingVowelClusters.cpp"
#include "languageIncludes/vowelClusters.cpp"
#include "languageIncludes/middleConsonantClusters.cpp"

// these must be included after
#include "languageIncludes/startingConsonantClustersFreq.cpp"
#include "languageIncludes/endingConsonantClustersFreq.cpp"
#include "languageIncludes/startingVowelClustersFreq.cpp"
#include "languageIncludes/vowelClustersFreq.cpp"
#include "languageIncludes/middleConsonantClustersFreq.cpp"


#include <limits.h>


// controls how far away clusters can map, based on frequency of
// occurrence.  Low values mean common clusters only map to other common 
// clusters, and uncommon clusters only map to uncommon cluster.
static int closeSetSize = 3;


// max age where hearing an utterance in another language has impact
static double maxLanguageLearningAge = 3.0;

// fraction of each utterance that hearer learns 
// (coin flip for each phoneme heard)
static double languageLearningRate = 0.5;

// Words repeated within the last X words are ignored for learning purposes
static int languageLearningRepetitionLimit = 20;

// When how much more of a language a learner learns when they first
// hear the language at a young age (a sudden jump in learning before
// more gradual learning based on learning rate.
static double languageLearningBaseFraction = 0.1;




#define NUM_CLUSTER_SETS 5

#define START_I 0
#define END_I 1
#define START_VOWEL_I 2
#define VOWEL_I 3
#define MID_I 4


const int allClusterSizes[ NUM_CLUSTER_SETS ] = 
{ NUM_STARTING_CONSONANT_CLUSTERS,
  NUM_ENDING_CONSONANT_CLUSTERS,
  NUM_STARTING_VOWEL_CLUSTERS,
  NUM_VOWEL_CLUSTERS,
  NUM_MIDDLE_CONSONANT_CLUSTERS };


const char **allClusters[ NUM_CLUSTER_SETS ] = 
{ startingConsonantClusters,
  endingConsonantClusters,
  startingVowelClusters,
  vowelClusters,
  middleConsonantClusters };


const int *allClustersFreq[ NUM_CLUSTER_SETS ] = 
{ startingConsonantClustersFreq,
  endingConsonantClustersFreq,
  startingVowelClustersFreq,
  vowelClustersFreq,
  middleConsonantClustersFreq };


int allClustersFreqTotals[ NUM_CLUSTER_SETS ];



typedef struct ClusterIndex {
        int setIndex;
        int index;
    } ClusterIndex;







typedef struct SpotSortRecord {
        int m;
        int freq;
    } SpotSortRecord;

    

static CustomRandomSource randSource( 12483 );



// self-reversing shuffle that swaps elements with others
// in list that have closest freuqency
// inCloseWindow defines how far apart they can get
static void closeFreqMirrorShuffle( SimpleVector<int> *inIndexList,
                                    const int *inFreq,
                                    int inCloseWindow ) {

    int len = inIndexList->size();
    
    // indexes into inIndexList that haven't been swapped yet
    SimpleVector<int> spotsLeft;
    SimpleVector<int> spotsFreq;
    
    // sort these by freq to make operations below faster
    // more likely to find equal-value freqs near each other
    // when searching in inner loop below

    SimpleVector<SpotSortRecord> sortRecords( len );
    
    MinPriorityQueue<SpotSortRecord> sortQueue;
    for( int i=0; i<len; i++ ) {
        SpotSortRecord r = { i, inFreq[i] };
        sortQueue.insert( r, r.freq );
        }

    for( int i=0; i<len; i++ ) {
        SpotSortRecord r = sortQueue.removeMin();
        
        spotsLeft.push_back( r.m );
        spotsFreq.push_back( r.freq );
        }
    
    // if odd, one spot remains unswapped
    int numPairs = len / 2;
    for( int p=0; p<numPairs; p++ ) {

        int numSpotsLeft = spotsLeft.size();
        
        // start at end, because deleteElement cheaper there
        int aLoc = numSpotsLeft - 1;
        int indA = spotsLeft.getElementDirect( aLoc );
        
        int freqA = spotsFreq.getElementDirect( aLoc );

        spotsLeft.deleteElement( aLoc );
        spotsFreq.deleteElement( aLoc );
        numSpotsLeft --;
        
        SimpleVector<int> closeBLoc;
        SimpleVector<int> closeBInd;
        
        int thisWindSize = inCloseWindow;
        if( thisWindSize > spotsLeft.size() ) {
            thisWindSize = spotsLeft.size();
            }

        int lastClosestSeenDist = 0;
        
        while( closeBLoc.size() < thisWindSize ) {
            int minLoc = -1;
            int minDist = INT_MAX;
            
            // run backwards, to find stuff near aLoc, which is
            // most likely to have similar frequency
            for( int m=numSpotsLeft - 1; m>=0; m-- ) {
                
                int testFreq = spotsFreq.getElementDirectFast( m );
                
                int dist = abs( testFreq - freqA );
                

                if( dist < minDist &&
                    ( dist > lastClosestSeenDist ||
                      ( dist == lastClosestSeenDist &&
                        // not already on our list
                        closeBLoc.getElementIndex( m ) == -1 ) ) ) {
                    
                    minLoc = m;
                    minDist = dist;
                    
                    if( dist == lastClosestSeenDist ) {
                        // tie for last seen, stop here
                        // can't get better than this
                        break;
                        }
                    }
                }

            if( minLoc == -1 ) {
                // none found
                printf( "Non found!\n" );
                break;
                }

            lastClosestSeenDist = minDist;
            
            closeBLoc.push_back( minLoc );
            closeBInd.push_back( spotsLeft.getElementDirect( minLoc ) );
            }
        

        

        
        if( closeBInd.size() > 0 ) {
        
            int closePick = 
                randSource.getRandomBoundedInt( 0, closeBInd.size() -1 );
            int indB = closeBInd.getElementDirect( closePick );
            
            int closePickLoc = closeBLoc.getElementDirect( closePick );
            
            spotsLeft.deleteElement( closePickLoc );
            spotsFreq.deleteElement( closePickLoc );
            
            //printf( "Swapping index %d with %d\n", indA, indB );
            

            // swap A and B in main vector
            int temp = inIndexList->getElementDirect( indA );
            
            *( inIndexList->getElement( indA ) ) =
                inIndexList->getElementDirect( indB );
            
            *( inIndexList->getElement( indB ) ) = temp;
            }
        }
    
    }



typedef struct LanguageLearningMap {
        int eveIDA;
        int eveIDB;
        
        // records this player's learning of the language map between
        // these two eves
        int playerID;

        // false until first utterance heard by listener
        char firstPhraseHeard;
        
        // these are false if the mapping is not learned, true if learned
        char startingMapping[ NUM_STARTING_CONSONANT_CLUSTERS ];
        char endingMapping[ NUM_ENDING_CONSONANT_CLUSTERS ];
        char startingVowelMapping[ NUM_STARTING_VOWEL_CLUSTERS ];
        char vowelMapping[ NUM_VOWEL_CLUSTERS ];
        char middleMapping[ NUM_MIDDLE_CONSONANT_CLUSTERS ];

        char *allMappings[ NUM_CLUSTER_SETS ];
        
        // keep a log of recenly-heard words, and
        // ignore learning more about that word until enough other
        // words have come in between
        SimpleVector<char*> recentWords;
    } LanguageLearningMap;




#define NUM_VOWELS 6

const char vowels[ NUM_VOWELS ] = { 'a', 'e', 'i', 'o', 'u', 'y' };


static char isVowelStart( const char *inSyllable, char inAllowY = false ) {
    // y not vowel at start
    int limit = NUM_VOWELS - 1;
    
    if( inAllowY ) {
        limit ++;
        }

    for( int i=0; i<limit; i++ ) {
        if( inSyllable[0] == vowels[i] ) {
            return true;
            }
        }
    return false;
    }



// returns pointer to remainder of word after cluster removed
// outClustIndex = -1 if none found
static char *findInitialCluster( char *inWord,
                                 int inNumSourceClusters,
                                 const char *inSourceClusters[],
                                 int *outClustIndex ) {
    
    for( int c=0; c<inNumSourceClusters; c++ ) {
            
        const char *clust = inSourceClusters[c];
            
        char *loc = strstr( inWord, clust );
            
        if( loc == inWord ) {
            // found at start;
            *outClustIndex = c;
                
            // skip it
            return &( inWord[ strlen( clust ) ] );
            }
        }

    *outClustIndex = -1;
    return inWord;
    }




// Maps inSourceClusters to inDestClusters based on set and cluster index
// Applies the combined learning of both learning maps
// If the listener (B) can still learn, we potentially update their learning
// map for this cluster.
static const char *clusterMap( 
    const char **inSourceClusters[ NUM_CLUSTER_SETS ],
    const char **inDestClusters[ NUM_CLUSTER_SETS ],
    int inSetIndex,
    int inClusterIndex,
    LanguageLearningMap *inLearnA,
    LanguageLearningMap *inLearnB,
    char inCanLearnB ) {
    
    if( inLearnA->allMappings[inSetIndex][inClusterIndex] ||
        inLearnB->allMappings[inSetIndex][inClusterIndex] ) {
        // know this cluster already, skip mapping it
        return inSourceClusters[ inSetIndex ][ inClusterIndex ];
        }
    else {   
        // map it
        const char *returnVal = 
            inDestClusters[ inSetIndex ][ inClusterIndex ];
        
        if( inCanLearnB &&
            randSource.getRandomDouble() > 1.0 - languageLearningRate ) {
            // weighted coin flipped heads
            
            // B learns this for the future
            inLearnB->allMappings[inSetIndex][inClusterIndex] = true;
            
            // need back-mapping too
            // search source map for dest cluster
            // mark the back-mapping learned too
            for( int c=0; c<allClusterSizes[inSetIndex]; c++ ) {
                if( strcmp( inSourceClusters[ inSetIndex ][c],
                            returnVal ) == 0 ) {
                    inLearnB->allMappings[inSetIndex][c] = true;
                    break;
                    }
                }
            // don't need to hear it repeated
            // know it the first time we learn it
            return inSourceClusters[ inSetIndex ][ inClusterIndex ];
            }
        return returnVal;
        }
    }





// if canLearnB, B's map is updated after the mapping for any clusters
// used
static char *remapWordNew( char *inWord, 
                           const char **inSourceClusters[ NUM_CLUSTER_SETS ],
                           const char **inDestClusters[ NUM_CLUSTER_SETS ],
                           LanguageLearningMap *inLearnA,
                           LanguageLearningMap *inLearnB,
                           char inCanLearnB ) {
    
    char *wordWorkingOrig = stringDuplicate( inWord );

    char *wordWorking = wordWorkingOrig;
    

    // first, find start cons cluster, if there is one

    int startClusterIndex = -1;
    
    if( ! isVowelStart( wordWorking ) ) {
        wordWorking = findInitialCluster( wordWorking, 
                                          allClusterSizes[ START_I ],
                                          inSourceClusters[ START_I ],
                                          &startClusterIndex );
        }
    
    int startVowelClusterIndex = -1;
    
    if( startClusterIndex == -1 ) {
        // starts with a vowel, treat it separately
        
        // (so we don't map it into a y-cluster by accident, breakign
        // reverse mapping)
        wordWorking = findInitialCluster( 
            wordWorking, 
            allClusterSizes[ START_VOWEL_I ],
            inSourceClusters[ START_VOWEL_I ],
            &startVowelClusterIndex );
        }
    


    // now longest consonant end cluster
    int endClusterIndex = -1;
    
    unsigned int remainLen = strlen( wordWorking );

    for( int c=0; c<allClusterSizes[ END_I ]; c++ ) {
        
        const char *clust = inSourceClusters[ END_I ][c];
        
        char *loc = strstr( wordWorking, clust );
        
        if( loc != NULL &&
            strlen( clust ) ==
            (unsigned int)( ( remainLen - ( loc - wordWorking ) ) ) ) {
            
            // match, and at end of word
            endClusterIndex = c;
            
            // terminate here to skip it
            loc[0] = '\0';
            }
        }
    
    // now search middle for vowel and consonant clusters

    SimpleVector<ClusterIndex> middleClusterIndices;
    
    while( strlen( wordWorking ) > 0 ) {
        // some letters left
        
        int setIndex = -1;
        int midIndex = -1;
        
        if( isVowelStart( wordWorking, true ) ) {
            setIndex = VOWEL_I;
            }
        else {
            setIndex = MID_I;
            }
        
        wordWorking = findInitialCluster( wordWorking, 
                                          allClusterSizes[ setIndex ],
                                          inSourceClusters[ setIndex ],
                                          &midIndex );
            
        if( midIndex != -1 ) {
            ClusterIndex ci = { setIndex, midIndex };
            middleClusterIndices.push_back( ci );
            }
        else {
            // none found?
            printf( "No mid cluster found in remainder of word %s\n",
                    wordWorking );
            break;
            }
        }
    
    
    SimpleVector<char> mappedWord;
    
    if( startClusterIndex != -1 ) {
        mappedWord.appendElementString( 
            clusterMap( 
                inSourceClusters,
                inDestClusters,
                START_I,
                startClusterIndex,
                inLearnA,
                inLearnB,
                inCanLearnB ) );
        //mappedWord.appendElementString( 
        //    inDestClusters[ START_I ][ startClusterIndex ] );
        }
    if( startVowelClusterIndex != -1 ) {
        mappedWord.appendElementString( 
            clusterMap( 
                inSourceClusters,
                inDestClusters,
                START_VOWEL_I,
                startVowelClusterIndex,
                inLearnA,
                inLearnB,
                inCanLearnB ) );
        //mappedWord.appendElementString( 
        //    inDestClusters[ START_VOWEL_I ][ startVowelClusterIndex ] );
        }
    
    for( int m=0; m < middleClusterIndices.size(); m++ ) {
        ClusterIndex ci = middleClusterIndices.getElementDirect( m );
        
        mappedWord.appendElementString( 
            clusterMap( 
                inSourceClusters,
                inDestClusters,
                ci.setIndex,
                ci.index,
                inLearnA,
                inLearnB,
                inCanLearnB ) );
        //mappedWord.appendElementString(
        //    inDestClusters[ ci.setIndex ][ ci.index ] );
        }
    // add any remaining portion of the word, which could not
    // be matched to mid clusters
    // (this is an error condition)
    if( strlen( wordWorking ) > 0 ) {
        mappedWord.appendElementString( wordWorking );
        }
    if( endClusterIndex != -1 ) {
        mappedWord.appendElementString( 
            clusterMap( 
                inSourceClusters,
                inDestClusters,
                END_I,
                endClusterIndex,
                inLearnA,
                inLearnB,
                inCanLearnB ) );
        //mappedWord.appendElementString( 
        //    inDestClusters[ END_I ][ endClusterIndex ] );
        }
    
    delete [] wordWorkingOrig;
    
    return mappedWord.getElementString();
    }






typedef struct BidirectionalLanguageMap {
        int eveIDA;
        int eveIDB;
        
        int seed;

        const char *startingMapping[ NUM_STARTING_CONSONANT_CLUSTERS ];
        const char *endingMapping[ NUM_ENDING_CONSONANT_CLUSTERS ];
        const char *startingVowelMapping[ NUM_STARTING_VOWEL_CLUSTERS ];
        const char *vowelMapping[ NUM_VOWEL_CLUSTERS ];
        const char *middleMapping[ NUM_MIDDLE_CONSONANT_CLUSTERS ];

        const char **allMappings[ NUM_CLUSTER_SETS ];
    } BidirectionalLanguageMap;



static void initMapping( BidirectionalLanguageMap *inMap,
                         int inEveIDA,
                         int inEveIDB,
                         int inRandSeed ) {
    
    inMap->eveIDA = inEveIDA;
    inMap->eveIDB = inEveIDB;
    inMap->seed = inRandSeed;

    inMap->allMappings[ START_I ] = inMap->startingMapping;
    inMap->allMappings[ END_I ] = inMap->endingMapping;
    inMap->allMappings[ START_VOWEL_I ] = inMap->startingVowelMapping;
    inMap->allMappings[ VOWEL_I ] = inMap->vowelMapping;
    inMap->allMappings[ MID_I ] = inMap->middleMapping;
    
    randSource.reseed( inRandSeed );

    SimpleVector<int> shuffles[NUM_CLUSTER_SETS];
    

    for( int s=0; s<NUM_CLUSTER_SETS; s++ ) {
        for( int c=0; c<allClusterSizes[s]; c++ ) {
            shuffles[s].push_back( c );
            }
        closeFreqMirrorShuffle( &( shuffles[s] ), 
                                allClustersFreq[ s ],
                                closeSetSize );
        
        for( int c=0; c<allClusterSizes[s]; c++ ) {            
            inMap->allMappings[s][ shuffles[s].getElementDirect( c ) ] = 
                allClusters[s][ c ];
            }
        }
    
    }


    


typedef struct EveLangRecord {
        int eveID;
        int langCount;
        
        // maps where this eve is eveIDA
        SimpleVector<BidirectionalLanguageMap *> languageMaps;
    } EveLangRecord;

    
static SimpleVector<EveLangRecord*> langRecords;



static char checkForWordRepeat( LanguageLearningMap *inMap,
                                char *inWord ) {
    for( int i=0; i< inMap->recentWords.size(); i++ ) {
        char *otherWord = inMap->recentWords.getElementDirect( i );
        
        if( strcmp( inWord, otherWord ) == 0 ) {
            // hit
            // don't move to end
            return true;
            }
        }
    
    // no hit yet
    inMap->recentWords.push_back( stringDuplicate( inWord ) );
    
    if( inMap->recentWords.size() > languageLearningRepetitionLimit ) {
        // list too long, remove oldest
        delete [] inMap->recentWords.getElementDirect( 0 );
        inMap->recentWords.deleteElement( 0 );
        }
    return false;
    }

    
    



static void initMapping( LanguageLearningMap *inMap,
                         int inEveIDA,
                         int inEveIDB,
                         int inPlayerID ) {
    inMap->eveIDA = inEveIDA;
    inMap->eveIDB = inEveIDB;
    inMap->playerID = inPlayerID;
    inMap->firstPhraseHeard = false;

    inMap->allMappings[ START_I ] = inMap->startingMapping;
    inMap->allMappings[ END_I ] = inMap->endingMapping;
    inMap->allMappings[ START_VOWEL_I ] = inMap->startingVowelMapping;
    inMap->allMappings[ VOWEL_I ] = inMap->vowelMapping;
    inMap->allMappings[ MID_I ] = inMap->middleMapping;


    for( int s=0; s<NUM_CLUSTER_SETS; s++ ) {    
        memset( inMap->allMappings[s], false, allClusterSizes[s] );
        }
    }


LanguageLearningMap *cloneMapping( LanguageLearningMap *inMapping ) {
    LanguageLearningMap *c = new LanguageLearningMap;
    
    initMapping( c, inMapping->eveIDA, inMapping->eveIDB, inMapping->playerID );
    
    c->firstPhraseHeard = inMapping->firstPhraseHeard;

    for( int s=0; s<NUM_CLUSTER_SETS; s++ ) {
        memcpy( c->allMappings[s], inMapping->allMappings[s], 
                allClusterSizes[s] );
        }
    return c;
    }



typedef struct PlayerLearningRecord {
        int playerID;
        SimpleVector<LanguageLearningMap*> learningMaps;
    } PlayerLearningRecord;


static SimpleVector<PlayerLearningRecord *> learningRecords;


static LanguageLearningMap blankLearningMap;




// if age too old to learn, and no map exists, returns NULL
// if young enough to learn, and no map exists, returns fresh map
static LanguageLearningMap *getPlayerLearningMap( int inEveIDA, int inEveIDB,
                                                  int inPlayerID,
                                                  double inPlayerAge,
                                                  int inParentID = -1 ) {
    
    PlayerLearningRecord *playerRec = NULL;
    
    for( int p=0; p < learningRecords.size(); p++ ) {
        PlayerLearningRecord *r = learningRecords.getElementDirect( p );
        
        if( r->playerID == inPlayerID ) {
            playerRec = r;
            break;
            }
        }
    
    if( playerRec == NULL ) {
        // no rec for this player
        if( inPlayerAge > maxLanguageLearningAge ) {
            return NULL;
            }
        playerRec = new PlayerLearningRecord;
        playerRec->playerID = inPlayerID;
        
        learningRecords.push_back( playerRec );
        }

    LanguageLearningMap *map = NULL;
    
    for( int m=0; m< playerRec->learningMaps.size(); m++ ) {
        LanguageLearningMap *checkMap = 
            playerRec->learningMaps.getElementDirect( m );
        
        if( checkMap->eveIDA == inEveIDA &&
            checkMap->eveIDB == inEveIDB ) {
            map = checkMap;
            break;
            }
        }

    if( map == NULL ) {
        // no player map for this eve combo
        if( inPlayerAge > maxLanguageLearningAge ) {
            return NULL;
            }

        // copy from parent, if it exists
        LanguageLearningMap *parentMap = NULL;
        
        if( inParentID != -1 ) {
            // NULL if parent dead or parent never learned this language
            parentMap = 
                getPlayerLearningMap( inEveIDA, inEveIDB,
                                      inParentID,
                                      // dummy parent age
                                      // don't allow creation of new parent
                                      // map here
                                      maxLanguageLearningAge + 1 );
            }
        

        // add a blank one
        map = new LanguageLearningMap;
        initMapping( map, inEveIDA, inEveIDB, inPlayerID );

        if( parentMap != NULL ) {
            
            // copy parent's map            
            for( int s=0; s<NUM_CLUSTER_SETS; s++ ) {    
                memcpy( map->allMappings[s], parentMap->allMappings[s],
                        allClusterSizes[s] );
                }
            }

        playerRec->learningMaps.push_back( map );
        }
    
    return map;
    }





void initLanguage() {
    initMapping( &blankLearningMap, 0, 0, 0 );

    for( int i=0; i<NUM_CLUSTER_SETS; i++ ) {
        allClustersFreqTotals[ i ] = 0;
        for( int c=0; c<allClusterSizes[i]; c++ ) {
            allClustersFreqTotals[ i ] += allClustersFreq[i][c];
            }
        }

    stepLanguage();
    }



static void freeEveLangRecord( EveLangRecord *inR ) {
    for( int m=0; m < inR->languageMaps.size(); m++ ) {
        delete inR->languageMaps.getElementDirect( m );
        }
    
    delete inR;
    }


static void freePlayerLearningRecord( PlayerLearningRecord *inR ) {
    for( int m=0; m < inR->learningMaps.size(); m++ ) {
        inR->learningMaps.getElementDirect( m )->
            recentWords.deallocateStringElements();
        
        delete inR->learningMaps.getElementDirect( m );
        }
    
    delete inR;
    }


void removePlayerLanguageMaps( int inPlayerID ) {
    for( int p=0; p<learningRecords.size(); p++ ) {
        PlayerLearningRecord *r = learningRecords.getElementDirect( p );
        if( r->playerID == inPlayerID ) {
            freePlayerLearningRecord( r );
            learningRecords.deleteElement( p );
            return;
            }
        }
    }



void freeLanguage() {

    for( int e=0; e<langRecords.size(); e++ ) {
        EveLangRecord *r = langRecords.getElementDirect( e );
        
        freeEveLangRecord( r );
        }
    langRecords.deleteAll();

    for( int e=0; e<learningRecords.size(); e++ ) {
        PlayerLearningRecord *r = learningRecords.getElementDirect( e );
        
        freePlayerLearningRecord( r );
        }
    learningRecords.deleteAll();
    }



void addEveLanguage( int inPlayerID ) {
    EveLangRecord *r = new EveLangRecord;
    r->eveID = inPlayerID;
    r->langCount = 1;
    
    langRecords.push_back( r );
    }



static EveLangRecord *getLangRecord( int inEveID, int *outIndex ) {
    for( int e=0; e<langRecords.size(); e++ ) {
        EveLangRecord *r = langRecords.getElementDirect( e );
        if( r->eveID == inEveID ) {
            *outIndex = e;
            return r;
            }
        }
    return NULL;
    }



void incrementLanguageCount( int inEveID ) {
    int e;
    EveLangRecord *r = getLangRecord( inEveID, &e );
    
    if( r != NULL ) {
        r->langCount ++;
        }
    }


int decrementLanguageCount( int inEveID ) {
    int countLeft = 0;
    
    int rInd;
    EveLangRecord *r = getLangRecord( inEveID, &rInd );
    
    if( r != NULL ) {
        r->langCount --;

        countLeft = r->langCount;

        if( r->langCount <= 0 ) {
            // language is dead, last speaker died
            countLeft = 0;
            
            freeEveLangRecord( r );                        
            langRecords.deleteElement( rInd );

            // now walk through and remove other mappings
            for( int e=0; e<langRecords.size(); e++ ) {
                EveLangRecord *rOther = langRecords.getElementDirect( e );
                
                for( int m=0; m < rOther->languageMaps.size(); m++ ) {
                    BidirectionalLanguageMap *map =
                        rOther->languageMaps.getElementDirect( m );
                    
                    if( map->eveIDA == inEveID ||
                        map->eveIDB == inEveID ) {
                        delete map;
                        
                        rOther->languageMaps.deleteElement( m );
                        m--;
                        }
                    }
                }
            }
        }
    
    return countLeft;
    }




void stepLanguage() {
    // reload settings
    maxLanguageLearningAge = 
        SettingsManager::getFloatSetting( "maxLanguageLearningAge", 3.0 );
    languageLearningRate = 
        SettingsManager::getFloatSetting( "languageLearningRate", 0.5 );
    languageLearningRepetitionLimit = 
        SettingsManager::getIntSetting( "languageLearningRepetitionLimit", 20 );
    languageLearningBaseFraction = 
        SettingsManager::getFloatSetting( "languageLearningBaseFraction", 0.1 );
    
    // see if there's one mapping that needs generating
    // spread the work out for generating mappings
    for( int e=0; e<langRecords.size(); e++ ) {
        EveLangRecord *r = langRecords.getElementDirect( e );
        
        int numMappingsThisShouldHave = e;
        
        int numMappingsThisHas = r->languageMaps.size();
        
        if( numMappingsThisHas < numMappingsThisShouldHave ) {
            
            // generate one mapping
            BidirectionalLanguageMap *map = new BidirectionalLanguageMap;
            
            int otherEveID = 
                langRecords.getElementDirect( numMappingsThisHas )->eveID;
                         
            int seed = r->eveID + otherEveID + time( NULL );

            initMapping( map,
                         r->eveID,
                         otherEveID,
                         seed );
            
            r->languageMaps.push_back( map );
                         
            return;
            }
        }
    }


// handles punctuation, etc.
char *mapLanguagePhrase( char *inPhrase, int inEveIDA, int inEveIDB,
                         int inPlayerIDA, int inPlayerIDB,
                         double inAgeA, double inAgeB,
                         int inParentIDA, int inParentIDB,
                         double inFractionToPassThrough ) {

    if( inEveIDA == inEveIDB ) {
        // self, no translation
        return stringDuplicate( inPhrase );
        }

    int lookupEveID = inEveIDA;
    int otherEveID = inEveIDB;
    
    if( inEveIDB > lookupEveID ){
        lookupEveID = inEveIDB;
        otherEveID = inEveIDA;
        }


    // these can be NULL
    LanguageLearningMap *learnA = getPlayerLearningMap( lookupEveID, otherEveID,
                                                        inPlayerIDA,
                                                        inAgeA,
                                                        inParentIDA );
    LanguageLearningMap *learnB = getPlayerLearningMap( lookupEveID, otherEveID,
                                                        inPlayerIDB,
                                                        inAgeB,
                                                        inParentIDB );
    
    
    // only B, the listener, can ever learn more
    char canLearnB = ( inAgeB <= maxLanguageLearningAge );
    
    if( learnA == NULL ) {
        learnA = &blankLearningMap;
        }

    if( learnB == NULL ) {
        learnB = &blankLearningMap;
        // never update blank map
        canLearnB = false;
        }
    
    
    if( canLearnB && ! learnB->firstPhraseHeard &&
        languageLearningBaseFraction > 0 ) {
        
        // apply base learning fraction now
                
        for( int i=0; i<NUM_CLUSTER_SETS; i++ ) {

            // how much unlearned weight is left in this set?
            int freqTotalLeftToLearn = 0;

            for( int c=0; c<allClusterSizes[i]; c++ ) {
                if( ! learnB->allMappings[i][c] ) {
                    freqTotalLeftToLearn += allClustersFreq[i][c];
                    }
                }
            
            if( freqTotalLeftToLearn == 0 ) {
                // done learning this set entirely
                continue;
                }

            // fraction of remaining weight
            // for example, 10% of what's left, over and over
            // Thus, the total weight learned is asymptotic to 100% over time
            // but never actually gets there.
            // We learn something like 10%, 19%, 27%, 34%, etc.
            // it will get there at very end, once the fraction of 
            // remaining weight left to learn is < 1, because we round up
            // to 1.
            int freqTotalToLearnThisRun = 
                ceil( freqTotalLeftToLearn * languageLearningBaseFraction );

            int freqTotalLearnedThisRun = 0;
            
            int c = 0;
            
            while( freqTotalLearnedThisRun < freqTotalToLearnThisRun ) {
                // jump into remaining clusters randomly, with
                // our chance of landing on a given cluster based on its
                // fraction of the remaining weight
                int freqToSkip = 
                    randSource.getRandomBoundedInt( 0,
                                                    freqTotalLeftToLearn );
                // jump into the remaining clusters by freqToSkip amount
                
                int clusterHit = -1;
                
                int freqSkipped = 0;
                for( int s=0; s<allClusterSizes[i]; s++ ) {
                    // skip already-learned clusters during this
                    // weighted picking process
                    if( ! learnB->allMappings[i][s] ) {
                        freqSkipped += allClustersFreq[i][s];
                    
                        if( freqSkipped >= freqToSkip ) {
                            // landed in this cluster
                            clusterHit = s;
                            break;
                            }
                        }
                    }
                
                if( clusterHit != -1 ) {
                    int s = clusterHit;
                    
                    learnB->allMappings[i][s] = true;
                    freqTotalLearnedThisRun += allClustersFreq[i][s];
                    
                    freqTotalLeftToLearn -= allClustersFreq[i][s];
                    }
                else {
                    // no clusters left to learn
                    break;
                    }
                c++;
                }
            }
        
        learnB->firstPhraseHeard = true;
        }


    // temporarily replace map with a stripped one that lets more pass through
    // but don't do this if gradual learning over time is enabled
    char deleteMapA = false;
    
    if( learnA != NULL && inFractionToPassThrough > 0 && 
        languageLearningRate == 0 ) {
        
        LanguageLearningMap *tweakedMap = cloneMapping( learnA );
        
        int seed = inEveIDA + inEveIDB;
        
        CustomRandomSource tweakRand( seed );
        
        for( int s=0; s<NUM_CLUSTER_SETS; s++ ) {
            for( int c=0; c<allClusterSizes[s]; c++ ) {   
                if( tweakRand.getRandomDouble() <= inFractionToPassThrough ) {
                    tweakedMap->allMappings[s][c] = true;
                    }
                }
            }

        
        learnA = tweakedMap;

        deleteMapA = true;
        }


    char *returnString = NULL;
    
    for( int e=0; e<langRecords.size(); e++ ) {
        EveLangRecord *r = langRecords.getElementDirect( e );

        if( r->eveID == lookupEveID ) {
            for( int m=0; m< r->languageMaps.size(); m++ ) {
                
                BidirectionalLanguageMap *map =
                    r->languageMaps.getElementDirect( m );
                
                if( map->eveIDB == otherEveID ) {
                    // found it!
                    
                    char *lcPhrase = stringToLowerCase( inPhrase );

                    SimpleVector<char*> words;
                    SimpleVector<char> wordIsAlpha;

                    int nextChar = 0;
                    int len = strlen( lcPhrase );
                    
                    while( nextChar < len ) {
                        
                        SimpleVector<char> thisWord;
                        
                        while( nextChar < len &&
                               lcPhrase[ nextChar ] >= 'a' &&
                               lcPhrase[ nextChar ] <= 'z' ) {
                            
                            thisWord.push_back( lcPhrase[ nextChar ] );
                            nextChar++;
                            }
                        if( thisWord.size() > 0 ) {
                            words.push_back( thisWord.getElementString() );
                            wordIsAlpha.push_back( true );
                            }

                        while( nextChar < len &&
                               ( lcPhrase[ nextChar ] < 'a' ||
                                 lcPhrase[ nextChar ] > 'z' )  ) {

                            // non-alpha chars are each separate words
                            char *charWord = new char[2];
                            charWord[0] = lcPhrase[ nextChar ];
                            charWord[1] = '\0';
                            
                            words.push_back( charWord );
                            wordIsAlpha.push_back( false );
                            
                            nextChar++;
                            }
                        }
                    
                    delete [] lcPhrase;
                    

                    SimpleVector<char> transPhraseWorking;                    

                    for( int w=0; w<words.size(); w++ ) {
                        char *word = words.getElementDirect( w );
                        
                        if( wordIsAlpha.getElementDirect( w ) ) {
                            
                            char canLearnThisWord = canLearnB;
                            
                            if( canLearnThisWord ) {
                                // make sure it's not a repeat
                                canLearnThisWord = 
                                    ! checkForWordRepeat( learnB, word );
                                }
                            
                            
                            char *newWord = remapWordNew( word, 
                                                          allClusters,
                                                          map->allMappings,
                                                          learnA,
                                                          learnB,
                                                          canLearnThisWord );

                            transPhraseWorking.appendElementString( newWord );
                            delete [] newWord;
                            }
                        else {
                            // add non-alpha directly
                            transPhraseWorking.appendElementString( word );
                            }
                        
                        delete [] word;
                        }

                    char *newPhrase = transPhraseWorking.getElementString();
                    
                    char *ucNew = stringToUpperCase( newPhrase );
                    delete [] newPhrase;
                    
                    if( deleteMapA ) {
                        delete learnA;
                        }
                    
                    return ucNew;
                    }
                }
            
            }
        }
    
    
    if( returnString == NULL ) {
        returnString = stringDuplicate( inPhrase );
        }
    
    if( deleteMapA ) {
        delete learnA;
        }            

    return returnString;
    }

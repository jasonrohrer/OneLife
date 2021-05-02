#ifndef LIVING_LIFE_PAGE_INCLUDED
#define LIVING_LIFE_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/util/SettingsManager.h"

#include "minorGems/game/game.h"


#include "transitionBank.h"

#include "GamePage.h"

#include "Picker.h"


#include "pathFind.h"

#include "animationBank.h"
#include "emotion.h"

#include "TextField.h"


#define NUM_HUNGER_BOX_SPRITES 20

#define NUM_TEMP_ARROWS 6
#define NUM_HUNGER_DASHES 6

#define NUM_HINT_SHEETS 4


#define NUM_HOME_ARROWS 8

#define NUM_YUM_SLIPS 4

namespace fovmod {
    extern float gui_fov_scale;
    extern int gui_fov_scale_hud;
    extern int gui_fov_offset_x;
    extern int gui_fov_offset_y;
}

typedef struct LiveObject {
        int id;

        int displayID;

        char allSpritesLoaded;

        char onScreen;
        
        double age;
        double ageRate;
        
        char finalAgeSet;

        double lastAgeSetTime;
        
        SimpleVector<int> lineage;
        
        int lineageEveID;
        

        char outOfRange;
        char dying;
        char sick;
        
        char *name;

        char *relationName;
        
        int curseLevel;
        
        int excessCursePoints;

        int curseTokenCount;
        

        // roll back age temporarily to make baby revert to crying
        // when baby speaks
        char tempAgeOverrideSet;
        double tempAgeOverride;
        double tempAgeOverrideSetTime;
        

        int foodStore;
        int foodCapacity;
        
        int maxFoodStore;
        int maxFoodCapacity;

        // -1 unless we're currently being held
        // by an adult
        int heldByAdultID;
        
        // -1 if not set
        // otherwise, ID of adult that is holding us according to pending
        // messages, but not according to already-played messages
        int heldByAdultPendingID;
        

        // usually 0, unless we're being held by an adult
        // and just got put down
        // then we slide back into position
        doublePair heldByDropOffset;

        // the actual world pos we were last held at
        char lastHeldByRawPosSet;
        doublePair lastHeldByRawPos;
        
        // track this so that we only send one jump message even if
        // the player clicks more than once before the server registers the
        // jump
        double jumpOutOfArmsSentTime;
        
        // true if locally-controlled baby is attempting to jump out of arms
        char babyWiggle;
        double babyWiggleProgress;

        
        // usually 0, but used to slide into and out of riding position
        doublePair ridingOffset;
        

        // 0 or positive holdingID means holding nothing or an object
        // a negative number here means we're holding another player (baby)
        // and the number, made positive, is the ID of the other player
        int holdingID;
        int lastHoldingID;

        char holdingFlip;

        double lastFlipSendTime;
        char lastFlipSent;

        char heldPosOverride;
        char heldPosOverrideAlmostOver;
        doublePair heldObjectPos;
        double heldObjectRot;
        int heldPosSlideStepCount;
        
        AnimType curAnim;
        AnimType lastAnim;
        double lastAnimFade;

        // anim tracking for held object
        AnimType curHeldAnim;
        AnimType lastHeldAnim;
        double lastHeldAnimFade;



        // furture states that curAnim should fade to, one at a time
        SimpleVector<AnimType> *futureAnimStack;
        SimpleVector<AnimType> *futureHeldAnimStack;
        
        // store frame counts in fractional form
        // this allows animations that can vary in speed without
        // ever experiencing discontinuities 
        // (an integer frame count, with a speed modifier applied later
        // could jump backwards in time when the modifier changes)
        double animationFrameCount;
        double heldAnimationFrameCount;

        double lastAnimationFrameCount;
        double lastHeldAnimationFrameCount;
        

        double frozenRotFrameCount;
        double heldFrozenRotFrameCount;
        
        char frozenRotFrameCountUsed;
        char heldFrozenRotFrameCountUsed;
        

        float heat;
        

        int numContained;
        int *containedIDs;
        SimpleVector<int> *subContainedIDs;
        
        ClothingSet clothing;
        // stacks of items contained in each piece of clothing
        SimpleVector<int> clothingContained[ NUM_CLOTHING_PIECES ];

        float clothingHighlightFades[ NUM_CLOTHING_PIECES ];
        
        int currentMouseOverClothingIndex;
        

        // current fractional grid position and speed
        doublePair currentPos;
        // current speed is move delta per frame
        double currentSpeed;

        // current move speed in grid cells per sec
        double currentGridSpeed;
        

        // for instant reaction to move command when server hasn't
        // responded yet
        // in grid spaces per sec
        double lastSpeed;

        // recompute speed periodically during move so that we don't
        // fall behind when frame rate fluctuates
        double timeOfLastSpeedUpdate;
        
        // destination grid position
        int xd;
        int yd;
        
        // true if xd,yd set based on a truncated PM from the server
        char destTruncated;
        
        
        // use a waypoint along the way during pathfinding.
        // path must pass through this point on its way to xd,yd
        char useWaypoint;
        int waypointX, waypointY;
        // max path length to find that uses waypoint
        // if waypoint-including path is longer than this
        // a path stopping at the waypoint will be used instead
        // and xd, yd will be repaced with waypoint
        int maxWaypointPathLength;
        

        // last confirmed stationary position of this
        // object on the server (from the last player_update)
        int xServer;
        int yServer;
        

        int pathLength;
        GridPos *pathToDest;

                
        int closestDestIfPathFailedX;
        int closestDestIfPathFailedY;
        

        int currentPathStep;
        doublePair currentMoveDirection;
        
        int numFramesOnCurrentStep;

        char onFinalPathStep;


        // how long whole move should take
        double moveTotalTime;
        
        // wall clock time in seconds object should arrive
        double moveEtaTime;

        // skip drawing this object
        char hide;
        
        char inMotion;
        
        int lastMoveSequenceNumber;

        char displayChar;
        
        int actionTargetX;
        int actionTargetY;
        
        // tweak for when we are performing an action on a moving object
        // that hasn't reach its destination yet.  actionTargetX,Y is the
        // destination, but this is the closest cell where it was at
        // when we clicked on it.
        int actionTargetTweakX;
        int actionTargetTweakY;

        char pendingAction;
        float pendingActionAnimationProgress;
        double pendingActionAnimationStartTime;
        
        double lastActionSendStartTime;
        // how long it took server to get back to us with a PU last
        // time we sent an action.  Most recent round-trip time
        double lastResponseTimeDelta;
        
        
        // NULL if no active speech
        char *currentSpeech;
        double speechFade;
        // wall clock time when speech should start fading
        double speechFadeETATime;

        char speechIsSuccessfulCurse;
        

        char shouldDrawPathMarks;
        double pathMarkFade;
        

        // messages that arrive while we're still showing our current
        // movement animation
        SimpleVector<char*> pendingReceivedMessages;
        char somePendingMessageIsMoreMovement;


        // NULL if none
        Emotion *currentEmot;
        // wall clock time when emot clears
        double emotClearETATime;

        char killMode;
        int killWithID;

    } LiveObject;




typedef struct GraveInfo {
        GridPos worldPos;
        char *relationName;
        // wall clock time when grave was created
        // (for old graves, estimated based on grave age
        // and current age rate)
        double creationTime;

        // if server sends -1 for grave age, we don't display age
        char creationTimeUnknown;

        // to prevent YEARS display from ticking up while we
        // are still mousing over (violates the erased pencil consistency)
        // -1 if not set
        int lastMouseOverYears;
        // last time we displayed a mouse-over label for this
        // used to detect when we've moused away, even if not mousing
        // over another grave
        double lastMouseOverTime;
    } GraveInfo;
        

typedef struct OwnerInfo {
    GridPos worldPos;
    
    SimpleVector<int> *ownerList;
    } OwnerInfo;





typedef struct PointerHitRecord {
        int closestCellX;
        int closestCellY;
        int hitSlotIndex;

        char hit;
        char hitSelf;
    
        char hitOtherPerson;
        int hitOtherPersonID;
        
        int hitClothingIndex;

        // when we click in a square, only count as hitting something
        // if we actually clicked the object there.  Else, we can walk
        // there if unblocked.
        char hitAnObject;

        // for case where we hit an object that we remembered placing
        // which may be behind
        // should NEVER click through a person
        char hitOurPlacement;

        // true if hitOurPlacement happened THROUGH another non-person object
        char hitOurPlacementBehind;
        

    } PointerHitRecord;



typedef struct OldArrow {
        int i;
        float heat;
        float fade;
    } OldArrow;



typedef struct HomeArrow {
        // true for at most one arrow, the current one
        char solid;
        
        // fade for erased arrows
        float fade;
    } HomeArrow;
        


// for objects moving in-transit in special cases where we can't store
// them in the map (if they're moving onto an occupied space that should
// only change when they're done moving)
// We track them separately from the map until they are done moving
typedef struct ExtraMapObject {
        int objectID;

        double moveSpeed;
        doublePair moveOffset;
        
        double animationFrameCount;
        double animationLastFrameCount;
        
        double animationFrozenRotFrameCount;
        
        AnimType curAnimType;
        AnimType lastAnimType;
        double lastAnimFade;
        char flip;

        SimpleVector<int> containedStack;
        SimpleVector< SimpleVector<int> > subContainedStack;
    } ExtraMapObject;
        
        



class LivingLifePage : public GamePage, public ActionListener {
        
    public:

        LivingLifePage();
        ~LivingLifePage();
        
        void clearMap();
        
        // enabled tutorail next time a connection loads
        void runTutorial();
        

        char isMapBeingPulled();

        // destroyed by caller
        // can be NULL
        char *getDeathReason();

        // prevent a jitter when frameRateFactor changes due to fps lag
        void adjustAllFrameCounts( double inOldFrameRateFactor,
                                   double inNewFrameRateFactor );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

        virtual void keyDown( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
        virtual void keyUp( unsigned char inASCII );

        
        // handles error detection, total byte counting, etc.
        void sendToServerSocket( char *inMessage );
        
        void sendBugReport( int inBugNumber );


        int getRequiredVersion() {
            return mRequiredVersion;
            }
			

		void setNextActionMessage( const char* str, int x, int y );
		int getObjId( int mapX, int mapY );
		bool objIdReverseAction( int objId );
		void pickUpBabyInRange();
		void pickUpBaby( int x, int y );
		void useBackpack( bool replace = false );
		void usePocket( int clothingID );
		void useOnSelf();
		void takeOffBackpack();
		void setOurSendPosXY(int &x, int &y);
		bool isCharKey(unsigned char c, unsigned char key);
		
		void actionAlphaRelativeToMe( int x, int y );
		void actionBetaRelativeToMe( int x, int y );
		void useTileRelativeToMe( int x, int y ) ;
		void dropTileRelativeToMe( int x, int y ) ;
		
		void movementStep();
		bool findNextMove(int &x, int &y, int dir);
		int getNextMoveDir(int direction, int add);
		int getMoveDirection();
		bool setMoveDirIfSafe(int &x, int &y, int dir);
		void setMoveDirection(int &x, int &y, int direction);
		bool tileHasClosedDoor(int x, int y);
		bool dirIsSafeToWalk(int x, int y, int dir);



		doublePair minitechGetLastScreenViewCenter();
		char *minitechGetDisplayObjectDescription(int objId);
		bool minitechSayFieldIsFocused() { return mSayField.isFocused(); }

        virtual void actionPerformed( GUIComponent *inTarget );
        

    protected:

        int mServerSocket;
        
        int mRequiredVersion;

        char mForceRunTutorial;
        int mTutorialNumber;

        char mGlobalMessageShowing;
        double mGlobalMessageStartTime;
        SimpleVector<char*>mGlobalMessagesToDestroy;
        

        int mFirstServerMessagesReceived;
        
        char mStartedLoadingFirstObjectSet;
        char mDoneLoadingFirstObjectSet;
        double mStartedLoadingFirstObjectSetStartTime;

        float mFirstObjectSetLoadingProgress;
        
        

        // an offset that we apply to all server-recieved coordinates
        // before storing them locally, and reverse-apply to all local
        // coordinates before sending them to the server.

        // This keeps our local coordinates in a low range and prevents
        // rounding errors caused by rendering huge integers as 32-bit floats
        // on most graphics cards.
        //
        // We base this on the center of the first map chunk received
        char mMapGlobalOffsetSet;
        GridPos mMapGlobalOffset;

        // conversion function for received coordinates into local coords
        void applyReceiveOffset( int *inX, int *inY );
        // converts local coors for sending back to server
		
	public:
        
        int sendX( int inX );
        int sendY( int inY );


        int mMapD;
		
        int *mMap;
        
	protected:
        
        int *mMapBiomes;
        int *mMapFloors;

        char *mMapCellDrawnFlags;

        double *mMapAnimationFrameCount;
        double *mMapAnimationLastFrameCount;
        
        double *mMapAnimationFrozenRotFrameCount;
        char *mMapAnimationFrozenRotFrameCountUsed;

        int *mMapFloorAnimationFrameCount;


        // all tiles on ground work their way toward animation type of
        // "ground" but may have a lingering types after being dropped
        AnimType *mMapCurAnimType;
        AnimType *mMapLastAnimType;
        double *mMapLastAnimFade;
        
        // 0,0 for most, except for a newly-dropped object
        // as it slides back into grid position
        doublePair *mMapDropOffsets;
        double *mMapDropRot;

        SoundUsage *mMapDropSounds;
        

        // 0, 0 for most, except objects that are moving
        doublePair *mMapMoveOffsets;
        // speed in CELL_D per sec
        double *mMapMoveSpeeds;
        

        // true if left-right flipped (to match last drop)
        // not tracked on server, so resets when object goes off of screen
        char *mMapTileFlips;
        

        SimpleVector<int> *mMapContainedStacks;

        SimpleVector< SimpleVector<int> > *mMapSubContainedStacks;
        
        
        // true if this map spot was something that our
        // player was responsible for placing
        char *mMapPlayerPlacedFlags;
        
        SimpleVector<GridPos> mMapExtraMovingObjectsDestWorldPos;
        SimpleVector<int> mMapExtraMovingObjectsDestObjectIDs;
        SimpleVector<ExtraMapObject> mMapExtraMovingObjects;

        public: // minitech
        int mMapOffsetX;
        int mMapOffsetY;
		protected: // minitech

        char mEKeyEnabled;
        char mEKeyDown;
        

        SpriteHandle mGuiPanelSprite;
        SpriteHandle mGuiBloodSprite;
        

        SpriteHandle mHungerBoxSprites[ NUM_HUNGER_BOX_SPRITES ];
        SpriteHandle mHungerBoxFillSprites[ NUM_HUNGER_BOX_SPRITES ];

        SpriteHandle mHungerBoxErasedSprites[ NUM_HUNGER_BOX_SPRITES ];
        SpriteHandle mHungerBoxFillErasedSprites[ NUM_HUNGER_BOX_SPRITES ];
        
        SpriteHandle mTempArrowSprites[ NUM_TEMP_ARROWS ];
        SpriteHandle mTempArrowErasedSprites[ NUM_TEMP_ARROWS ];

        SpriteHandle mHungerDashSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerDashErasedSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerBarSprites[ NUM_HUNGER_DASHES ];
        SpriteHandle mHungerBarErasedSprites[ NUM_HUNGER_DASHES ];

        SpriteHandle mNotePaperSprite;

        SpriteHandle mFloorSplitSprite;

        SpriteHandle mCellBorderSprite;
        SpriteHandle mCellFillSprite;
        

        SpriteHandle mHomeSlipSprite;
        SpriteHandle mHomeArrowSprites[ NUM_HOME_ARROWS ];
        SpriteHandle mHomeArrowErasedSprites[ NUM_HOME_ARROWS ];
		
		SpriteHandle sheetSprites[9] = {nullptr};
        
        HomeArrow mHomeArrowStates[ NUM_HOME_ARROWS ];

        SimpleVector<int> mCulvertStoneSpriteIDs;
        
        SimpleVector<char*> mPreviousHomeDistStrings;
        SimpleVector<float> mPreviousHomeDistFades;
        

        // offset from current view center
        doublePair mNotePaperHideOffset;
        doublePair mNotePaperPosOffset;
        doublePair mNotePaperPosTargetOffset;


        doublePair mHomeSlipHideOffset;
        doublePair mHomeSlipPosOffset;
        doublePair mHomeSlipPosTargetOffset;

        
        SimpleVector<char*> mLastKnownNoteLines;
        
        SimpleVector<char> mErasedNoteChars;
        SimpleVector<doublePair> mErasedNoteCharOffsets;
        SimpleVector<float> mErasedNoteCharFades;

        SimpleVector<char> mCurrentNoteChars;
        SimpleVector<doublePair> mCurrentNoteCharOffsets;
        
        SimpleVector<char*> mSentChatPhrases;

        
        SoundSpriteHandle mHungerSound;
        char mPulseHungerSound;

        SoundSpriteHandle mTutorialSound;
        SoundSpriteHandle mCurseSound;

        
        SpriteHandle mHungerSlipSprites[3];

        // offset from current view center
        doublePair mHungerSlipHideOffsets[3];
        doublePair mHungerSlipShowOffsets[3];
        doublePair mHungerSlipPosOffset[3];
        doublePair mHungerSlipPosTargetOffset[3];
        
        double mHungerSlipWiggleTime[3];
        double mHungerSlipWiggleAmp[3];
        double mHungerSlipWiggleSpeed[3];
        
        // for catching dir change at peak of starving slip
        // to time sound
        double mStarvingSlipLastPos[2];
                                    

        // index of visble one, or -1
        int mHungerSlipVisible;
        
        
        SpriteHandle mHintSheetSprites[NUM_HINT_SHEETS];
        
        // offset from current view center
        doublePair mHintHideOffset[NUM_HINT_SHEETS];
        doublePair mHintPosOffset[NUM_HINT_SHEETS];
        doublePair mHintTargetOffset[NUM_HINT_SHEETS];
        
        doublePair mHintExtraOffset[NUM_HINT_SHEETS];

        // # separates lines
        char *mHintMessage[NUM_HINT_SHEETS];
        int mHintMessageIndex[NUM_HINT_SHEETS];
        int mNumTotalHints[ NUM_HINT_SHEETS ];

        int mLiveHintSheetIndex;

        char mForceHintRefresh;
        
        int mCurrentHintObjectID;
        int mCurrentHintIndex;
        
        int mNextHintObjectID;
        int mNextHintIndex;

        SimpleVector<TransRecord *> mLastHintSortedList;
        int mLastHintSortedSourceID;
        char *mLastHintFilterString;
        
        // string that's waiting to be shown on hint-sheet 4
        char *mPendingFilterString;
        

        // table sized to number of possible objects
        int *mHintBookmarks;
        

        int getNumHints( int inObjectID );
        char *getHintMessage( int inObjectID, int inIndex );

        char *mHintFilterString;
        

        
        // offset from current view center
        doublePair mTutorialHideOffset[NUM_HINT_SHEETS];
        doublePair mTutorialPosOffset[NUM_HINT_SHEETS];
        doublePair mTutorialTargetOffset[NUM_HINT_SHEETS];

        doublePair mTutorialExtraOffset[NUM_HINT_SHEETS];

        // # separates lines
        const char *mTutorialMessage[NUM_HINT_SHEETS];

        char mTutorialFlips[NUM_HINT_SHEETS];

        int mLiveTutorialSheetIndex;
        int mLiveTutorialTriggerNumber;



        // relative to map corner, but not necessary in bounds
        // of locally stored map
        GridPos getMapPos( int inWorldX, int inWorldY );

        // -1 if outside bounds of locally stored map
        int getMapIndex( int inWorldX, int inWorldY );
        

        int mCurrentArrowI;
        float mCurrentArrowHeat;
        
        SimpleVector<OldArrow> mOldArrows;
        
        char *mCurrentDes;
        SimpleVector<char*> mOldDesStrings;
        SimpleVector<float> mOldDesFades;
        

        char *mCurrentLastAteString;
        int mCurrentLastAteFillMax;
        SimpleVector<char*> mOldLastAteStrings;
        SimpleVector<int> mOldLastAteFillMax;
        SimpleVector<float> mOldLastAteFades;
        SimpleVector<float> mOldLastAteBarFades;

        
        void drawHungerMaxFillLine( doublePair inAteWordsPos,
                                    int inMaxFill,
                                    SpriteHandle *inBarSprites,
                                    SpriteHandle *inDashSprites,
                                    char inSkipBar,
                                    char inSkipDashes );
        
        
        int mYumBonus;
        SimpleVector<int> mOldYumBonus;
        SimpleVector<float> mOldYumBonusFades;

        int mYumMultiplier;

        SpriteHandle mYumSlipSprites[ NUM_YUM_SLIPS ];
        int mYumSlipNumberToShow[ NUM_YUM_SLIPS ];
        doublePair mYumSlipHideOffset[ NUM_YUM_SLIPS ];
        doublePair mYumSlipPosOffset[ NUM_YUM_SLIPS ];
        doublePair mYumSlipPosTargetOffset[ NUM_YUM_SLIPS ];
        

        // the object that we're mousing over
        int mLastMouseOverID;
        int mCurMouseOverID;
        float mCurMouseOverFade;
        
        GridPos mCurMouseOverSpot;
        char mCurMouseOverBehind;

        GridPos mCurMouseOverWorld;

        
        char mCurMouseOverPerson;
        char mCurMouseOverSelf;
        

        SimpleVector<GridPos> mPrevMouseOverSpots;
        SimpleVector<char> mPrevMouseOverSpotsBehind;
        SimpleVector<float> mPrevMouseOverSpotFades;
        

        // the ground cell that we're mousing over
        GridPos mCurMouseOverCell;
        float mCurMouseOverCellFade;
        float mCurMouseOverCellFadeRate;
        
        GridPos mLastClickCell;
        

        SimpleVector<GridPos> mPrevMouseOverCells;
        SimpleVector<float> mPrevMouseOverCellFades;
        

        SimpleVector<GridPos> mPrevMouseClickCells;
        SimpleVector<float> mPrevMouseClickCellFades;
        

        float mLastMouseOverFade;

        SpriteHandle mChalkBlotSprite;
        SpriteHandle mPathMarkSprite;
        SpriteHandle mGroundOverlaySprite[4];
        
        SpriteHandle mTeaserArrowLongSprite;
        SpriteHandle mTeaserArrowMedSprite;
        SpriteHandle mTeaserArrowShortSprite;
        SpriteHandle mTeaserArrowVeryShortSprite;
        SpriteHandle mLineSegmentSprite;
        
        
        // not visible, but used for its text filtering
        // capabilities
        TextField mSayField;
        
        double mPageStartTime;

        void computePathToDest( LiveObject *inObject );
        
        double computePathSpeedMod( LiveObject *inObject, int inPathLength );
        
        // check if same floor is present when we take a step in x or y
        char isSameFloor( int inFloor, GridPos inFloorPos, int inDX, int inDY );
        
        // forces next pointerDown call to avoid everything but ground clicks
        char mForceGroundClick;
        

		public: // minitech
        LiveObject *getOurLiveObject();
        LiveObject *getLiveObject( int inID );
        protected: // minitech

        void clearLiveObjects();
        
        // inSpeaker can be NULL
        void drawChalkBackgroundString( doublePair inPos, 
                                        const char *inString,
                                        double inFade,
                                        double inMaxWidth,
                                        LiveObject *inSpeaker = NULL,
                                        int inForceMinChalkBlots = -1,
                                        FloatColor *inForceBlotColor = NULL,
                                        FloatColor *inForceTextColor = NULL );
        
        
        void drawOffScreenSounds();
        


        // returns an animation pack that can be used to draw the
        // held object.  The pack's object ID is -1 if nothing is held
        ObjectAnimPack drawLiveObject( 
            LiveObject *inObj,
            SimpleVector<LiveObject *> *inSpeakers,
            SimpleVector<doublePair> *inSpeakersPos );
        

        void drawMapCell( int inMapI, 
                          int inScreenX, int inScreenY,
                          char inHighlightOnly = false,
                          // blocks frame update for cell and animation sounds
                          char inNoTimeEffects = false );
        
        void checkForPointerHit( PointerHitRecord *inRecord,
                                 float inX, float inY );
        


        void handleOurDeath( char inDisconnect = false );
        

        char *mDeathReason;
        

        double mRemapDelay;
        double mRemapPeak;
        double mRemapDirection;
        double mCurrentRemapFraction;
        
        

        ExtraMapObject copyFromMap( int inMapI );
        
        void putInMap( int inMapI, ExtraMapObject *inObj );
        

        char getCellBlocksWalking( int inMapX, int inMapY );
        
        
        char mShowHighlights;


        void handleAnimSound( int inObjectID, double inAge, 
                              AnimType inType,
                              int inOldFrameCount, int inNewFrameCount,
                              double inPosX, double inPosY );
        

        SimpleVector<GraveInfo> mGraveInfo;

        SimpleVector<OwnerInfo> mOwnerInfo;
        
        void clearOwnerInfo();
    

        // end the move of an extra moving object and stick it back
        // in the map at its destination.
        // inExtraIndex is its index in the mMapExtraMovingObjects vectors
        void endExtraObjectMove( int inExtraIndex );
        

        char mUsingSteam;
        char mZKeyDown;

        
        //FOV
        void changeHUDFOV( float newScale = 1.0f );
        void changeFOV( float newScale = 1.0f );
        void calcOffsetHUD();
        void calcFontScale( float newScale, Font* font );

        char mPlayerInFlight;

        Picker mObjectPicker;

    };



#endif

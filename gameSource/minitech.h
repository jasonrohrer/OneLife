#ifndef minitech_H
#define minitech_H

#include "LivingLifePage.h"
#include <vector>
#include <string>
#include <regex>


class minitech
{

public:

	
	static bool minitechEnabled;
	static float guiScale;
    
    static bool showUncraftables;
    static bool showCommentsAndTagsInObjectDescription;
	
	static float viewWidth;
	static float viewHeight;

	static Font *handwritingFont;
	static Font *mainFont;	
	static Font *tinyHandwritingFont;
	static Font *tinyMainFont;

	typedef struct {
		doublePair posTL;
		doublePair posBR;
		bool mouseHover;
		bool mouseClick;
	} mouseListener;

	static void setLivingLifePage(
		LivingLifePage *inLivingLifePage, 
		SimpleVector<LiveObject> *inGameObjects, 
		int inmMapD,
		int inPathFindingD,
		SimpleVector<int> *inmMapContainedStacks,
		SimpleVector<SimpleVector<int>> *inmMapSubContainedStacks
		);
	static LivingLifePage *livingLifePage;
	static LiveObject *ourLiveObject;
	static SimpleVector<LiveObject> *players;
	static int mMapD;
	static int pathFindingD;
	static int maxObjects;
	static SimpleVector<int> *mMapContainedStacks;
	static SimpleVector<SimpleVector<int>> *mMapSubContainedStacks;
	
	static bool minitechMinimized;
	static unsigned char minimizeKey;
	static int stepCount;
	static float currentX;
	static float currentY;

	static bool posWithinArea(doublePair pos, doublePair areaTL, doublePair areaBR);
	static bool posEqual(doublePair pos1, doublePair pos2);
	static int getDummyParent(int objId);
	static int getDummyLastUse(int objId);
	static bool isCategory(int objId);
	static mouseListener* getMouseListenerByArea(
		std::vector<mouseListener*>* listeners, doublePair posTL, doublePair posBR );
	static GridPos getClosestTile(GridPos src, int objId);	
	static bool isUseDummy(int objId);
	static bool isUseDummyAndNotLastUse(int objId);
	static int getDummyUse(int objId);
	static int compareObjUse(int idA, int idB);
	static bool isProbabilitySet(int objId);
	static float getTransProbability(TransRecord* trans);
    static bool isUncraftable(int objId);
	static unsigned int LevenshteinDistance(const std::string& s1, const std::string& s2);
    static std::vector<std::string> Tokenize( const std::string str, const std::string regpattern );

	
	static int objIdFromXY( int x, int y );
	static std::vector<bool> getObjIsCloseVector();
	static std::string getObjDescriptionComment(int objId);
	static std::string getObjDescriptionTagData( const std::string &objComment, const char *tagName );
	static std::vector<TransRecord*> getUsesTrans(int objId);
	static std::vector<TransRecord*> getProdTrans(int objId);
	
	static void drawPoint(doublePair posCen, std::string color);
	static void drawObj(
		doublePair posCen, 
		int objId, 
		std::string strDescFirstLine = "", 
		std::string strDescSecondLine = "");
	static void drawStr(
		std::string str, 
		doublePair posCen, 
		std::string font, 
		bool withBackground = true, 
		bool avoidOffScreen = false);
	static void drawTileRect( int x, int y, std::string color, bool flashing = false );
	static void drawBox(doublePair posCen, float height, float width, float lineWidth);
	
	static void initOnBirth();
	static void livingLifeStep();
	static bool livingLifeKeyDown(unsigned char inASCII);
	static void livingLifeDraw(float mouseX, float mouseY);
	static bool livingLifePageMouseDown(float mouseX, float mouseY);
	static void clearStep();
	
	static std::vector<TransRecord*> currentHintTrans;
	static int currentTwoTechPage;
	static int useOrMake;
	static int lastUseOrMake;
	static int currentHintObjId;
	static int lastHintObjId;
	static std::string lastHintStr;
    static bool lastHintSearchNoResults;
	static bool changeHintObjOnTouch;
	static std::vector<mouseListener*> twotechMouseListeners;
	static mouseListener* prevListener;
	static mouseListener* nextListener;
	static std::vector<TransRecord*> sortUsesTrans(std::vector<TransRecord*> unsortedTrans);
	static std::vector<TransRecord*> sortProdTrans(std::vector<TransRecord*> unsortedTrans);
	static void updateDrawTwoTech();
	static void inputHintStrToSearch(std::string hintStr);
    static void changeCurrentHintObjId(int objID);

	

	
};


#endif

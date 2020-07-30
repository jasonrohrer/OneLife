#ifndef minitech_H
#define minitech_H

#include "LivingLifePage.h"
#include <vector>
#include <string>


using namespace std;

class minitech
{

public:

	
	static bool minitechEnabled;
	static float guiScale;
	
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
	static int stepCount;
	static float currentX;
	static float currentY;

	static bool posWithinArea(doublePair pos, doublePair areaTL, doublePair areaBR);
	static bool posEqual(doublePair pos1, doublePair pos2);
	static int getDummyParent(int objId);
	static bool isCategory(int objId);
	static mouseListener* getMouseListenerByArea(
		vector<mouseListener*>* listeners, doublePair posTL, doublePair posBR );
	static GridPos getClosestTile(GridPos src, int objId);	
	
	static int objIdFromXY( int x, int y );
	static vector<bool> getObjIsCloseVector();
	static vector<TransRecord*> getUsesTrans(int objId);
	static vector<TransRecord*> getProdTrans(int objId);
	
	static void drawPoint(doublePair posCen, string color);
	static void drawObj(
		doublePair posCen, 
		int objId, 
		string strDescFirstLine = "", 
		string strDescSecondLine = "");
	static void drawStr(
		string str, 
		doublePair posCen, 
		string font, 
		bool withBackground = true, 
		bool avoidOffScreen = false);
	static void drawTileRect( int x, int y, string color, bool flashing = false );
	
	static void initOnBirth();
	static void livingLifeStep();
	static bool livingLifeKeyDown(unsigned char inASCII);
	static void livingLifeDraw(float mouseX, float mouseY);
	static bool livingLifePageMouseDown(float mouseX, float mouseY);
	
	static vector<TransRecord*> currentHintTrans;
	static int currentTwoTechPage;
	static int useOrMake;
	static int lastUseOrMake;
	static int currentHintObjId;
	static int lastHintObjId;
	static vector<mouseListener*> twotechMouseListeners;
	static mouseListener* prevListener;
	static mouseListener* nextListener;
	static vector<TransRecord*> sortUsesTrans(vector<TransRecord*> unsortedTrans);
	static vector<TransRecord*> sortProdTrans(vector<TransRecord*> unsortedTrans);
	static void updateDrawTwoTech();

	

	
};


#endif

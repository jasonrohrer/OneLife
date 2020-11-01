

#include <vector>
#include <string>
#include <numeric> //iota
#include <cmath> //pow and sqrt
#include <algorithm> //vector sort

#include "LivingLifePage.h"
#include "groundSprites.h"
#include "objectBank.h"
#include "categoryBank.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"


#include "minitech.h"

using namespace std;

bool minitech::minitechEnabled = true;
float minitech::guiScale = 1.0f;

float minitech::viewWidth = 1280.0;
float minitech::viewHeight = 720.0;

Font *minitech::handwritingFont;
Font *minitech::mainFont;
Font *minitech::tinyHandwritingFont;
Font *minitech::tinyMainFont;

LivingLifePage *minitech::livingLifePage;
LiveObject *minitech::ourLiveObject;
SimpleVector<LiveObject> *minitech::players;
int minitech::mMapD;
int minitech::pathFindingD;
int minitech::maxObjects;
SimpleVector<int> *minitech::mMapContainedStacks;
SimpleVector<SimpleVector<int>> *minitech::mMapSubContainedStacks;

bool minitech::minitechMinimized = true;
unsigned char minitech::minimizeKey;
int minitech::stepCount;
float minitech::currentX;
float minitech::currentY;

vector<TransRecord*> minitech::currentHintTrans;
int minitech::currentTwoTechPage;
int minitech::useOrMake;
int minitech::lastUseOrMake;
int minitech::currentHintObjId;
int minitech::lastHintObjId;
string minitech::lastHintStr;
bool minitech::changeHintObjOnTouch;
vector<minitech::mouseListener*> minitech::twotechMouseListeners;
minitech::mouseListener* minitech::prevListener;
minitech::mouseListener* minitech::nextListener;

const char *biomeNames[] = {"GRASSLANDS",
							"SWAMP",
							"YELLOW PRAIRIES",
							"BADLANDS",
							"TUNDRA",
							"DESERT",
							"JUNGLE",
							"OCEAN"};



void minitech::setLivingLifePage(
	LivingLifePage *inLivingLifePage, 
	SimpleVector<LiveObject> *inGameObjects, 
	int inmMapD, 
	int inPathFindingD,
	SimpleVector<int> *inmMapContainedStacks,
	SimpleVector<SimpleVector<int>> *inmMapSubContainedStacks
	) {
	
	maxObjects = getMaxObjectID() + 1;
	livingLifePage = inLivingLifePage;
	players = inGameObjects;
	mMapD = inmMapD;
	pathFindingD = inPathFindingD;
	mMapContainedStacks = inmMapContainedStacks;
	mMapSubContainedStacks = inmMapSubContainedStacks;
	
	minitechEnabled = SettingsManager::getIntSetting( "useMinitech", 1 );
	char *minimizeKeyFromSetting = SettingsManager::getStringSetting("minitechMinimizeKey", "v");
	minimizeKey = minimizeKeyFromSetting[0];
}

void minitech::initOnBirth() { 
	
	for (auto p: twotechMouseListeners) {
		delete(p);
	}
	twotechMouseListeners.clear();
	twotechMouseListeners.shrink_to_fit();
	
	currentHintTrans.clear();
	currentHintTrans.shrink_to_fit();
	changeHintObjOnTouch = true;
	
	if (prevListener != NULL) delete(prevListener);
	if (nextListener != NULL) delete(nextListener);
	prevListener = NULL;
	nextListener = NULL;
	
	if (handwritingFont != NULL) delete handwritingFont;
	if (mainFont != NULL) delete mainFont;
	if (tinyHandwritingFont != NULL) delete tinyHandwritingFont;
	if (tinyMainFont != NULL) delete tinyMainFont;
	handwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16*guiScale );
	handwritingFont->setMinimumPositionPrecision( 1 );
	mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16*guiScale );
	mainFont->setMinimumPositionPrecision( 1 );	
	tinyHandwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16/2*guiScale );
	tinyHandwritingFont->setMinimumPositionPrecision( 1 );
	tinyMainFont = new Font( getFontTGAFileName(), 6, 16, false, 16/2*guiScale );
	tinyMainFont->setMinimumPositionPrecision( 1 );

}



bool minitech::posWithinArea(doublePair pos, doublePair areaTL, doublePair areaBR) {
	return 
		pos.x >= areaTL.x &&
		pos.x <= areaBR.x &&
		pos.y <= areaTL.y &&
		pos.y >= areaBR.y;
}

bool minitech::posEqual(doublePair pos1, doublePair pos2) {
	return 
		pos1.x == pos2.x &&
		pos1.y == pos2.y;
}

int minitech::getDummyParent(int objId) {
	if (objId <= 0 || objId >= maxObjects) return objId;
	ObjectRecord* o = getObject(objId);
	if (o != NULL) {
		if (o->isUseDummy) return o->useDummyParent;
	}
	return objId;
}

int minitech::getDummyLastUse(int objId) {
	if (objId <= 0 || objId >= maxObjects) return objId;
	ObjectRecord* o = getObject(objId);
	if (o != NULL) {
		int parentID = o->id;
		if (o->isUseDummy) {
			parentID = o->useDummyParent;
		}
		ObjectRecord* parent = getObject(parentID);
		if (parent->numUses > 1) {
			return parent->useDummyIDs[0];
		}
	}
	return objId;
}

bool minitech::isCategory(int objId) {
	if (objId <= 0) return false;
	CategoryRecord *c = getCategory( objId );
	if (c == NULL) return false;
    if( !c->isPattern && c->objectIDSet.size() > 0 ) return true;
    return false;
}

minitech::mouseListener* minitech::getMouseListenerByArea( 
	vector<mouseListener*>* listeners, doublePair posTL, doublePair posBR ) {
	for (int i=0; i<listeners->size(); i++) {
		if (
			posEqual( (*listeners)[i]->posTL, posTL) &&
			posEqual( (*listeners)[i]->posBR, posBR)
			) {
			return (*listeners)[i];
		}
	}
	mouseListener* listener = new mouseListener();
	listener->posTL = posTL;
	listener->posBR = posBR;
	listener->mouseHover = false;
	listeners->push_back(listener);
	return listener;
}

GridPos minitech::getClosestTile(GridPos src, int objId) {
	
	objId = getDummyParent(objId);
	
	int *mMap = livingLifePage->mMap;
		
	int mMapOffsetX = livingLifePage->mMapOffsetX;
	int mMapOffsetY = livingLifePage->mMapOffsetY;
	
	int pathOffsetX = pathFindingD/2 - currentX;
	int pathOffsetY = pathFindingD/2 - currentY;
	
	float foundBestDist = 9999;
	float foundBestX, foundBestY;
	
	bool tileFound = false;

	for( int y=0; y<pathFindingD; y++ ) {
		int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
		int mapY_abs = mapY + mMapOffsetY - mMapD / 2;
		
		for( int x=0; x<pathFindingD; x++ ) {
			int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
			int mapX_abs = mapX + mMapOffsetX - mMapD / 2;
			
			if( mapY >= 0 && mapY < mMapD &&
				mapX >= 0 && mapX < mMapD ) { 

				bool foundInThisTile = false;

				int mapI = mapY * mMapD + mapX;
				int id = mMap[mapI];
				id = getDummyParent(id);

				if (id == objId) {
					float foundDist = sqrt(pow(src.y - mapY_abs, 2) + pow(src.x - mapX_abs, 2));
					if (foundDist < foundBestDist) {
						tileFound = true;
						foundInThisTile = true;
						foundBestX = mapX_abs;
						foundBestY = mapY_abs;
						foundBestDist = foundDist;
						continue;
					}
				}
				
				if (mMapContainedStacks[mapI].size() > 0) {
					for (int i=0; i < mMapContainedStacks[mapI].size(); i++) {
						id = mMapContainedStacks[mapI].getElementDirect(i);
						id = getDummyParent(id);
						if (id == objId) {
							float foundDist = sqrt(pow(src.y - mapY_abs, 2) + pow(src.x - mapX_abs, 2));
							if (foundDist < foundBestDist) {
								tileFound = true;
								foundInThisTile = true;
								foundBestX = mapX_abs;
								foundBestY = mapY_abs;
								foundBestDist = foundDist;
							}
						}
					}
					if (foundInThisTile) continue;
				}
				
				if (mMapSubContainedStacks[mapI].size() > 0) {
					for (int i=0; i < mMapSubContainedStacks[mapI].size(); i++) {
						SimpleVector<int> subContainedStack = mMapSubContainedStacks[mapI].getElementDirect(i);
						for (int k=0; k < subContainedStack.size(); k++) {
							id = subContainedStack.getElementDirect(k);
							id = getDummyParent(id);
							if (id == objId) {
								float foundDist = sqrt(pow(src.y - mapY_abs, 2) + pow(src.x - mapX_abs, 2));
								if (foundDist < foundBestDist) {
									tileFound = true;
									foundInThisTile = true;
									foundBestX = mapX_abs;
									foundBestY = mapY_abs;
									foundBestDist = foundDist;
								}
							}
						}
					}
					if (foundInThisTile) continue;
				}
				
			}
		}
	}
	
	GridPos foundPos = {9999, 9999};
	if (tileFound) {
		foundPos = {foundBestX, foundBestY};
	}
	return foundPos;
}

bool minitech::isUseDummy(int objId) {
	if (objId <= 0) return false;
	ObjectRecord* o = getObject(objId);
	if (o == NULL) return false;
	return o->isUseDummy;
}

bool minitech::isUseDummyAndNotLastUse(int objId) {
	if (objId <= 0) return false;
	ObjectRecord* o = getObject(objId);
	if (o == NULL) return false;
	if (o->isUseDummy) {
		if (o->thisUseDummyIndex != 0) {
			return true;
		}
	}
	return false;
}

int minitech::getDummyUse(int objId) {
	//return -1 if not applicable
	//return 0 if it is the parent object
	//then 1 is the last use, and 2 is #use 2 etc.
	if (objId <= 0) return -1;
	ObjectRecord* o = getObject(objId);
	if (o == NULL) return -1;
	if (o->numUses > 1) return 0;
	if (o->isUseDummy) {
		return o->thisUseDummyIndex + 1;
	}
	return -1;
}

int minitech::compareObjUse(int idA, int idB) {
	//return -1 if a and b have the same parent and a's use > b's
	//return 1 if a and b have the same parent and a's use < b's
	//return 0 otherwise
	
	if (idA <= 0 || idB <= 0) return 0;
	
	ObjectRecord* a = getObject(idA);
	ObjectRecord* b = getObject(idB);
	
	if (a == NULL || b == NULL) return 0;
	
	int aParentId = 0;
	int bParentId = 0;
	if (a->numUses > 1) aParentId = a->id;
	if (b->numUses > 1) bParentId = b->id;
	if (a->isUseDummy) aParentId = a->useDummyParent;
	if (b->isUseDummy) bParentId = b->useDummyParent;
	
	if (aParentId == 0 || bParentId == 0) return 0;
	if (aParentId != bParentId) return 0;
	
	int aUse = 0;
	int bUse = 0;
	if (aParentId == a->id) aUse = a->numUses;
	if (bParentId == b->id) bUse = b->numUses;
	if (a->isUseDummy) aUse = a->thisUseDummyIndex + 1;
	if (b->isUseDummy) bUse = b->thisUseDummyIndex + 1;
	
	if (aUse > bUse) return -1;
	if (aUse < bUse) return 1;
	return 0;
}

bool minitech::isProbabilitySet(int objId) {
	if (objId <= 0) return false;
	CategoryRecord *c = getCategory( objId );
	if (c == NULL) return false;
    if( c->isProbabilitySet ) return true;
    return false;
}

float minitech::getTransProbability(TransRecord* trans) {
	if (trans == NULL) return -1.0;
	int idA = trans->actor;
	int idB = trans->target;
	int idC = trans->newActor;
	int idD = trans->newTarget;
	
	TransRecord* t = getTrans( idA, idB, trans->lastUseActor, trans->lastUseTarget );
	if (t == NULL) return -1.0;
	int origIdC = t->newActor;
	int origIdD = t->newTarget;
	
	int cOrD = -1;
	if ( isProbabilitySet(origIdC) ) cOrD = 0;
	if ( isProbabilitySet(origIdD) ) cOrD = 1;
	if (cOrD != -1) {
		CategoryRecord* c;
		if (cOrD == 0) c = getCategory( origIdC );
		if (cOrD == 1) c = getCategory( origIdD );
		SimpleVector<int> idSet = c->objectIDSet;
		SimpleVector<float> wSet = c->objectWeights;
		for (int i=0; i<idSet.size(); i++) {
			TransRecord* staticTrans = new TransRecord;
			*staticTrans = *t;
			int newId = idSet.getElementDirect(i);
			if (cOrD == 0) staticTrans->newActor = newId;
			if (cOrD == 1) staticTrans->newTarget = newId;
			
			if ( staticTrans->newActor == idC && staticTrans->newTarget == idD ) return wSet.getElementDirect(i);
		}
	}
	return -1.0;
}

int minitech::objIdFromXY( int x, int y ) {
	int mMapOffsetX = livingLifePage->mMapOffsetX;
	int mMapOffsetY = livingLifePage->mMapOffsetY;
	int *mMap = livingLifePage->mMap;
	int mapX = x - mMapOffsetX + mMapD / 2;
	int mapY = y - mMapOffsetY + mMapD / 2;
	return mMap[ mapY * mMapD + mapX ];
}

vector<bool> minitech::getObjIsCloseVector() {
	
	vector<bool> objIsClose(maxObjects, false);
	
	int *mMap = livingLifePage->mMap;
		
	int mMapOffsetX = livingLifePage->mMapOffsetX;
	int mMapOffsetY = livingLifePage->mMapOffsetY;
	
	int pathOffsetX = pathFindingD/2 - currentX;
	int pathOffsetY = pathFindingD/2 - currentY;
	
	for( int y=0; y<pathFindingD; y++ ) {
		int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
		int mapY_abs = mapY + mMapOffsetY - mMapD / 2;
		
		for( int x=0; x<pathFindingD; x++ ) {
			int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
			int mapX_abs = mapX + mMapOffsetX - mMapD / 2;
			
			if( mapY >= 0 && mapY < mMapD &&
				mapX >= 0 && mapX < mMapD ) { 
				
				bool foundInThisTile = false;
				
				int mapI = mapY * mMapD + mapX;
				int id = mMap[mapI];
				
				if ( ! (!id || id <= 0 || id >= maxObjects) ) {
					objIsClose[id] = true;
					objIsClose[getDummyParent(id)] = true;
					foundInThisTile = true;
				}
				
				if (mMapContainedStacks[mapI].size() > 0) {
					for (int i=0; i < mMapContainedStacks[mapI].size(); i++) {
						id = mMapContainedStacks[mapI].getElementDirect(i);
						objIsClose[id] = true;
						objIsClose[getDummyParent(id)] = true;
						foundInThisTile = true;
					}
					if (foundInThisTile) continue;
				}
				
				if (mMapSubContainedStacks[mapI].size() > 0) {
					for (int i=0; i < mMapSubContainedStacks[mapI].size(); i++) {
						SimpleVector<int> subContainedStack = mMapSubContainedStacks[mapI].getElementDirect(i);
						for (int k=0; k < subContainedStack.size(); k++) {
							id = subContainedStack.getElementDirect(k);
							objIsClose[id] = true;
							objIsClose[getDummyParent(id)] = true;
							foundInThisTile = true;
						}
					}
					if (foundInThisTile) continue;
				}
				
			}
		}
	}
	return objIsClose;
}

unsigned int minitech::LevenshteinDistance(const std::string& s1, const std::string& s2) {
	const std::size_t len1 = s1.size(), len2 = s2.size();
	std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

	d[0][0] = 0;
	for(unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
	for(unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

	for(unsigned int i = 1; i <= len1; ++i)
		for(unsigned int j = 1; j <= len2; ++j)
                      // note that std::min({arg1, arg2, arg3}) works only in C++11,
                      // for C++98 use std::min(std::min(arg1, arg2), arg3)
                      d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1) });
	return d[len1][len2];
}



void minitech::drawPoint(doublePair posCen, string color) {
	float pointSize = 3 * guiScale;
	
	if (color == "red") setDrawColor( 1, 0, 0, 1 );
	if (color == "green") setDrawColor( 0, 1, 0, 1 );
	if (color == "blue") setDrawColor( 0, 0, 1, 1 );
	
	drawRect( posCen, pointSize, pointSize );	
}

void minitech::drawObj(doublePair posCen, int objId, string strDescFirstLine, string strDescSecondLine) {
	ObjectRecord* obj = getObject(objId);
	if (objId <= 0) {
		string firstPart;
		string secondPart;
		if (strDescFirstLine != "" && strDescSecondLine != "" ) {
			firstPart = strDescFirstLine;
			secondPart = strDescSecondLine;
		} else {
			firstPart = "EMPTY";
			secondPart = "HAND";
		}
		doublePair firstLine = posCen;
		int tinyLineHeight = 12;
		firstLine.y += tinyLineHeight/2*guiScale;
		drawStr(firstPart, firstLine, "tinyHandwritten", false);
		firstLine.y -= tinyLineHeight*guiScale;
		drawStr(secondPart, firstLine, "tinyHandwritten", false);
		return;
	}
	if (obj == NULL) return;
	int maxD = getMaxDiameter( obj );
	double zoom = 1;
	float maxSize = 76.0;
	if( maxD > maxSize ) zoom = maxSize / maxD;
	zoom = zoom * guiScale;
	doublePair posAfterOffset = sub (posCen, mult( getObjectCenterOffset( obj ), zoom ));
	drawObject( obj, 2, posAfterOffset, 0, false, false, 20, 0, false, false,
				getEmptyClothingSet(), zoom );
}

void minitech::drawStr(
	string str, doublePair posCen, string font, 
	bool withBackground, bool avoidOffScreen) {
	
	doublePair screenCenter = livingLifePage->minitechGetLastScreenViewCenter();
	
	char sBuf[64];
	sprintf( sBuf, str.c_str() );
	float textWidth = 0;
	if (font == "handwritten") {
		textWidth = handwritingFont->measureString( sBuf );
	} else if (font == "main") {
		textWidth = mainFont->measureString( sBuf );
	} else if (font == "tinyHandwritten") {
		textWidth = tinyHandwritingFont->measureString( sBuf );
	} else if (font == "tinyMain") {
		textWidth = tinyMainFont->measureString( sBuf );
	}
	
	float lineHeight = 24*guiScale;
	float padding = 12*guiScale;
	
	if (font == "tinyHandwritten" || font == "tinyMain") {
		lineHeight = lineHeight * 0.5;
		padding = padding * 0.5;
	}
	
	float recWidth = textWidth + padding*2;
	float recHeight = 1 * lineHeight + padding*2;
	
	doublePair posLT = {posCen.x - recWidth/2, posCen.y + recHeight/2};	
	
	if (avoidOffScreen) {
		doublePair offset = {0.0, 0.0};
		if (posLT.x + recWidth > screenCenter.x + viewWidth/2) {
			offset.x = screenCenter.x + viewWidth/2 - recWidth - posLT.x;
		} else if (posLT.x < screenCenter.x - viewWidth/2) {
			offset.x = - posLT.x + screenCenter.x - viewWidth/2;
		}
		if (posLT.y > screenCenter.y + viewHeight/2) {
			offset.y = screenCenter.y + viewHeight/2 - posLT.y;
		} else if (posLT.y - recHeight < screenCenter.y - viewHeight/2) {
			offset.y = recHeight - posLT.y + screenCenter.y - viewHeight/2;
		}
		posLT = add(posLT, offset);
		posCen = add(posCen, offset);
	}
	
	doublePair textPos = {posLT.x + padding, posCen.y};
	
	if (withBackground) {
		setDrawColor( 0, 0, 0, 0.8 );
		drawRect( posCen, recWidth/2, recHeight/2);
	}
	
	setDrawColor( 1, 1, 1, 1 );
	if (font == "handwritten") {
		handwritingFont->drawString( sBuf, textPos, alignLeft );
	} else if (font == "main") {
		mainFont->drawString( sBuf, textPos, alignLeft );
	} else if (font == "tinyHandwritten") {
		tinyHandwritingFont->drawString( sBuf, textPos, alignLeft );
	} else if (font == "tinyMain") {
		tinyMainFont->drawString( sBuf, textPos, alignLeft );
	}
}

void minitech::drawTileRect( int x, int y, string color, bool flashing ) {
	doublePair startPos = { (double)x, (double)y };
	startPos.x *= CELL_D;
	startPos.y *= CELL_D;
	float maxAlpha = 0.5;
	float alpha;
	if (flashing) {
		alpha = (stepCount % 40) / 40.0;
		if (alpha > 0.5) alpha = 1 - alpha;
		alpha *= maxAlpha;
	} else {
		alpha = maxAlpha;
	}
	if (color == "red") setDrawColor( 1, 0, 0, alpha );
	if (color == "green") setDrawColor( 0, 1, 0, alpha );
	if (color == "blue") setDrawColor( 0, 0, 1, alpha );
	drawRect( startPos, CELL_D/2, CELL_D/2 );
}

void minitech::drawBox(doublePair posCen, float height, float width, float lineWidth) {
	doublePair posCenTopSide = {posCen.x, posCen.y + ( height / 2 - lineWidth / 2 )};
	doublePair posCenBottomSide = {posCen.x, posCen.y - ( height / 2 - lineWidth / 2 )};
	doublePair posCenRightSide = {posCen.x + ( width / 2 - lineWidth / 2 ), posCen.y};
	doublePair posCenLeftSide = {posCen.x - ( width / 2 - lineWidth / 2 ), posCen.y};
	
	drawRect( posCenTopSide, width/2 - lineWidth, lineWidth/2 );
	drawRect( posCenBottomSide, width/2 - lineWidth, lineWidth/2 );
	drawRect( posCenRightSide, lineWidth/2, height/2 );
	drawRect( posCenLeftSide, lineWidth/2, height/2 );
}



vector<TransRecord*> minitech::getUsesTrans(int objId) {
	
	SimpleVector<TransRecord*> *usesTrans = getAllUses( objId );
	vector<TransRecord*> results;
	
	int numTrans = 0;
	if( usesTrans != NULL ) {
		numTrans = usesTrans->size();
	}
	if( numTrans == 0 ) {
		return results; 
	}

	for( int t=0; t<numTrans; t++ ) {
		
		TransRecord *trans = usesTrans->getElementDirect( t );
		
		int idA = trans->actor;
		int idB = trans->target;
		int idC = trans->newActor;
		int idD = trans->newTarget;
		
		int cOrD = -1;
		if ( isProbabilitySet(idC) ) cOrD = 0;
		if ( isProbabilitySet(idD) ) cOrD = 1;
		if (cOrD != -1) {
			CategoryRecord* c;
			if (cOrD == 0) c = getCategory( idC );
			if (cOrD == 1) c = getCategory( idD );
			SimpleVector<int> idSet = c->objectIDSet;
			for (int i=0; i<idSet.size(); i++) {
				TransRecord* staticTrans = new TransRecord;
				*staticTrans = *trans;
				int newId = idSet.getElementDirect(i);
				if (cOrD == 0) staticTrans->newActor = newId;
				if (cOrD == 1) staticTrans->newTarget = newId;
				results.push_back(staticTrans);
			}
			continue;
		}
		
		if ( isUseDummyAndNotLastUse(idA) && isUseDummyAndNotLastUse(idC) ) continue;
		if ( isUseDummyAndNotLastUse(idB) && isUseDummyAndNotLastUse(idD) ) continue;
		if ( isCategory(idA) || isCategory(idB) || isCategory(idC) || isCategory(idD) ) continue;
		if ( trans->lastUseActor || trans->lastUseTarget ) continue;
		
		results.push_back(trans);

	}
	
	return results;
}

vector<TransRecord*> minitech::getProdTrans(int objId) {
	
	SimpleVector<TransRecord*> *prodTrans = getAllProduces( objId );
	vector<TransRecord*> results;
	
	int numTrans = 0;
	if( prodTrans != NULL ) {
		numTrans = prodTrans->size();
	}

	for( int t=0; t<numTrans; t++ ) {
		
		TransRecord *trans = prodTrans->getElementDirect( t );
		
		int idA = trans->actor;
		int idB = trans->target;
		int idC = trans->newActor;
		int idD = trans->newTarget;
		
		if ( idA == objId || idB == objId ) continue;
		if ( isUseDummyAndNotLastUse(idA) && isUseDummyAndNotLastUse(idC) ) continue;
		if ( isUseDummyAndNotLastUse(idB) && isUseDummyAndNotLastUse(idD) ) continue;
		if ( isCategory(idA) || isCategory(idB) || isCategory(idC) || isCategory(idD) ) continue;
		if ( trans->lastUseActor || trans->lastUseTarget ) continue;
		
		results.push_back(trans);

	}
	
	int numCategoriesForObject = getNumCategoriesForObject( objId );
	for (int i=0; i<numCategoriesForObject; i++) {
		int cId = getCategoryForObject(objId, i);
		CategoryRecord* c = getCategory( cId );
		
		if (c->isProbabilitySet) {
			SimpleVector<TransRecord*> *prodPTrans = getAllProduces( cId );
			int numPTrans = 0;
			if( prodPTrans != NULL ) {
				numPTrans = prodPTrans->size();
			}
			for( int t=0; t<numPTrans; t++ ) {
				TransRecord* staticTrans = new TransRecord;
				*staticTrans = *(prodPTrans->getElementDirect( t ));
				
				if (staticTrans->actor == cId) staticTrans->actor = objId;
				if (staticTrans->target == cId) staticTrans->target = objId;
				if (staticTrans->newActor == cId) staticTrans->newActor = objId;
				if (staticTrans->newTarget == cId) staticTrans->newTarget = objId;
				
				results.push_back(staticTrans);
			}
		}
	}
	
	return results;
}

vector<TransRecord*> minitech::sortUsesTrans(vector<TransRecord*> unsortedTrans) {
	
	vector<bool> boolCloseVect = getObjIsCloseVector();
	vector<float> rankScores(unsortedTrans.size(), 0);
	
	for ( int i=0; i<unsortedTrans.size(); i++ ) {
		TransRecord *trans = unsortedTrans[i];
		
		int idA = trans->actor;
		int idB = trans->target;
		int idC = trans->newActor;
		int idD = trans->newTarget;
		int holdingID = getDummyParent(ourLiveObject->holdingID);
		
		GridPos currentPos = {currentX, currentY};
		
		float punishmentScore = 9999.0;
		
		//idea behind the sorting:
		//- show transition involving ingredients (actor and target) which are nearby first, sort these by distance
		//- sort the far-away transitions by the object depth of the ingredients

		if (idA == ourLiveObject->holdingID || idA <= 0) {
			rankScores[i] += 0;
		} else if (boolCloseVect[idA]) {
			GridPos pos = getClosestTile(currentPos, idA);
			float dist = sqrt(pow(currentX - pos.x, 2) + pow(currentY - pos.y, 2));
			rankScores[i] += dist;
		} else {
			//+1 to make sure transitions with 0 depth, naturally spawning ingredients won't show up on top
			rankScores[i] += punishmentScore * (getObjectDepth(idA) + 1); 
		}

		if (idB == ourLiveObject->holdingID || idB <= 0) {
				  
			rankScores[i] += 0;
		} else if (boolCloseVect[idB]) {
			GridPos pos = getClosestTile(currentPos, idB);
			float dist = sqrt(pow(currentX - pos.x, 2) + pow(currentY - pos.y, 2));
			rankScores[i] += dist;
		} else {
			rankScores[i] += punishmentScore * (getObjectDepth(idB) + 1);
	
		}

		//dont always show the transition to stack the object we are currently holding
		if ( idA == idB && idA == ourLiveObject->holdingID ) {
			rankScores[i] += punishmentScore * (getObjectDepth(idA) + 1);
		}
		
	}
	
	vector<std::size_t> index(unsortedTrans.size());
	iota(index.begin(), index.end(), 0);
	sort(index.begin(), index.end(), [&](size_t a, size_t b) { return rankScores[a] < rankScores[b]; });
	
	vector<TransRecord*> temp(unsortedTrans.size());
	for ( int i=0; i<unsortedTrans.size(); i++ ) {
		temp[i] = unsortedTrans[index[i]];
	}
	return temp;
}

vector<TransRecord*> minitech::sortProdTrans(vector<TransRecord*> unsortedTrans) {
	
	vector<bool> boolCloseVect = getObjIsCloseVector();
	vector<float> rankScores(unsortedTrans.size(), 0);
	
	for ( int i=0; i<unsortedTrans.size(); i++ ) {
		TransRecord *trans = unsortedTrans[i];
		
		int idA = trans->actor;
		int idB = trans->target;
		int idC = trans->newActor;
		int idD = trans->newTarget;
		int holdingID = getDummyParent(ourLiveObject->holdingID);
		
		GridPos currentPos = {currentX, currentY};
		
		float punishmentScore = 32.0;
		
		//idea behind the sorting:
		//- show craft-from-scratch transition on the very top
		//- show transition involving ingredients (actor and target) which are nearby first, sort these by distance
		//- sort the far-away transitions by the object depth of the ingredients
		
		if (idA == ourLiveObject->holdingID || idA <= 0) {
			rankScores[i] += 0;
		} else if (boolCloseVect[idA]) {
			GridPos pos = getClosestTile(currentPos, idA);
			float dist = sqrt(pow(currentX - pos.x, 2) + pow(currentY - pos.y, 2));
			rankScores[i] += dist;
			
		} else {
			rankScores[i] += punishmentScore * (getObjectDepth(idA) + 1);
	
		}

		if (idB == ourLiveObject->holdingID || idB <= 0) {
			rankScores[i] += 0;
	
		} else if (boolCloseVect[idB]) {
			GridPos pos = getClosestTile(currentPos, idB);
			float dist = sqrt(pow(currentX - pos.x, 2) + pow(currentY - pos.y, 2));
			rankScores[i] += dist;
			
		} else {
			rankScores[i] += punishmentScore * (getObjectDepth(idB) + 1);
	
		}
															 
																																			 
																		   
		
		//For craft-from-scratch transition, object depth of both ingredience would be shallower than the product object
		int currDepth = getObjectDepth(currentHintObjId);
		if (
			(idA <= 0 || getObjectDepth(idA) < currDepth) &&
			(idB <= 0 || getObjectDepth(idB) < currDepth)
			) {
			rankScores[i] = 0;
		}

	}
	
	vector<std::size_t> index(unsortedTrans.size());
	iota(index.begin(), index.end(), 0);
	sort(index.begin(), index.end(), [&](size_t a, size_t b) { return rankScores[a] < rankScores[b]; });
	
	vector<TransRecord*> temp(unsortedTrans.size());
	for ( int i=0; i<unsortedTrans.size(); i++ ) {
		temp[i] = unsortedTrans[index[i]];
	}
	return temp;
}

void minitech::updateDrawTwoTech() {
	
	int defaultNumOfLines = 5;
	float iconSize = 76.0/2 *guiScale;
	float paddingX = 12.0 *guiScale;
	float paddingY = 12.0 *guiScale;
	float lineSpacing = 16.0 *guiScale; //spacing between lines of transition
	float lineHeight = 24.0 *guiScale; //line height of plus and equal sign, handwriting height = 24
	
	float separatorHeight = 12.0 *guiScale; //space between header rect and main rect
	float centerXSeparation = 32.0 *guiScale; //space between how-to buttons and item icon in header
	float tinyLineHeight = 12.0 *guiScale; //line height of how-to buttons, tinyHandwriting height = 12
	
	float contentOffsetY = iconSize/4; //for extra tall objects
	float iconCaptionYOffset = - iconSize/2;
	float buttonHeight = iconSize;
	
	doublePair screenPos = livingLifePage->minitechGetLastScreenViewCenter();
	// drawStr(to_string(guiScale), screenPos, "tinyHandwritten", true, true);
	
	float recWidth;
	float recHeight;
	
	doublePair posLT = livingLifePage->minitechGetLastScreenViewCenter();
	posLT.x = posLT.x + viewWidth/2;
	posLT.y = posLT.y - viewHeight/2;
	
	if (minitechMinimized) {
		
		recWidth = paddingX + 7*iconSize + paddingX;
		recHeight = paddingY/2 + lineHeight/2 + paddingY/2;
		posLT.y = posLT.y + recHeight + (60); //panel height = 60
		posLT.x = posLT.x - recWidth;
		doublePair posCenter = {posLT.x + recWidth / 2, posLT.y - recHeight / 2};
		doublePair posBR = {posLT.x + recWidth, posLT.y - recHeight};
		setDrawColor( 0, 0, 0, 0.8 );
		drawRect( posCenter, recWidth/2, recHeight/2);
		
		drawStr("[+] CRAFTING GUIDE", posCenter, "tinyHandwritten", false);
		mouseListener* maxListener = getMouseListenerByArea(
			&twotechMouseListeners, sub(posLT, screenPos), sub(posBR, screenPos));
		if (maxListener->mouseHover) {
			setDrawColor( 1, 1, 1, 0.3 );
			drawRect( posCenter, recWidth/2, recHeight/2);
		}
		if (maxListener->mouseClick) {
			minitechMinimized = false;
			maxListener->mouseClick = false;
		}
		return;
		
	}
	
	int transSize = currentHintTrans.size();
		
	if (transSize == 0) {
		
		recWidth = paddingX + 7*iconSize + paddingX;
		recHeight = paddingY + 1*iconSize + paddingY;
		posLT.y = posLT.y + recHeight + (60); //panel height = 60
		posLT.x = posLT.x - recWidth;
		doublePair posCenter = {posLT.x + recWidth / 2, posLT.y - recHeight / 2};
		setDrawColor( 0, 0, 0, 0.8 );
		drawRect( posCenter, recWidth/2, recHeight/2);
		drawStr("NO RECIPES FOUND :)", posCenter, "tinyHandwritten", false);
		
	} else {
		
		int maxPage = int( ceil( float(transSize) / float(defaultNumOfLines) ) );
		if (currentTwoTechPage < 0) currentTwoTechPage = maxPage - 1;
		if ( currentTwoTechPage >= maxPage ) currentTwoTechPage = 0;
		bool showNextPageButton = transSize > (currentTwoTechPage+1)*defaultNumOfLines;
		bool showPreviousPageButton = currentTwoTechPage > 0;
		
		int startIndex = currentTwoTechPage*defaultNumOfLines;
		int endIndex = min(transSize, (currentTwoTechPage+1)*defaultNumOfLines);
		vector<TransRecord*> transToShow(currentHintTrans.begin() + startIndex, currentHintTrans.begin() + endIndex);
		
		int numOfLines = endIndex - startIndex;
		
		if (!showPreviousPageButton && !showNextPageButton) buttonHeight = 0;
		
		recWidth = paddingX + 7*iconSize + paddingX;
		recHeight = paddingY + (numOfLines-1)*lineSpacing + numOfLines*iconSize + buttonHeight + paddingY;
		
		posLT.y = posLT.y + recHeight + (60); //panel height = 60
		posLT.x = posLT.x - recWidth;
		
		doublePair posCenter = {posLT.x + recWidth / 2, posLT.y - recHeight / 2};
		setDrawColor( 0, 0, 0, 0.8 );
		drawRect( posCenter, recWidth/2, recHeight/2);
		
		doublePair posLineLCen = {
			posLT.x + paddingX, 
			posLT.y - paddingY - contentOffsetY - iconSize/2
			};
		
		int highlightObjId = 0;
		vector<pair<mouseListener*,int>> iconListenerIds;
		for (int i=0; i<numOfLines; i++) {
			if (i>0) posLineLCen.y -= iconSize+lineSpacing;
			
			TransRecord* trans = transToShow[i];
			
			// printf("DEBUG: %d + %d = %d + %d\n", trans->actor, trans->target, trans->newActor, trans->newTarget);
			// printf("DEBUG: %d\n", trans->autoDecaySeconds);
			
			if (trans == NULL) {
				
				ObjectRecord* currentHintObj = getObject(currentHintObjId);
				if (currentHintObj->numBiomes > 0) {
					
					doublePair pos = posLineLCen;
					pos.x += iconSize/2;
					pos.x += iconSize;
					
					doublePair firstLine = pos;
					firstLine.y += tinyLineHeight/2;
					drawStr("NATURALLY", firstLine, "tinyHandwritten", false);
					firstLine.y -= tinyLineHeight;
					drawStr("SPAWN IN:", firstLine, "tinyHandwritten", false);

					pos.x += iconSize;
					pos.x += iconSize;
					if (currentHintObj->numBiomes < 3) pos.x += iconSize;
					
					for (int b = 0; b < currentHintObj->numBiomes; b++) {
						GroundSpriteSet *s = groundSprites[ currentHintObj->biomes[b] ];
						setDrawColor( 1, 1, 1, 1 );
						drawSprite( s->squareTiles[0][0], pos, 0.25 *guiScale );
						
						doublePair iconLT = {pos.x - iconSize/2, pos.y + iconSize/2};
						doublePair iconBR = {pos.x + iconSize/2, pos.y - iconSize/2};		
						mouseListener* biomeIconListener = getMouseListenerByArea(
							&twotechMouseListeners, sub(iconLT, screenPos), sub(iconBR, screenPos));
						pair<mouseListener*,int> biomeIconListenerId(biomeIconListener, 100000 + currentHintObj->biomes[b]);
						iconListenerIds.push_back(biomeIconListenerId);
						
						pos.x += iconSize;
					}
				}
				
				continue;
			}
			

			doublePair posLineTL = {
				posLineLCen.x - paddingX,
				posLineLCen.y + iconSize/2
			};
			doublePair posLineBR = {
				posLineTL.x + recWidth,
				posLineTL.y - iconSize
			};

			mouseListener* lineListener = getMouseListenerByArea(
				&twotechMouseListeners, sub(posLineTL, screenPos), sub(posLineBR, screenPos));
			if (lineListener->mouseHover) {
				doublePair posLineCen = {posCenter.x, posLineLCen.y};
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(posLineCen, recWidth/2, iconSize/2 * 1.25);
				
				int holdingID = ourLiveObject->holdingID;
				holdingID = getDummyParent(holdingID);
				if (trans->actor == trans->target) {
					highlightObjId = trans->actor;
				} else if (trans->actor > 0 && trans->actor != holdingID) {
					highlightObjId = trans->actor;
				} else if (trans->target > 0 && trans->target != holdingID) {
					highlightObjId = trans->target;
				} else {
					highlightObjId = 0;
				}
			}
			
			
			doublePair iconLT;
			doublePair iconBR;
			
			doublePair pos = posLineLCen;
			pos.x += iconSize/2;
			iconLT = {pos.x - iconSize/2, pos.y + iconSize/2};
			iconBR = {pos.x + iconSize/2, pos.y - iconSize/2};		
			mouseListener* iconAListener = getMouseListenerByArea(
				&twotechMouseListeners, sub(iconLT, screenPos), sub(iconBR, screenPos));
			pair<mouseListener*,int> iconAListenerId(iconAListener, trans->actor);
			iconListenerIds.push_back(iconAListenerId);
			if (iconAListener->mouseHover && trans->actor > 0) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
			if (trans->actor == -1 && trans->autoDecaySeconds != 0) {
				if ( trans->autoDecaySeconds < 0 ) {
					drawObj(pos, trans->actor, "WAIT", to_string(- trans->autoDecaySeconds) + " HR");
				} else {
					int decayTime = trans->autoDecaySeconds;
					if (decayTime >= 60) {
						decayTime = int(round(decayTime / 60.0));
						drawObj(pos, trans->actor, "WAIT", to_string(decayTime) + " MIN");
					} else {
						drawObj(pos, trans->actor, "WAIT", to_string(decayTime) + " SEC");
					}
				}
			} else {
				drawObj(pos, trans->actor);
			}
			if (iconAListener->mouseClick && trans->actor > 0) {
				currentHintObjId = trans->actor;
				if (compareObjUse(trans->actor, trans->newActor) == -1) currentHintObjId = getDummyParent(trans->actor);
				if (compareObjUse(trans->actor, trans->newActor) == 1) currentHintObjId = getDummyLastUse(trans->actor);
			}

			
			pos.x += iconSize;
			drawStr("+", pos, "handwritten", false);
			
			pos.x += iconSize;
			iconLT = {pos.x - iconSize/2, pos.y + iconSize/2};
			iconBR = {pos.x + iconSize/2, pos.y - iconSize/2};		
			mouseListener* iconBListener = getMouseListenerByArea(
				&twotechMouseListeners, sub(iconLT, screenPos), sub(iconBR, screenPos));
			pair<mouseListener*,int> iconBListenerId(iconBListener, trans->target);
			iconListenerIds.push_back(iconBListenerId);
			if (iconBListener->mouseHover && trans->target > 0) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
			if (trans->target == -1) {
				drawObj(pos, trans->target, "EMPTY", "GROUND");
			} else {
				drawObj(pos, trans->target);
			}
			if (iconBListener->mouseClick && trans->target > 0) {
				currentHintObjId = trans->target;
				if (compareObjUse(trans->target, trans->newTarget) == -1) currentHintObjId = getDummyParent(trans->target);
				if (compareObjUse(trans->target, trans->newTarget) == 1) currentHintObjId = getDummyLastUse(trans->target);
			}
			
			
			pos.x += iconSize;
			float transProb = getTransProbability(trans);
			drawStr("=", pos, "handwritten", false);
			if ( transProb != -1 ) {
				string firstPart = "=";
				string secondLine = to_string( transProb * 100 );
				secondLine = secondLine.substr(0, secondLine.find(".") + 2) + "PCT";
				doublePair chanceLinePos = pos;
				float tinyLineHeight = 15.0*guiScale;
				chanceLinePos.y -= tinyLineHeight;
				drawStr(secondLine, chanceLinePos, "tinyHandwritten", false);
			}
			
			pos.x += iconSize;
			iconLT = {pos.x - iconSize/2, pos.y + iconSize/2};
			iconBR = {pos.x + iconSize/2, pos.y - iconSize/2};		
			mouseListener* iconCListener = getMouseListenerByArea(
				&twotechMouseListeners, sub(iconLT, screenPos), sub(iconBR, screenPos));
			pair<mouseListener*,int> iconCListenerId(iconCListener, trans->newActor);
			iconListenerIds.push_back(iconCListenerId);
			if (iconCListener->mouseHover && trans->newActor > 0) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
			if (trans->actor > 0 && trans->target > 0 && trans->newActor == 0) {
				drawObj(pos, trans->newActor);
			} else if (trans->actor == -1 && trans->autoDecaySeconds != 0 && trans->newActor == 0) {
				if (trans->move != 0) {
					drawStr("MOVING...", pos, "tinyHandwritten", false);
				} else if (trans->newTarget == 0) {
					drawStr("DESPAWNS", pos, "tinyHandwritten", false);
				} else {
					drawObj(pos, trans->newActor, "TURNING", "INTO...");
				}
			} else if (trans->newActor == 0) {
				drawObj(pos, trans->newActor, "EMPTY", "GROUND");
			} else {
				drawObj(pos, trans->newActor);
			}
			if (trans->actorChangeChance != 1.0) {
				string secondLine = to_string( trans->actorChangeChance * 100 );
				secondLine = secondLine.substr(0, secondLine.find(".") + 2) + "PCT";
				doublePair chanceLinePos = pos;
				float tinyLineHeight = 15.0*guiScale;
				chanceLinePos.y -= tinyLineHeight;
				drawStr(secondLine, chanceLinePos, "tinyHandwritten", false);
			}
			if (iconCListener->mouseClick && trans->newActor > 0) {
				currentHintObjId = trans->newActor;
				if (compareObjUse(trans->newActor, trans->actor) == -1) currentHintObjId = getDummyParent(trans->newActor);
				if (compareObjUse(trans->newActor, trans->actor) == 1) currentHintObjId = getDummyLastUse(trans->newActor);
			}
			
			pos.x += iconSize;
			if (trans->actor == -1 && trans->autoDecaySeconds != 0 && trans->newActor == 0) {
				//not drawing the plus sign for pure Changes over time...
			} else {
				drawStr("+", pos, "handwritten", false);
			}
			
			pos.x += iconSize;
			iconLT = {pos.x - iconSize/2, pos.y + iconSize/2};
			iconBR = {pos.x + iconSize/2, pos.y - iconSize/2};		
			mouseListener* iconDListener = getMouseListenerByArea(
				&twotechMouseListeners, sub(iconLT, screenPos), sub(iconBR, screenPos));
			pair<mouseListener*,int> iconDListenerId(iconDListener, trans->newTarget);
			iconListenerIds.push_back(iconDListenerId);
			if (iconDListener->mouseHover && trans->newTarget > 0) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
			if (trans->actor == -1 && trans->autoDecaySeconds != 0 && trans->newTarget == 0) {
				//Despawn transitions, "DESPAWNS" is written in the newActor slot, keep this slot empty
			} else {
				drawObj(pos, trans->newTarget, "EMPTY", "GROUND");
			}
			if (trans->targetChangeChance != 1.0) {
				string secondLine = to_string( trans->targetChangeChance * 100 );
				secondLine = secondLine.substr(0, secondLine.find(".") + 2) + "PCT";
				doublePair chanceLinePos = pos;
				float tinyLineHeight = 15.0*guiScale;
				chanceLinePos.y -= tinyLineHeight;
				drawStr(secondLine, chanceLinePos, "tinyHandwritten", false);
			}
			if (iconDListener->mouseClick && trans->newTarget > 0) {
				currentHintObjId = trans->newTarget;
				if (compareObjUse(trans->newTarget, trans->target) == -1) currentHintObjId = getDummyParent(trans->newTarget);
				if (compareObjUse(trans->newTarget, trans->target) == 1) currentHintObjId = getDummyLastUse(trans->newTarget);
			}
		}
		
		if (highlightObjId > 0) {
			GridPos currentPos = {currentX, currentY};
			GridPos closestHintObjPos = getClosestTile(currentPos, highlightObjId);
			if ( !(closestHintObjPos.x == 9999 && closestHintObjPos.y == 9999) ) {
				drawTileRect(closestHintObjPos.x, closestHintObjPos.y, "blue", true);
			}
		}
		
		posLineLCen.y -= buttonHeight; 
		doublePair pos = posLineLCen;
		pos.x += iconSize/2;
		if (showPreviousPageButton) {
			drawStr("<", pos, "main", false);
			doublePair prevPageButtonTLPos = {pos.x - iconSize/2, pos.y + iconSize/2};
			doublePair prevPageButtonBRPos = {pos.x + iconSize/2, pos.y - iconSize/2};
			
			prevListener = getMouseListenerByArea(
				&twotechMouseListeners, 
				sub(prevPageButtonTLPos, screenPos), 
				sub(prevPageButtonBRPos, screenPos));
			if (prevListener->mouseHover) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
		} else {
			prevListener = NULL;
		}
		
		pos.x += iconSize*6;
		if (showNextPageButton) {
			drawStr(">", pos, "main", false);
			doublePair nextPageButtonTLPos = {pos.x - iconSize/2, pos.y + iconSize/2};
			doublePair nextPageButtonBRPos = {pos.x + iconSize/2, pos.y - iconSize/2};
			
			nextListener = getMouseListenerByArea(
				&twotechMouseListeners, 
				sub(nextPageButtonTLPos, screenPos), 
				sub(nextPageButtonBRPos, screenPos));
			if (nextListener->mouseHover) {
				setDrawColor( 1, 1, 1, 0.3 );
				drawRect(pos, iconSize/2, iconSize/2);
			}
		} else {
			nextListener = NULL;
		}
		
		if (showPreviousPageButton || showNextPageButton) {
			pos.x -= iconSize*3;
			string pageInd = to_string(currentTwoTechPage + 1) + "/" + to_string(maxPage);
			drawStr(pageInd, pos, "tinyMain", false);
		}
		
		for (int i=0; i<iconListenerIds.size(); i++) {
			mouseListener* listener = iconListenerIds[i].first;
			int id = iconListenerIds[i].second;
			doublePair iconLT = add(listener->posTL, screenPos);
			doublePair iconCen = { iconLT.x + iconSize/2, iconLT.y - iconSize/2 };
			if (listener->mouseHover && id > 0) {
				doublePair captionPos = {iconCen.x, iconCen.y + iconCaptionYOffset};
				
				if (id >= 100000) {
					int biomeId = id - 100000;
					string biomeName(biomeNames[biomeId]);
					drawStr(biomeName, captionPos, "tinyHandwritten", true, true);
					continue;
				}
				
				string objName(livingLifePage->minitechGetDisplayObjectDescription(id));
				drawStr(objName, captionPos, "tinyHandwritten", true, true);
				
				ObjectRecord* o = getObject(id);
				string objFullDesc(stringToUpperCase(o->description));
				int poundPos = objFullDesc.find("#");
				if (poundPos != -1) {
					string objDesc(objFullDesc.substr(poundPos + 1));
					captionPos.y -= tinyLineHeight*2;
					drawStr(objDesc, captionPos, "tinyHandwritten", true, true);
				}
			}
		}
	}




	float barWidth = recWidth;
	float barHeight = 0;
	float barOffsetY = 0;
	bool showBar = lastHintStr != "" || (ourLiveObject->holdingID != 0 && ourLiveObject->holdingID == currentHintObjId);
	
	if (true) {
		barHeight = tinyLineHeight;
		barOffsetY = - barHeight/2;
	}

	float headerWidth = recWidth;
	float headerHeight = (paddingY + iconSize + barHeight + paddingY);
	doublePair headerLT = {posLT.x, posLT.y + separatorHeight + headerHeight};
	doublePair headerCen = {headerLT.x + headerWidth / 2, headerLT.y - headerHeight / 2};
	setDrawColor( 0, 0, 0, 0.8 );
	drawRect( headerCen, headerWidth/2, headerHeight/2);

	string useStr = "HOW DO I USE:";
	string makeStr = "HOW DO I MAKE:";
	float textWidth = tinyHandwritingFont->measureString( makeStr.c_str() );
	float textXOffset = -iconSize/2 - centerXSeparation/2;
	doublePair textCen = {headerCen.x + textXOffset, headerCen.y + barOffsetY};
	doublePair firstLine = {textCen.x, textCen.y - tinyLineHeight};
	doublePair secondLine = {textCen.x, textCen.y + tinyLineHeight};
	doublePair firstLineLT = {firstLine.x - textWidth/2 - paddingX/2, firstLine.y + tinyLineHeight/2 + paddingY/2};
	doublePair firstLineBR = {firstLine.x + textWidth/2 + paddingX/2, firstLine.y - tinyLineHeight/2 - paddingY/2};
	doublePair secondLineLT = {secondLine.x - textWidth/2 - paddingX/2, secondLine.y + tinyLineHeight/2 + paddingY/2};
	doublePair secondLineBR = {secondLine.x + textWidth/2 + paddingX/2, secondLine.y - tinyLineHeight/2 - paddingY/2};

	setDrawColor( 1, 1, 1, 0.3 );
	if (useOrMake == 0) {
		drawRect( firstLine, textWidth/2 + paddingX/2, tinyLineHeight/2 + paddingY/2);
	} else if (useOrMake == 1) {
		drawRect( secondLine, textWidth/2 + paddingX/2, tinyLineHeight/2 + paddingY/2);
	}
	drawStr(useStr, firstLine, "tinyHandwritten", false);
	drawStr(makeStr, secondLine, "tinyHandwritten", false);

	mouseListener* useListener = getMouseListenerByArea(
		&twotechMouseListeners, 
		sub(firstLineLT, screenPos), 
		sub(firstLineBR, screenPos));
	if (useListener->mouseClick) useOrMake = 0;
	mouseListener* makeListener = getMouseListenerByArea(
		&twotechMouseListeners, 
		sub(secondLineLT, screenPos), 
		sub(secondLineBR, screenPos));
	if (makeListener->mouseClick) useOrMake = 1;


	float iconXOffset = textWidth/2 + centerXSeparation/2;
	doublePair iconCen = {headerCen.x + iconXOffset, headerCen.y + barOffsetY};
	drawObj(iconCen, currentHintObjId);

	doublePair iconTL = {iconCen.x - iconSize/2, iconCen.y + iconSize/2};
	doublePair iconBR = {iconCen.x + iconSize/2, iconCen.y - iconSize/2};
	mouseListener* headerIconListener = getMouseListenerByArea(
		&twotechMouseListeners, 
		sub(iconTL, screenPos), 
		sub(iconBR, screenPos));
	if (headerIconListener->mouseHover && currentHintObjId > 0) {
		doublePair captionPos = {iconCen.x, iconCen.y + iconCaptionYOffset};
		string objName(livingLifePage->minitechGetDisplayObjectDescription(currentHintObjId));
		drawStr(objName, captionPos, "tinyHandwritten", true, true);
		
		ObjectRecord* o = getObject(currentHintObjId);
		string objFullDesc(stringToUpperCase(o->description));
		int poundPos = objFullDesc.find("#");
		if (poundPos != -1) {
			string objDesc(objFullDesc.substr(poundPos + 1));
			captionPos.y -= tinyLineHeight*2;
			drawStr(objDesc, captionPos, "tinyHandwritten", true, true);
		}
	}
	
	if (showBar) {
		string searchStr;
		if (lastHintStr != "") {
			searchStr = "SEARCHING: " + lastHintStr;
		} else if (ourLiveObject->holdingID != 0 && ourLiveObject->holdingID == currentHintObjId) {
			string objName(livingLifePage->minitechGetDisplayObjectDescription(currentHintObjId));
			searchStr = "HOLDING: " + objName;
		}
		
		doublePair barCen = {headerLT.x + barWidth / 2, headerLT.y - barHeight / 2 - paddingY/2};
		drawStr(searchStr, barCen, "tinyHandwritten", false);
	}
	
	doublePair headerRT = {headerLT.x + headerWidth, headerLT.y};
	doublePair minRT = {headerRT.x - paddingX/2, headerRT.y - paddingY/2};
	doublePair minCen = {minRT.x - tinyLineHeight/2, minRT.y - tinyLineHeight/2};
	doublePair minLT = {minRT.x - iconSize/2, minRT.y};
	doublePair minBR = {minRT.x, minRT.y - iconSize/2};
	drawStr("[-]", minCen, "tinyHandwritten", false);
	mouseListener* minListener = getMouseListenerByArea(
		&twotechMouseListeners, 
		sub(minLT, screenPos), 
		sub(minBR, screenPos));
	if (minListener->mouseHover) {
		setDrawColor( 1, 1, 1, 0.3 );
		drawRect(minCen, iconSize/4, iconSize/4);
	}
	if (minListener->mouseClick) {
		minitechMinimized = true;
		minListener->mouseClick = false;
	}
}

void minitech::inputHintStrToSearch(string hintStr) {
	lastHintStr = hintStr;
	if (hintStr == "") {
		changeHintObjOnTouch = true;
	} else {
		changeHintObjOnTouch = false;
	
		int numHits = 0;
		int numRemain = 0;
		ObjectRecord **hitsSimpleVector = searchObjects( hintStr.c_str(),
											 0,
											 200,
											 &numHits, &numRemain );
		
		if (numHits > 0) {
			vector<ObjectRecord*> unsortedHits;
			for (int i=0; i<numHits; i++) {
				unsortedHits.push_back(hitsSimpleVector[i]);
			}
			
			vector<std::size_t> index(unsortedHits.size());
			iota(index.begin(), index.end(), 0);
			sort(index.begin(), index.end(), [&](size_t a, size_t b) { 
				// string aDesc(stringToUpperCase(unsortedHits[a]->description));
				// string bDesc(stringToUpperCase(unsortedHits[b]->description));
				// int aLDist = LevenshteinDistance(hintStr, aDesc); 
				// int bLDist = LevenshteinDistance(hintStr, bDesc);
				// return aLDist < bLDist;
				return unsortedHits[a]->id < unsortedHits[b]->id;
			});
			
			vector<ObjectRecord*> sortedHits(unsortedHits.size());
			for ( int i=0; i<unsortedHits.size(); i++ ) {
				sortedHits[i] = unsortedHits[index[i]];
			}
			
			currentHintObjId = sortedHits[0]->id;
		}
	}
}


void minitech::livingLifeDraw(float mX, float mY) {
	
	if (!minitechEnabled) return;
	
	doublePair screenPos = livingLifePage->minitechGetLastScreenViewCenter();
	doublePair mousePos = {mX, mY};
	doublePair mousePosScreenAdj = sub(mousePos, screenPos);
	
	for ( int i=twotechMouseListeners.size() - 1; i>=0; i-- ) {
		mouseListener* listener = twotechMouseListeners[i];
		if ( posWithinArea(mousePosScreenAdj, listener->posTL, listener->posBR) ) {
			listener->mouseHover = true;
			continue;
		}
		listener->mouseHover = false;
		
		if ( !listener->mouseHover && !listener->mouseClick ) {
			twotechMouseListeners.erase( twotechMouseListeners.begin() + i );
		}
	}
	
	if ( prevListener != NULL ) {
		if ( !prevListener->mouseHover && !prevListener->mouseClick ) {
			prevListener = NULL;
		}
	}
	if ( nextListener != NULL ) {
		if ( !nextListener->mouseHover && !nextListener->mouseClick ) {
			nextListener = NULL;
		}
	}
	
	// currentHintObjId = getDummyParent(currentHintObjId);
	
	if ( lastHintObjId == 0 && currentHintObjId != 0 ) minitechMinimized = false;
	
	if ( (lastHintObjId != currentHintObjId || lastUseOrMake != useOrMake) && !minitechMinimized ) {
		lastHintObjId = currentHintObjId;
		lastUseOrMake = useOrMake;
		currentTwoTechPage = 0;
		
		for (auto p: twotechMouseListeners) {
			delete(p);
		}
		twotechMouseListeners.clear();
		twotechMouseListeners.shrink_to_fit();
		

		vector<TransRecord*> unsortedTrans;
		
		if (useOrMake == 0) {
			unsortedTrans = getUsesTrans(currentHintObjId);
		} else if (useOrMake == 1) {
			unsortedTrans = getProdTrans(currentHintObjId);
		}
		
		// There could be duplicated transitions...
		sort( unsortedTrans.begin(), unsortedTrans.end() );
		unsortedTrans.erase( 
			unique( unsortedTrans.begin(), unsortedTrans.end() ), 
			unsortedTrans.end() 
			);
			
		if (useOrMake == 0) {
			currentHintTrans = sortUsesTrans(unsortedTrans);
		} else if (useOrMake == 1) {
			currentHintTrans = sortProdTrans(unsortedTrans);
			
			ObjectRecord* currentHintObj = getObject(currentHintObjId);
			if (currentHintObj != NULL && currentHintObj->numBiomes > 0 && currentHintObj->mapChance > 0) 
				currentHintTrans.insert(currentHintTrans.begin(), NULL);
		}

	}
	
	updateDrawTwoTech();

}

void minitech::livingLifeStep() {
	ourLiveObject = livingLifePage->getOurLiveObject();
	if (!ourLiveObject) return;
	
	stepCount++;
	if (stepCount > 10000) stepCount = 0;
	
	currentX = round(ourLiveObject->currentPos.x);
	currentY = round(ourLiveObject->currentPos.y);
	

}

bool minitech::livingLifeKeyDown(unsigned char inASCII) {	
	
	if (livingLifePage->minitechSayFieldIsFocused()) return false;
	
	bool commandKey = isCommandKeyDown();
	bool shiftKey = isShiftKeyDown();


	
	if (!commandKey && !shiftKey && inASCII == 9) {
		currentTwoTechPage += 1;
	}
	
	if (!commandKey && shiftKey && inASCII == 9) {
		currentTwoTechPage -= 1;
	}
	
	if (!shiftKey && !commandKey && toupper(inASCII) == toupper(minimizeKey)) {
		minitechMinimized = !minitechMinimized;
	}
	
	// if ( inASCII == 'p' ) {
		// guiScale += 0.3;
	// }
	// if ( inASCII == 'l' ) {
		// guiScale -= 0.3;
	// }
	// if ( inASCII == 'p' || inASCII == 'l' ) {
		// delete minitech::handwritingFont;
		// delete minitech::mainFont;
		// delete minitech::tinyHandwritingFont;
		// delete minitech::tinyMainFont;
		// minitech::handwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16*guiScale );
		// minitech::handwritingFont->setMinimumPositionPrecision( 1 );
		// minitech::mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16*guiScale );
		// minitech::mainFont->setMinimumPositionPrecision( 1 );	
		// minitech::tinyHandwritingFont = new Font( "font_handwriting_32_32.tga", 3, 6, false, 16/2*guiScale );
		// minitech::tinyHandwritingFont->setMinimumPositionPrecision( 1 );
		// minitech::tinyMainFont = new Font( getFontTGAFileName(), 6, 16, false, 16/2*guiScale );
		// minitech::tinyMainFont->setMinimumPositionPrecision( 1 );
	// }
	
	return false;
}

bool minitech::livingLifePageMouseDown( float mX, float mY ) {
	
	doublePair screenPos = livingLifePage->minitechGetLastScreenViewCenter();
	doublePair mousePos = {mX, mY};
	doublePair mousePosScreenAdj = sub(mousePos, screenPos);
	
	bool clickCaught = false;
	for ( int i=0; i<twotechMouseListeners.size(); i++ ) {
		mouseListener* listener = twotechMouseListeners[i];
		if ( posWithinArea(mousePosScreenAdj, listener->posTL, listener->posBR) ) {
			listener->mouseClick = true;
			clickCaught = true;
			continue;
		}
		listener->mouseClick = false;
	}
	
	if (prevListener != NULL) {
		if (prevListener->mouseClick) {
			currentTwoTechPage -= 1;
			prevListener->mouseClick = false;
		}
	}
	if (nextListener != NULL) {
		if (nextListener->mouseClick) {
			currentTwoTechPage += 1;
			nextListener->mouseClick = false;
		}
	}
	
	return clickCaught;
}

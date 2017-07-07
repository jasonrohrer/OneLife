#include "SpritePickable.h"
#include "ObjectPickable.h"
#include "OverlayPickable.h"
#include "CategoryPickable.h"

SimpleVector<int> SpritePickable::sStack;
SimpleVector<int> ObjectPickable::sStack;
SimpleVector<int> OverlayPickable::sStack;
SimpleVector<int> CategoryPickable::sStack;

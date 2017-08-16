#include "SpritePickable.h"
#include "ObjectPickable.h"
#include "OverlayPickable.h"
#include "GroundPickable.h"

SimpleVector<int> SpritePickable::sStack;
SimpleVector<int> ObjectPickable::sStack;
SimpleVector<int> OverlayPickable::sStack;
SimpleVector<int> GroundPickable::sStack;

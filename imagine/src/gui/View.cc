#include "View.hh"

ResourceFace * View::defaultFace = 0;
bool View::needsBackControl = needsBackControlDefault;
View *View::modalView = nullptr;
View::RemoveModalViewDelegate View::removeModalViewDel;

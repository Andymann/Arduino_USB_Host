// MenuItem.cpp
#include "SubMenuItem.h"

SubMenuItem::SubMenuItem() {
  sName = "";
}

SubMenuItem::SubMenuItem(String pName) {
  sName = pName;
}

String SubMenuItem::getName(){
  return sName;
}

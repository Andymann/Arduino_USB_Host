
// MenuItem.h
#ifndef RootMenuItem_h
#define RootMenuItem_h

#include <Arduino.h>
#include "SubMenuItem.h"

class RootMenuItem {
  private:
    uint8_t iSubMenuItemCount = 0;
    uint8_t iSelectedIndex = 0;
    String sName;
    SubMenuItem arrSubMenuItem[7];

  public:
    RootMenuItem(String pName, uint8_t pSubMenuItemCount);
    void addSubMenuItem(SubMenuItem pItem);
    void selectFirstItem();
    void selectNextItem();
    void getSubMenuItems();
};

#endif
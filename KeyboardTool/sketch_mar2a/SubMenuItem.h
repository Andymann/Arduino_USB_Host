// MenuItem.h
#ifndef SubMenuItem_h
#define SubMenuItem_h

#include <Arduino.h>

class SubMenuItem {
  private:
    String sName;

  public:
    SubMenuItem();
    SubMenuItem(String pName);
    String getName();
};

#endif
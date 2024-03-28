
// MenuItem.cpp
#include "RootMenuItem.h"
#include "SubMenuItem.h"

RootMenuItem::RootMenuItem(String pName, uint8_t pSubMenuItemCount) {
  iSubMenuItemCount = pSubMenuItemCount;
  sName = pName;
}


void RootMenuItem::addSubMenuItem(SubMenuItem pItem){
  for(uint8_t i=0; i<iSubMenuItemCount; i++){
    if(arrSubMenuItem[i].getName()==""){
      arrSubMenuItem[i] = pItem;
      Serial.println("RootMenuItem" + sName + " addSubMenuItem(): adding " + pItem.getName() + " at position " + i);
      iSubMenuItemCount++;
      break;
    }
  }
}

void RootMenuItem::selectFirstItem(){
  iSelectedIndex = 0;
}

void RootMenuItem::selectNextItem(){

}

void RootMenuItem::getSubMenuItems(){
  Serial.println(sName);
  for(uint8_t i=0; i<iSubMenuItemCount; i++){
    if( arrSubMenuItem[i].getName() != "" ){
      Serial.println("\t" + arrSubMenuItem[i].getName());
    }else{
      break;
    }
  }
}
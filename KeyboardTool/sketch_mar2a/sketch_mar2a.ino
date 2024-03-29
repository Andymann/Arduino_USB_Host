

/*
  KeyboardTool
  Based on mini USB host shield
  Takes a MIDI input from a USB MIDI Keyboard, processes it and sends it out via MIDI socket.

    -velocity fixer: converts incoming velocity to a fix value
    -velocity booster: multiplies a note's velocity by a given factor (for keyboards with no adjustable velcity-curve)
    -scaler 1: only processes notes that fit into a selected scale
    -scaler 2: maps every incoming note to the nearest note of a selected scale

    74hc4067 doesn't need pulldown resistors on slow buttons but DEFINITELY on a rotary encoder

*/
#include <usbh_midi.h>
#include <usbhub.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <light_CD74HC4067.h>
#include <Button2.h>
#include "AppFeature.h"


//Define your font here. Default font: lcd5x7
#define menuFont ZevvPeep8x16



USB Usb;
USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

#define VERSION "0.5"
#define MAX_RESET 8 //MAX3421E pin 12
#define MAX_GPX   9 //MAX3421E pin 17

#define ENCODER_A 13
#define ENCODER_B 14
#define ENCODER_CLICK 12

// 0X3C+SA0 - 0x3C or 0x3D
#define DISPLAY_I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

CD74HC4067 mux(2, 3, 4, 5);  // S0, S1, S2, S3.  EN->GND
#define DEMUX_PIN A0 // S16
bool muxValue[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // The value of the Buttons as read from the multiplexer

int iEncoderValue = 0;
bool bEncoderClick_old = false;

#define LONGCLICKTIMEMS 2000
#define LED 6

AppFeature arrFeatures[] = {
  AppFeature("PT", FEATURE_GROUP_VELOCITY, VELOCITY_PASSTHRU, true),
  AppFeature("Fix63", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_63),
  AppFeature("Fix100", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_100),
  AppFeature("Fix127", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_127),
  AppFeature("Rnd", FEATURE_GROUP_VELOCITY, VELOCITY_RANDOM),
  
  AppFeature("PT", FEATURE_GROUP_SCALE, SCALE_PASSTHRU, true),
  AppFeature("Major", FEATURE_GROUP_SCALE, SCALE_MAJOR),
  AppFeature("Minor", FEATURE_GROUP_SCALE, SCALE_MINOR),
  AppFeature("Penta", FEATURE_GROUP_SCALE, SCALE_PENTATONIC),

  AppFeature(" ", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_PASSTHROUGH, true),
  AppFeature("C", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_C),
  AppFeature("C#", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_Cs),
  AppFeature("D", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_D),
  AppFeature("D#", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_Ds),
  AppFeature("E", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_E),
  AppFeature("F", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_F),
  AppFeature("F#", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_Fs),
  AppFeature("G", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_G),
  AppFeature("G#", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_Gs),
  AppFeature("A", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_A),
  AppFeature("A#", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_As),
  AppFeature("H", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_H),


  AppFeature("PT", FEATURE_GROUP_CHANNEL, CHANNEL_PASSTHRU, true),
  AppFeature("1", FEATURE_GROUP_CHANNEL, CHANNEL_1),
  AppFeature("2", FEATURE_GROUP_CHANNEL, CHANNEL_2),
  AppFeature("3", FEATURE_GROUP_CHANNEL, CHANNEL_3),
  AppFeature("4", FEATURE_GROUP_CHANNEL, CHANNEL_4),
  AppFeature("5", FEATURE_GROUP_CHANNEL, CHANNEL_5),
  AppFeature("6", FEATURE_GROUP_CHANNEL, CHANNEL_6),
  AppFeature("7", FEATURE_GROUP_CHANNEL, CHANNEL_7),
  AppFeature("8", FEATURE_GROUP_CHANNEL, CHANNEL_8),
  AppFeature("9", FEATURE_GROUP_CHANNEL, CHANNEL_9),
  AppFeature("10", FEATURE_GROUP_CHANNEL, CHANNEL_10),
  AppFeature("11", FEATURE_GROUP_CHANNEL, CHANNEL_11),
  AppFeature("12", FEATURE_GROUP_CHANNEL, CHANNEL_12),
  AppFeature("13", FEATURE_GROUP_CHANNEL, CHANNEL_13),
  AppFeature("14", FEATURE_GROUP_CHANNEL, CHANNEL_14),
  AppFeature("15", FEATURE_GROUP_CHANNEL, CHANNEL_15),
  AppFeature("16", FEATURE_GROUP_CHANNEL, CHANNEL_16),
  AppFeature(" ", FEATURE_GROUP_PLACEHOLDER, 0)
};

const uint8_t FEATURECOUNT = 40;
int iMenuPosition = -3;
uint8_t iRootNoteOffset=0;

//Button2 btnHelper_PRESET1;
//Button2 btnHelper_PRESET2;
//Button2 btnHelper_PRESET3;

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  //sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  //SerialPrintln(buf); 
}

void setup()
{
  randomSeed(analogRead(1));
  pinMode(DEMUX_PIN, INPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(31250);
  
  digitalWrite(LED, 1);
  while(!Serial);
  //SerialPrintln("Serial init'd");
    pinMode(MAX_GPX, INPUT);
    pinMode(MAX_RESET, OUTPUT);
    digitalWrite(MAX_RESET, LOW);
    delay(50); //wait 20ms
    digitalWrite(MAX_RESET, HIGH);
    delay(50); //wait 20ms

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );

  // Register onInit() function
  Midi.attachOnInit(onInit);

  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
/*
  btnHelper_PRESET1.begin(VIRTUAL_PIN,INPUT, false);
  btnHelper_PRESET1.setButtonStateFunction(preset1ButtonStateHandler);
  btnHelper_PRESET1.setLongClickTime( LONGCLICKTIMEMS );
  btnHelper_PRESET1.setClickHandler(preset1ClickHandler);
  btnHelper_PRESET1.setLongClickDetectedHandler(preset1LongClickDetected);
  btnHelper_PRESET1.setChangedHandler(preset1ChangeHandler);  // Falls externer sync via sysex

  btnHelper_PRESET2.begin(VIRTUAL_PIN,INPUT, false);
  btnHelper_PRESET2.setButtonStateFunction(preset2ButtonStateHandler);
  btnHelper_PRESET2.setLongClickTime( LONGCLICKTIMEMS );
  btnHelper_PRESET2.setClickHandler(preset2ClickHandler);
  btnHelper_PRESET2.setLongClickDetectedHandler(preset2LongClickDetected);
  btnHelper_PRESET2.setChangedHandler(preset2ChangeHandler);  // Falls externer sync via sysex

  btnHelper_PRESET3.begin(VIRTUAL_PIN,INPUT, false);
  btnHelper_PRESET3.setButtonStateFunction(preset3ButtonStateHandler);
  btnHelper_PRESET3.setLongClickTime( LONGCLICKTIMEMS );
  btnHelper_PRESET3.setClickHandler(preset3ClickHandler);
  btnHelper_PRESET3.setLongClickDetectedHandler(preset3LongClickDetected);
  btnHelper_PRESET3.setChangedHandler(preset3ChangeHandler);  // Falls externer sync via sysex
*/
  //SerialPrintln("ok");
  showInfo(1000);
  digitalWrite(LED, 0);
}

void loop()
{
  readMux();
//  btnHelper_PRESET1.loop();
//  btnHelper_PRESET2.loop();
//  btnHelper_PRESET3.loop();


  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }

  iEncoderValue = queryEncoder();
  if(iEncoderValue!=0){
    // New value
    processMenuNavigation( iEncoderValue );
  }


  if((muxValue[ENCODER_CLICK==true]) && ( bEncoderClick_old==false )){
    processEncoderClick();
    bEncoderClick_old = true;
  }else if((muxValue[ENCODER_CLICK]==false) && ( bEncoderClick_old )){
    bEncoderClick_old = false;
  }
}

// Poll USB MIDI Controller and send to serial MIDI
void MIDI_poll()
{
  uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
  uint16_t  rcvd;

  if (Midi.RecvData( &rcvd,  bufMidi) == 0 ) {
    processData(bufMidi);
  }
  
}

void processData( uint8_t pBufMidi[] ){
  //printData(pBufMidi);
  switch (pBufMidi[0]) {
  case 0x08:
    processNoteOff(pBufMidi);
    break;
  case 0x09:
    processNoteOn(pBufMidi);
    break;
  case 0x0B:
    processCC(pBufMidi);
    break;
  default:
    // Statement(s)
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
}

}

void printData(uint8_t pBufMidi[]){
  char buf[16];
  for (int i = 0; i < 16; i++) {
    sprintf(buf, " %02X", pBufMidi[i]);
    SerialPrintln(buf);
  }
  SerialPrintln("");
}

void processNoteOff( uint8_t pBufMidi[] ){
  // tbd
}

void processNoteOn( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0x90;
  uint8_t iPitch = pBufMidi[2];
  uint8_t iVelocity = pBufMidi[3];
  bool bSendOut = true;
  
  
  //SerialPrintln("IN: Note ON " + String(iPitch%12) + "  Chan: " + String(iChannel+1) + "  Velo: " + String(iVelocity) );
  //displayIncoming( pBufMidi );
  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].isSelected()){
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
        if(iVelocity>0){
          if(arrFeatures[i].getFeature()==VELOCITY_PASSTHRU){
            // Don't change
          }else if(arrFeatures[i].getFeature()==VELOCITY_FIX_63){
            iVelocity = 63;
          }if(arrFeatures[i].getFeature()==VELOCITY_FIX_100){
            iVelocity = 100;
          }if(arrFeatures[i].getFeature()==VELOCITY_FIX_127){
            iVelocity = 127;
          }if(arrFeatures[i].getFeature()==VELOCITY_RANDOM){
            iVelocity=(int)random(128);
          }
        }else{
          
        }
      }
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
        if(arrFeatures[i].getFeature()==CHANNEL_PASSTHRU){
          // Don't change anything
        }else if(arrFeatures[i].getFeature()==CHANNEL_1){
          iChannel=1;
        }else if(arrFeatures[i].getFeature()==CHANNEL_2){
          iChannel=2;
        }else if(arrFeatures[i].getFeature()==CHANNEL_3){
        iChannel=3;
        }else if(arrFeatures[i].getFeature()==CHANNEL_4){
          iChannel=4;
        }else if(arrFeatures[i].getFeature()==CHANNEL_5){
          iChannel=5;
        }else if(arrFeatures[i].getFeature()==CHANNEL_6){
          iChannel=6;
        }else if(arrFeatures[i].getFeature()==CHANNEL_7){
        iChannel=7;
        }else if(arrFeatures[i].getFeature()==CHANNEL_8){
          iChannel=8;
        }else if(arrFeatures[i].getFeature()==CHANNEL_9){
        iChannel=9;
        }else if(arrFeatures[i].getFeature()==CHANNEL_10){
          iChannel=10;
        }else if(arrFeatures[i].getFeature()==CHANNEL_11){
        iChannel=11;
        }else if(arrFeatures[i].getFeature()==CHANNEL_12){
        iChannel=12;
        }else if(arrFeatures[i].getFeature()==CHANNEL_13){
        iChannel=13;
        }else if(arrFeatures[i].getFeature()==CHANNEL_14){
          iChannel=14;
        }else if(arrFeatures[i].getFeature()==CHANNEL_15){
        iChannel=15;
        }else if(arrFeatures[i].getFeature()==CHANNEL_16){
          iChannel=16;
        }
      }

      /*
        First we need to get the selected ROOT note. this will be taken as an offset: C=0, C#=1, etc.
        Scale transform: indices based on root note and iRootNoteOffset
          Major scale: 0 2 4 5 7 8 11
          Minor scale: 0 2 3 5 7 8 10
      */
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE){
        int tmpNote = (iPitch+12-iRootNoteOffset)%12;
        if(arrFeatures[i].getFeature()==SCALE_PASSTHRU){
          // Do nothing
        }else if(arrFeatures[i].getFeature()==SCALE_MAJOR){
          //SerialPrintln("X " + String(iRootNoteOffset)+ ":"+ String((iPitch%12) - iRootNoteOffset) );
          
          if( (tmpNote == 0)||
              (tmpNote == 2)||
              (tmpNote == 4)||
              (tmpNote == 5)||
              (tmpNote == 7)||
              (tmpNote == 9)||
              (tmpNote == 11) ){
                
            }else{
              bSendOut = false;
            }
        }else if(arrFeatures[i].getFeature()==SCALE_MINOR){
          if( (tmpNote == 0)||
              (tmpNote == 2)||
              (tmpNote == 3)||
              (tmpNote == 5)||
              (tmpNote == 7)||
              (tmpNote == 8)||
              (tmpNote == 10) ){
                
            }else{
              bSendOut = false;
            }
        }else if(arrFeatures[i].getFeature()==SCALE_PENTATONIC){

        }
      }
    }
  }
  
  if(bSendOut){
    if( iVelocity>0){
      digitalWrite(LED, 1);
    }else{
      digitalWrite(LED, 0);
    }
    

    
    //SerialPrintln("OUT: Note ON " + String(iPitch) + "  Chan: " + String(iChannel) + "  Velo: " + String(iVelocity) );
    Serial.write( byte(0x90 + iChannel-1) );
    Serial.write( byte(iPitch) );
    Serial.write( byte(iVelocity) );
  }
  
}

void processCC( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0xB0;
  uint8_t iController = pBufMidi[2];
  uint8_t iValue = pBufMidi[3];
  //SerialPrintln("CC. Channel:" + String(iChannel) + " CC:" + String(iController)+ " Value:" + String(iValue) );
}

void showInfo(int pWaitMS) {
  oled.setFont(menuFont);
  oled.clear();
  oled.set2X();
  oled.println("Keeboard");
  oled.set1X();
  oled.print("  Version ");
  oled.println(VERSION);
  oled.println();
  oled.println(" Andyland.info");
  delay(pWaitMS);
}

void displayIncoming( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0x90;
  uint8_t iPitch = pBufMidi[2];
  uint8_t iVelocity = pBufMidi[3];
  oled.clear();
  oled.set2X();
  oled.println( "Note:" +String(iPitch) );
  oled.set1X();
  oled.println("Velo:" + String(iVelocity) +  " Chan:" + String(iChannel));
}


void readMux() {
  // loop through channels 0 - 15
  for (uint8_t i = 0; i < 15; i++) {
    mux.channel(i);
    int val = digitalRead(DEMUX_PIN);
    muxValue[i] = val;
  }
}


bool encoder0PinALast = false;
uint8_t encoder0Pos = 128;
uint8_t encoder0PosOld = 128;

// Returns -1 / +1
int queryEncoder(){
  int iReturn = 0;
  if ((encoder0PinALast == false) && (muxValue[ENCODER_A] == true)) {
     if (muxValue[ENCODER_B] == false) {
       encoder0Pos--;
     } else {
       encoder0Pos++;
     }
   } 
   
   if ((encoder0PinALast == true) && (muxValue[ENCODER_A] == false)) {
     if (muxValue[ENCODER_B] == true) {
       encoder0Pos--;
     } else {
       encoder0Pos++;
     }
   }
   
   if (encoder0Pos != encoder0PosOld){
     if(encoder0Pos%2==0){
      if(encoder0Pos<encoder0PosOld){
        iReturn = -1;
      }else{
        iReturn = 1;
      }
     }
     encoder0PosOld = encoder0Pos;
   }
   encoder0PinALast = muxValue[ENCODER_A];
  return iReturn;
}

void processEncoderClick(){
  // We are at iMenuPosition
  uint8_t tmpFG = arrFeatures[iMenuPosition].getFeatureGroup();
  for(uint8_t i=0; i< FEATURECOUNT; i++){
    if( arrFeatures[i].getFeatureGroup() == tmpFG){
      arrFeatures[i].select(false);
    }
  }

  // If scale passthrough is selected then we select root-note-passthrough.
  if(arrFeatures[iMenuPosition].getFeatureGroup()==FEATURE_GROUP_SCALE){
    if(arrFeatures[iMenuPosition].getFeature()==SCALE_PASSTHRU){
      for(uint8_t i=0; i< FEATURECOUNT; i++){
        if( arrFeatures[i].getFeatureGroup() == FEATURE_GROUP_ROOTNOTE){
          if(arrFeatures[i].getFeature()==ROOTNOTE_PASSTHROUGH){
            arrFeatures[i].select(true);
          }else{
            arrFeatures[i].select(false);
          }
        }
      }
    }
  }

  if(  arrFeatures[iMenuPosition].getFeatureGroup() != FEATURE_GROUP_PLACEHOLDER ){
    arrFeatures[iMenuPosition].select(true);
  }
  
  if(arrFeatures[iMenuPosition].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
    if(arrFeatures[iMenuPosition].getFeature() > ROOTNOTE_PASSTHROUGH){
      iRootNoteOffset = arrFeatures[iMenuPosition].getFeature()-1; // <-- megapfiffig!
    }
  }
  
  showMenu(getPreviousMenuItem(iMenuPosition), getMenuItem( iMenuPosition ), getNextMenuItem(iMenuPosition));
}



void processMenuNavigation(int pDirection){
  if(iMenuPosition==-3){  // on the fery first encoder rotation we jump to the first item
    iMenuPosition = 0;
  }else{
    iMenuPosition += pDirection;
  }

  if(iMenuPosition<0){
    iMenuPosition = FEATURECOUNT-1;
  }else if(iMenuPosition>=FEATURECOUNT){
    iMenuPosition =0;
  }

  showMenu(getPreviousMenuItem(iMenuPosition), getMenuItem( iMenuPosition ), getNextMenuItem(iMenuPosition));
}

void showMenu(String pLine1, String pLine2, String pLine3) {
  //oled.setFont(menuFont);
  oled.clear();
  oled.setFont(TimesNewRoman16);
  //oled.set2X();
  //oled.set1X();
  oled.println( " " + pLine1 );
  //oled.set1X();
   oled.setFont(TimesNewRoman16_bold);
  oled.println( "*" + pLine2 );
  //oled.set1X();
   oled.setFont(TimesNewRoman16);
  oled.println( " " + pLine3 );
}

String getMenuItem(int pPosition){
  String sTmp = arrFeatures[pPosition].getText();
  if(arrFeatures[pPosition].isSelected()){
    sTmp += "(X)";
  }
  return getFeaturePrefix(pPosition) + " " + sTmp;
}

String getPreviousMenuItem(int pPosition){
  int tmpPos;
  String sTmp;
  if(pPosition<0){ 
    sTmp = "";
  }else if(pPosition==0){
    tmpPos = FEATURECOUNT-1;
  }else if(pPosition>0){
    tmpPos = pPosition-1;
  }

  sTmp= arrFeatures[tmpPos].getText();
  if(arrFeatures[tmpPos].isSelected()){
    sTmp += "(X)";
  }

  return getFeaturePrefix(tmpPos) + " " + sTmp;
}

String getNextMenuItem(int pPosition){
  String sTmp = "";
  int tmpPos;
  if(pPosition<0){ 
    tmpPos = 0;
  }else if((pPosition>=0)&&(pPosition<FEATURECOUNT-1)){
    tmpPos = pPosition+1;
  }else if(pPosition==FEATURECOUNT){
    return "UKU!";
  }else{
    tmpPos = 0;
  }

  sTmp = arrFeatures[tmpPos].getText();
  if(arrFeatures[tmpPos].isSelected()){
    sTmp += "(X)";
  }
  return getFeaturePrefix(tmpPos) + " " + sTmp;
}

String getFeaturePrefix(uint8_t pIndex){
  if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
    return "Chan";
  }else if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
    return "Velo";
  }if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_SCALE){
    return "Scale";
  }if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
    return "Root";
  }else{
    return "";
  }
}

byte preset1ButtonStateHandler() {
  bool b = muxValue[3];
  return b == 0 ? false : true;
}

void preset1ClickHandler(Button2& btn) {

}

void preset1LongClickDetected(Button2& btn) {

}

void preset1ChangeHandler( Button2& btn ){

}

byte preset2ButtonStateHandler() {
  bool b = muxValue[3];
  return b == 0 ? false : true;
}

void preset2ClickHandler(Button2& btn) {

}

void preset2LongClickDetected(Button2& btn) {

}

void preset2ChangeHandler( Button2& btn ){

}

byte preset3ButtonStateHandler() {
  bool b = muxValue[3];
  return b == 0 ? false : true;
}

void preset3ClickHandler(Button2& btn) {

}

void preset3LongClickDetected(Button2& btn) {

}

void preset3ChangeHandler( Button2& btn ){

}




void SerialPrintln(String p){
 //Serial.println(p);
}

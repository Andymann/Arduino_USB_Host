

/*
  KeyboardTool
  Based on mini USB host shield
  Takes a MIDI input from a USB MIDI Keyboard, processes it and sends it out via MIDI socket.

  A Bug in MAPPER mode: high pitch notes in c# minor, penta, etc.

*/
#include <usbh_midi.h>
#include <usbhub.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <light_CD74HC4067.h>
#include "AppFeature.h"
#include <EEPROM.h>

//Define your font here. Default font: lcd5x7
#define menuFont ZevvPeep8x16



USB Usb;
USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

#define VERSION "0.97"
#define MAX_RESET 8 //MAX3421E pin 12
#define MAX_GPX   9 //MAX3421E pin 17

#define BUTTON_0 0
#define BUTTON_1 1
#define BUTTON_2 2
#define BUTTON_3 3
#define ENCODER_A 13
#define ENCODER_B 14
#define ENCODER_CLICK 12

const uint8_t PRESET_0_BASE_ADDRESS = 10;
const uint8_t PRESET_1_BASE_ADDRESS = 20;
const uint8_t PRESET_2_BASE_ADDRESS = 30;
const uint8_t PRESET_3_BASE_ADDRESS = 40;

bool bButton_0_old = false;
bool bButton_1_old = false;
bool bButton_2_old = false;
bool bButton_3_old = false;

// 0X3C+SA0 - 0x3C or 0x3D
#define DISPLAY_I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

CD74HC4067 mux(2, 3, 4, 5);  // S0, S1, S2, S3.  EN->GND
#define DEMUX_PIN A0 // S16
bool muxValue[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // The value of the Buttons as read from the multiplexer

int iEncoderValue = 0;
bool bEncoderClick_old = false;
uint8_t iActivePreset = 127;

#define LED 6

AppFeature arrFeatures[] = {
  AppFeature("PT", FEATURE_GROUP_VELOCITY, VELOCITY_PASSTHRU, true),
  AppFeature("Fix63", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_63),
  AppFeature("Fix100", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_100),
  AppFeature("Fix127", FEATURE_GROUP_VELOCITY, VELOCITY_FIX_127),
  AppFeature("Rnd", FEATURE_GROUP_VELOCITY, VELOCITY_RANDOM),
  AppFeature("Rnd100", FEATURE_GROUP_VELOCITY, VELOCITY_RANDOM_100),
  
  AppFeature("Scale PT", FEATURE_GROUP_SCALE, SCALE_PASSTHRU, true),
  AppFeature("Major", FEATURE_GROUP_SCALE, SCALE_MAJOR),
  AppFeature("Minor", FEATURE_GROUP_SCALE, SCALE_MINOR),
  AppFeature("Penta Maj", FEATURE_GROUP_SCALE, SCALE_PENTATONIC_MAJOR),
  AppFeature("Penta Min", FEATURE_GROUP_SCALE, SCALE_PENTATONIC_MINOR),

  AppFeature("None", FEATURE_GROUP_ROOTNOTE, ROOTNOTE_PASSTHROUGH, true),
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

  AppFeature("Fltr", FEATURE_GROUP_SCALE_HANDLER, SCALE_HANDLER_FILTER, true),
  AppFeature("Map", FEATURE_GROUP_SCALE_HANDLER, SCALE_HANDLER_MAPPER),

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

const uint8_t FEATURECOUNT = 44;
int iMenuPosition = -3;
uint8_t iRootNoteOffset=0;

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  //sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  //SerialPrintln(buf); 
}

void deletePresets(){
  EEPROM.update(10, 255);
  EEPROM.update(11, 255);
  EEPROM.update(12, 255);
  EEPROM.update(13, 255);

  EEPROM.update(20, 255);
  EEPROM.update(21, 255);
  EEPROM.update(22, 255);
  EEPROM.update(23, 255);

  EEPROM.update(30, 255);
  EEPROM.update(31, 255);
  EEPROM.update(32, 255);
  EEPROM.update(33, 255);

  EEPROM.update(40, 255);
  EEPROM.update(41, 255);
  EEPROM.update(42, 255);
  EEPROM.update(43, 255);
}

void setup()
{
  //deletePresets();
  randomSeed(analogRead(1));
  pinMode(DEMUX_PIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
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

  Serial.begin(31250);
  while(!Serial);

  showInfo(1000);
  digitalWrite(LED, 0);
}

void loop()
{
  
  readMux();

  if( (bButton_0_old==false) && (muxValue[BUTTON_0]==true) ){
    bButton_0_old = true;
  }else{
    bButton_0_old = muxValue[BUTTON_0];
  }

  if( (bButton_1_old==false) && (muxValue[BUTTON_1]==true) ){
    if(muxValue[BUTTON_0]==false){
      loadPreset(0);
    }else{
      savePreset(0);
    }
    
    bButton_1_old = true;
  }else{
    bButton_1_old = muxValue[BUTTON_1];
  }

  if( (bButton_2_old==false) && (muxValue[BUTTON_2]==true) ){
    if(muxValue[BUTTON_0]==false){
      loadPreset(1);
    }else{
      savePreset(1);
    }
    bButton_2_old = true;
  }else{
    bButton_2_old = muxValue[BUTTON_2];
  }

  if( (bButton_3_old==false) && (muxValue[BUTTON_3]==true) ){
    if(muxValue[BUTTON_0]==false){
      loadPreset(2);
    }else{
      savePreset(2);
    }
    bButton_3_old = true;
  }else{
    bButton_3_old = muxValue[BUTTON_3];
  }

  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }


  iEncoderValue = queryEncoder();
  if(iEncoderValue!=0){
    // New value
    processMenuNavigation( iEncoderValue );
  }


  if((muxValue[ENCODER_CLICK]==true) && ( bEncoderClick_old==false )){
    processEncoderClick();
    bEncoderClick_old = true;
  }else if((muxValue[ENCODER_CLICK]==false) && ( bEncoderClick_old )){
    bEncoderClick_old = false;
  }

}

// Poll USB MIDI Controller and send to serial MIDI
void MIDI_poll(){

  uint8_t bufMidi[ 3 ];
  uint8_t size;

  do {
    if ( (size = Midi.RecvData(bufMidi)) > 0 ) {
      processData( bufMidi );
    }
  } while (size > 0);
}

void processData( uint8_t pBufMidi[] ){

  if((pBufMidi[0]>=0x80)&&(pBufMidi[0]<0x8F)){
    pBufMidi[2]=0x00;
    processNoteOn( pBufMidi );
  }else if((pBufMidi[0]>=0x90)&&(pBufMidi[0]<0x9F)){
    processNoteOn( pBufMidi );
  }else if((pBufMidi[0]>=0xB0)&&(pBufMidi[0]<0xBF)){
    processCC(pBufMidi);
  }else if((pBufMidi[0]>=0xE0)&&(pBufMidi[0]<0xEF)){
    processPitchwheel( pBufMidi );
  }else{
    processOther( pBufMidi );
  }

}

void printData(uint8_t pBufMidi[]){
  char buf[16];
  for (int i = 0; i < 16; i++) {
    sprintf(buf, " %02X", pBufMidi[i]);
    //SerialPrintln(buf);
  }
  //SerialPrintln("");
}

void processNoteOff( uint8_t pBufMidi[] ){
  // tbd
}


uint8_t processMidi_Velocity(uint8_t pBufMidi[]){
  uint8_t iVelocity = pBufMidi[2];

  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].isSelected()){
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
        if(iVelocity>0){
          if(arrFeatures[i].getFeature()==VELOCITY_PASSTHRU){
            // Don't change
          }else if(arrFeatures[i].getFeature()==VELOCITY_FIX_63){
            iVelocity = 63;
            break;
          }if(arrFeatures[i].getFeature()==VELOCITY_FIX_100){
            iVelocity = 100;
            break;
          }if(arrFeatures[i].getFeature()==VELOCITY_FIX_127){
            iVelocity = 127;
            break;
          }if(arrFeatures[i].getFeature()==VELOCITY_RANDOM){
            iVelocity=(int)random(128);
            break;
          }else if(arrFeatures[i].getFeature()==VELOCITY_RANDOM_100){
            iVelocity=(int)(95+random(11));
            break;
          }
        }else{
          iVelocity = 0;
          break;
        }
      }
    }
  }
  return iVelocity;
}

uint8_t processMidi_Channel( uint8_t pBufMidi[], uint8_t pOffset ){
  uint8_t iChannel;// = pBufMidi[1] - pOffset/*0x90*/;
  
  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].isSelected()){
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
        if(arrFeatures[i].getFeature()==CHANNEL_PASSTHRU){
          // Don't change anything
          iChannel = pBufMidi[0]- pOffset;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_1){
          iChannel=0;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_2){
          iChannel=1;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_3){
          iChannel=2;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_4){
          iChannel=3;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_5){
          iChannel=4;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_6){
          iChannel=5;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_7){
          iChannel=6;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_8){
          iChannel=7;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_9){
          iChannel=8;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_10){
          iChannel=9;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_11){
          iChannel=10;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_12){
          iChannel=11;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_13){
          iChannel=12;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_14){
          iChannel=13;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_15){
          iChannel=14;
          break;
        }else if(arrFeatures[i].getFeature()==CHANNEL_16){
          iChannel=15;
          break;
        }
      }
    }
  }
  return iChannel;
}


int processMidi_Pitch( uint8_t pBufMidi[] ){
  uint8_t iPitch = pBufMidi[1];
  bool bIsValidNote = false;
  bool bReprocessInput = true;
  int iNoteMapperOffset = 0;

  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
      if(arrFeatures[i].getFeature()==SCALE_HANDLER_FILTER){
        if( arrFeatures[i].isSelected() ){
          // Bei MAPPER wird die eingehende Note solange veraendert und neu verarbeitet, bis es passt.
          // Bei FILTER heisst es: Passt oder passt nicht.
          bReprocessInput = false;
        }
      }
    }
  }

  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].isSelected()){
      
      /*
        First we need to get the selected ROOT note. this will be taken as an offset: C=0, C#=1, etc.
        Scale transform: indices based on root note and iRootNoteOffset
      */
      if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE){
        // iPitch+12 in order to not run into negative values on lowest octave
        int tmpNote = (iPitch+12-iRootNoteOffset)%12;

        if(arrFeatures[i].getFeature()==SCALE_PASSTHRU){
          bIsValidNote = true;
        }else if(arrFeatures[i].getFeature()==SCALE_MAJOR){
          do{
            byte b = (tmpNote+iNoteMapperOffset)%12;
            if( (b == 0)||
                (b == 2)||
                (b == 4)||
                (b == 5)||
                (b == 7)||
                (b == 9)||
                (b == 11) ){
                  bIsValidNote = true;
                  bReprocessInput = false;
              }else{
                bIsValidNote = false;
                iNoteMapperOffset++;
              }
          }while(bReprocessInput == true);
          break;
        }else if(arrFeatures[i].getFeature()==SCALE_MINOR){
          do{
            byte b = (tmpNote+iNoteMapperOffset)%12;
            if( (b == 0)||
                (b == 2)||
                (b == 3)||
                (b == 5)||
                (b == 7)||
                (b == 8)||
                (b == 10) ){
                  bIsValidNote = true;
                  bReprocessInput = false;
              }else{
                bIsValidNote = false;
                iNoteMapperOffset++;
              }
          }while(bReprocessInput == true);
          break;
        }else if(arrFeatures[i].getFeature()==SCALE_PENTATONIC_MAJOR){
          do{
            byte b = (tmpNote+iNoteMapperOffset)%12;
            if( (b == 0)||
                (b == 2)||
                (b == 4)||
                (b == 7)||
                (b == 9)||
                (b == 12) ){
                  bIsValidNote = true;
                  bReprocessInput = false;
              }else{
                bIsValidNote = false;
                iNoteMapperOffset++;
              }
          }while(bReprocessInput == true);
          break;
        }else if(arrFeatures[i].getFeature()==SCALE_PENTATONIC_MINOR){
          do{
            byte b = (tmpNote+iNoteMapperOffset)%12;
            if( (b == 0)||
                (b == 3)||
                (b == 5)||
                (b == 7)||
                (b == 10)||
                (b == 12) ){
                  bIsValidNote = true;
                  bReprocessInput = false;
              }else{
                bIsValidNote = false;
                iNoteMapperOffset++;
              }
          }while(bReprocessInput == true);
          break;
        }
      }
    }
  }
  if( bIsValidNote==true ){
    return iPitch+iNoteMapperOffset;
  }else {
    return -1;
  }
}

void processNoteOn( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[0] - 0x90;
  int iPitch = pBufMidi[1];
  uint8_t iVelocity = pBufMidi[2];
  bool bSendOut = true;
  

  iVelocity = processMidi_Velocity( pBufMidi );
  iChannel = processMidi_Channel(pBufMidi, 0x90);
  iPitch = processMidi_Pitch( pBufMidi );

  if(iPitch>-1){
    digitalWrite(LED, 1);

    Serial.write( byte(0x90 + iChannel) );
    Serial.write( byte(uint8_t(iPitch)) );
    Serial.write( byte(iVelocity) );

    digitalWrite(LED, 0);
    Serial.flush();
  }
  
}

void processCC( uint8_t pBufMidi[] ){
  uint8_t iChannel = processMidi_Channel(pBufMidi, 0xB0);
  Serial.write( byte(0xB0 + iChannel) );
  Serial.write( pBufMidi[1] );
  Serial.write( pBufMidi[2] );
  Serial.flush();
}

void processPitchwheel( uint8_t pBufMidi[] ){
  uint8_t iChannel = processMidi_Channel(pBufMidi, 0xE0);
  Serial.write( byte(0xE0 + iChannel) );
  Serial.write( pBufMidi[1] );
  Serial.write( pBufMidi[2] );
  Serial.flush();
}

void processOther( uint8_t pBufMidi[] ){

  Serial.write( pBufMidi[0] );
  Serial.write( pBufMidi[1] );
  Serial.write( pBufMidi[2] );
  Serial.flush();
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

void showPreset(uint8_t pPresetID){
  oled.setFont(menuFont);
  oled.clear();
  oled.set2X();
  oled.println("Preset " + String(pPresetID));

}

void showMenu(String pLine1, String pLine2, String pLine3) {
  //oled.setFont(menuFont);
  oled.set1X();
  oled.setFont(TimesNewRoman16);
  oled.clear();
  oled.println( " " + pLine1 );
  oled.setFont(TimesNewRoman16_bold);
  oled.println( /*"*" +*/ pLine2 );
   oled.setFont(TimesNewRoman16);
  oled.println( " " + pLine3 );

  if(iActivePreset!=127){
    oled.print("                     [ Preset:" + String(iActivePreset) + " ]");
  }
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
    //return "Scale";
    return "";  //because 'Major, Minor, etc is unique by itself and saves string data
  }if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
    return "Root";
  }if(arrFeatures[pIndex].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
    return "";
  }else{
    return "";
  }
}


void SerialPrintln(String p){
  Serial.println(p);
}

/*
  Presets are organized byte-wise: The selected item of every individual feature_group is stored in 
  a dedicated byte. As of version 0.5 that's 4 bytes for the complete config. Order of storing:
  FEATURE_GROUP_VELOCITY, SCALE, ROOTNOTE, SCALEHANDLER, CHANNEL.

  The byte's value marks the INDEX of the selected item. All bytes 0 means: 
  Every first item of every FEATURE_GROUP is selected (default).

*/
void loadPreset(uint8_t pPresetIndex){


  uint8_t iAddress;

  if(pPresetIndex == 0){
    iAddress = PRESET_0_BASE_ADDRESS;
  }else if(pPresetIndex == 1){
    iAddress = PRESET_1_BASE_ADDRESS;
  }else if(pPresetIndex == 2){
    iAddress = PRESET_2_BASE_ADDRESS;
  }else if(pPresetIndex == 3){
    iAddress = PRESET_3_BASE_ADDRESS;
  }

  // How many items of every feature do we have?
  uint8_t iFeaturecount_velocity = 0;
  uint8_t iFeaturecount_scale = 0;
  uint8_t iFeaturecount_rootnote = 0;
  uint8_t iFeaturecount_channel = 0;
  uint8_t iFeatureCount_scalehandler = 0;

  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
      iFeaturecount_velocity++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE){
      iFeaturecount_scale++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
      iFeaturecount_rootnote++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
      iFeatureCount_scalehandler++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
      iFeaturecount_channel++;
    }
  }

  uint8_t val;

  val = EEPROM.read(iAddress + 0);
  if(val!=255){ // a.k.a. hier wurde schonmal etwas gespeichert
    for(uint8_t j=0; j<FEATURECOUNT; j++){
      if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
        arrFeatures[j].select(false);
      }
    }
    arrFeatures[val + 0].select(true);
  }

  val = EEPROM.read(iAddress + 1);
  if(val!=255){ 
    for(uint8_t j=0; j<FEATURECOUNT; j++){
      if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_SCALE){
        arrFeatures[j].select(false);
      }
    }
    arrFeatures[val + iFeaturecount_velocity].select(true);
  }

  val = EEPROM.read(iAddress + 2);
  if(val!=255){ 
    for(uint8_t j=0; j<FEATURECOUNT; j++){
      if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
        arrFeatures[j].select(false);
      }
    }
    arrFeatures[val + iFeaturecount_velocity + iFeaturecount_scale].select(true);
  }

  val = EEPROM.read(iAddress + 3);
  if(val!=255){ 
    for(uint8_t j=0; j<FEATURECOUNT; j++){
      if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
        arrFeatures[j].select(false);
      }
    }
    arrFeatures[val + iFeaturecount_velocity + iFeaturecount_scale + iFeaturecount_rootnote].select(true);
  }

  val = EEPROM.read(iAddress + 4);
  if(val!=255){ 
    for(uint8_t j=0; j<FEATURECOUNT; j++){
      if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
        arrFeatures[j].select(false);
      }
    }
    arrFeatures[val + iFeaturecount_velocity + iFeaturecount_scale + iFeaturecount_rootnote + iFeatureCount_scalehandler].select(true);
  }

  showPreset(pPresetIndex);
  iActivePreset = pPresetIndex;

}





void savePreset(uint8_t pPresetIndex){
  uint8_t iAddress;
  if(pPresetIndex == 0){
    iAddress = PRESET_0_BASE_ADDRESS;
  }else if(pPresetIndex == 1){
    iAddress = PRESET_1_BASE_ADDRESS;
  }else if(pPresetIndex == 2){
    iAddress = PRESET_2_BASE_ADDRESS;
  }else if(pPresetIndex == 3){
    iAddress = PRESET_3_BASE_ADDRESS;
  }

  // How many items of every feature do we have?
  uint8_t iFeaturecount_velocity = 0;
  uint8_t iFeaturecount_scale = 0;
  uint8_t iFeaturecount_rootnote = 0;
  uint8_t iFeaturecount_channel = 0;
  uint8_t iFeaturecount_scalehandler = 0;

  for(uint8_t i=0; i<FEATURECOUNT; i++){
    if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
      iFeaturecount_velocity++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE){
      iFeaturecount_scale++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
      iFeaturecount_rootnote++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
      iFeaturecount_scalehandler++;
    }else if(arrFeatures[i].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
      iFeaturecount_channel++;
    }
  }

  for(uint8_t j=0; j<FEATURECOUNT; j++){
    if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_VELOCITY){
      if(arrFeatures[j+0].isSelected()==true){
        EEPROM.update(iAddress+0, j);
      }
    }
  }

  for(uint8_t j=0; j<FEATURECOUNT; j++){
    if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_SCALE){
      if(arrFeatures[j].isSelected()==true){
        EEPROM.update(iAddress+1, j-iFeaturecount_velocity);
      }
    }
  }

  for(uint8_t j=0; j<FEATURECOUNT; j++){
    if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_ROOTNOTE){
      if(arrFeatures[j].isSelected()==true){
        EEPROM.update(iAddress+2, j - iFeaturecount_velocity - iFeaturecount_scale );
      }
    }
  }

  for(uint8_t j=0; j<FEATURECOUNT; j++){
    if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_SCALE_HANDLER){
      if(arrFeatures[j].isSelected()==true){
        EEPROM.update(iAddress+3, j - iFeaturecount_velocity - iFeaturecount_scale - iFeaturecount_rootnote);
      }
    }
  }

  for(uint8_t j=0; j<FEATURECOUNT; j++){
    if(arrFeatures[j].getFeatureGroup()==FEATURE_GROUP_CHANNEL){
      if(arrFeatures[j].isSelected()==true){
        EEPROM.update(iAddress+4, j - iFeaturecount_velocity - iFeaturecount_scale - iFeaturecount_rootnote - iFeaturecount_scalehandler);
      }
    }
  }


  showMenu("", "Save Preset " + String(pPresetIndex), "");
  delay(1000);
  oled.clear();
  delay(10);
  showMenu(getPreviousMenuItem(iMenuPosition), getMenuItem( iMenuPosition ), getNextMenuItem(iMenuPosition));

}
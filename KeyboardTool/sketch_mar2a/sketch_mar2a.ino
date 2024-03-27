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



USB Usb;
USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

#define VERSION "0.2"
#define MAX_RESET 8 //MAX3421E pin 12
#define MAX_GPX   9 //MAX3421E pin 17

#define ENCODER_A 13
#define ENCODER_B 14
#define ENCODER_CLICK 15

// 0X3C+SA0 - 0x3C or 0x3D
#define DISPLAY_I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

CD74HC4067 mux(2, 3, 4, 5);  // S0, S1, S2, S3.  EN->GND
#define DEMUX_PIN A0 // S16
bool muxValue[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  // The value of the Buttons as read from the multiplexer

bool bEncoderCLick = muxValue[15];

const String MNU_VELOCITY = "Velocity";
const String MNU_VELOCITY_PASSTHRU = "Passthru";
const String MNU_VELOCITY_FIX = "Fix";
const String MNU_VELOCITY_RANDOMFIX = "Fix+Random";
const String MNU_SCALE = "Scale";
const String MNU_CHANNEL = "Channel";
String sMenu[] = {"Velocity", ""};

int iEncoderValue = 0;

void onInit()
{
  char buf[20];
  uint16_t vid = Midi.idVendor();
  uint16_t pid = Midi.idProduct();
  sprintf(buf, "VID:%04X, PID:%04X", vid, pid);
  Serial.println(buf); 
}

void setup()
{
  pinMode(DEMUX_PIN, INPUT);
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Serial init'd");
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

  Serial.println("Ready");
  showInfo(3000);
}

void loop()
{
  readMux();
  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }

  iEncoderValue = queryEncoder();
  if(iEncoderValue!=0){
    Serial.println( iEncoderValue );
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
    Serial.print(buf);
  }
  Serial.println("");
}

void processNoteOff( uint8_t pBufMidi[] ){
  // tbd
}

void processNoteOn( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0x90;
  uint8_t iPitch = pBufMidi[2];
  uint8_t iVelocity = pBufMidi[3];
  if(iVelocity>0){
    //Serial.println("Note ON. Channel:" + String(iPitch) + " " + String(iChannel) );
    displayIncoming( pBufMidi );
  }
}

void processCC( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0xB0;
  uint8_t iController = pBufMidi[2];
  uint8_t iValue = pBufMidi[3];
  //Serial.println("CC. Channel:" + String(iChannel) + " CC:" + String(iController)+ " Value:" + String(iValue) );
}

void showInfo(int pWaitMS) {
  oled.setFont(ZevvPeep8x16);
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
  //Serial.println(String(muxValue[0]));
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
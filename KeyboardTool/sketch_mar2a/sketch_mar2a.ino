/*
  KeyboardTool
  Based on mini USB host shield
  Takes a MIDI input from a USB MIDI Keyboard, processes it and sends it out via MIDI socket.

    -velocity fixer: converts incoming velocity to a fix value
    -velocity booster: multiplies a note's velocity by a given factor (for keyboards with no adjustable velcity-curve)
    -scaler 1: only processes notes that fit into a selected scale
    -scaler 2: maps every incoming note to the nearest note of a selected scale

*/
#include <usbh_midi.h>
#include <usbhub.h>

USB Usb;
USBHub Hub(&Usb);
USBH_MIDI  Midi(&Usb);

#define MAX_RESET 7 //MAX3421E pin 12
#define MAX_GPX   8 //MAX3421E pin 17

void MIDI_poll();

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
  Serial.println("Ready");
}

void loop()
{
  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t bufMidi[MIDI_EVENT_PACKET_SIZE];
  uint16_t  rcvd;

  if (Midi.RecvData( &rcvd,  bufMidi) == 0 ) {
    processData(bufMidi);
  }
}

void processData( uint8_t pBufMidi[] ){
  printData(pBufMidi);
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
    Serial.println("Note ON. Channel:" + String(iChannel) );
  }
}

void processCC( uint8_t pBufMidi[] ){
  uint8_t iChannel = pBufMidi[1] - 0xB0;
  uint8_t iController = pBufMidi[2];
  uint8_t iValue = pBufMidi[3];
  //Serial.println("CC. Channel:" + String(iChannel) + " CC:" + String(iController)+ " Value:" + String(iValue) );
}
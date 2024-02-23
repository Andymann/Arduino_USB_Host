/*

ioreg|grep "Maschine"
    | |   |   | +-o Maschine Controller MK2@14100000  <class IOUSBHostDevice, id 0x100092b3c, registered, matched, active, busy 0 (113 ms), retain 65>
    | |   |   |   +-o Maschine Controller MK2 Audio Ctrl@0  <class IOUSBHostInterface, id 0x100092b45, registered, matched, active, busy 0 (11 ms), retain 6>
    | |   |   |   +-o Maschine Controller MK2 MIDI@1  <class IOUSBHostInterface, id 0x100092b46, registered, matched, active, busy 0 (8 ms), retain 12>
    | |   |   |   +-o Maschine Controller MK2 HID@2  <class IOUSBHostInterface, id 0x100092b47, registered, matched, active, busy 0 (20 ms), retain 13>
    | |   |   |   +-o Maschine Controller MK2 DFU@3  <class IOUSBHostInterface, id 0x100092b48, registered, matched, active, busy 0 (9 ms), retain 6>
    | |     +-o Maschine Controller MK2@14100000  <class AppleUSBDevice, id 0x100092b3e, registered, matched, active, busy 0 (3 ms), retain 23>
    | |       +-o Maschine Controller MK2 Audio Ctrl@0  <class AppleUSBInterface, id 0x100092b49, registered, matched, active, busy 0 (0 ms), retain 5>
    | |       +-o Maschine Controller MK2 MIDI@1  <class AppleUSBInterface, id 0x100092b4b, registered, matched, active, busy 0 (0 ms), retain 6>
    | |       +-o Maschine Controller MK2 HID@2  <class AppleUSBInterface, id 0x100092b4c, registered, matched, active, busy 0 (0 ms), retain 5>
    | |       +-o Maschine Controller MK2 DFU@3  <class AppleUSBInterface, id 0x100092b4d, registered, matched, active, busy 0 (0 ms), retain 5>

*/



#include <hidcomposite.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB     Usb;
USBHub Hub(&Usb);


//USBH_MIDI  Midi(&Usb);

//Revision 1.3 (DEV-09947)
#define MAX_RESET 7 //MAX3421E pin 12
#define MAX_GPX   8 //MAX3421E pin 17

// Override HIDComposite to be able to select which interface we want to hook into
class HIDSelector : public HIDComposite
{
public:
    HIDSelector(USB *p) : HIDComposite(p) {};

protected:
    void ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf); // Called by the HIDComposite library
    bool SelectInterface(uint8_t iface, uint8_t proto);
};

// Return true for the interface we want to hook into
bool HIDSelector::SelectInterface(uint8_t iface, uint8_t proto)
{
  if (proto != 0){
    Serial.println("Proto: true");
    return true;
  }
    

  return true;
}

// Will be called for all HID data received from the USB interface
void HIDSelector::ParseHIDData(USBHID *hid, uint8_t ep, bool is_rpt_id, uint8_t len, uint8_t *buf) {
//Serial.println("From USB");
#if 1
  if (len && buf)  {
    Notify(PSTR("\r\n"), 0x80);
    for (uint8_t i = 0; i < len; i++) {
      D_PrintHex<uint8_t > (buf[i], 0x80);
      Notify(PSTR(" "), 0x80);
    }
  }
#endif
}

HIDSelector    hidSelector(&Usb);

void setup()
{
  UsbDEBUGlvl = 0xff;
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
  //Midi.attachOnInit(onInit);
  Serial.println("Ready");
}



void loop()
{
  Usb.Task();
}


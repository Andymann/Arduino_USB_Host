# Arduino_USB_Host
  

Using Arduino and ESP32 as USB Host.
The code uses the infamous mini USB host shield from Aliexpress, etc..  
  

The image below shows the shield with the uncut trace to the '+' side of the USB connector. This means that the connected USB device is powered with 3.3 Volt only. Some devices work, some don't. An acitve (i.e. externally powered) USB hub can help in these cases. You can,  of course, also modify the shield by cutting the trace and providing externally sourced 5V to the USB connector.
</br>
</br>  
Devices I found working with the unmodified shield:
</br>

- Akai APC mini  
</br>
</br>
</br>

Pinout  
  
|PIN      | Arduino DUE        | Mini USB Host Shield | ESP32 |
|:------- | :----------------: | :------------------: | ----: |
| Reset   | D7                 | 15                   | 15    |
| GPX     | D8                 | 27                   |       |
| Mosi    | D11                | 2                    | 23    |
| Miso    | D12                | 3                    | 19    |
| SCK     | D13                | 4                    | 18    |
| SS      | D10                | 1                    | 5     |
|         | GND                | 11, 16               | GND   |
|         | 3v3                | 9                    | 3v3   |  
  
  
<img src="https://github.com/Andymann/Arduino_USB_Host/blob/main/Mini%20USB%20Host%20Shield%20Pinout.png" />
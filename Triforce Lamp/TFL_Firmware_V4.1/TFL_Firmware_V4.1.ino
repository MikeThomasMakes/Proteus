/* 
Mike Thomas 2015
06/25/15
ThomasTesla.com
All rights reserved.

Triforce Lamp Firmware 3.0
Neopixel Library + SoftwareSerial + ParseInt
Takes in data from bluetooth app to control Neopixel LEDs

Triforce Lamp App outputs data as follows:
0:255,255,255\r\n
1:255,255,255\r\n
2:255,255,255

Status: Restructuring Void Loop caused bluetooth apps to lose control.
To Do; Fix basic functionality and find a way to implement a constant rainbow display.
*/

#include <avr/io.h>
#include <EEPROM.h>

#include <Adafruit_NeoPixel.h>
#define PIN            1
#define NUMPIXELS      3
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include <SoftwareSerial.h>
SoftwareSerial mySerial(4,3); // (RX,TX) Sets physical pin 2 as TX and physical pin 3 as RX

//Initialize values to 0
//int pixel0 = 0;
int pixel1 = 1;
int pixel2 = 2;
int x = 1;

//Read last color combination. Note: Locations that have never been written to have the value of 255.
int red0 = EEPROM.read(0);
int green0 = EEPROM.read(1);
int blue0 = EEPROM.read(2);
int red1 = EEPROM.read(3);
int green1 = EEPROM.read(4);
int blue1 = EEPROM.read(5);
int red2 = EEPROM.read(6);
int green2 = EEPROM.read(7);
int blue2 = EEPROM.read(8);
int pixel0 = EEPROM.read(9);

//Temp color values
int temp_red0 = 0;
int temp_green0 = 0;
int temp_blue0 = 0;
int temp_pixel1;
int temp_red1 = 0;
int temp_green1 = 0;
int temp_blue1 = 0;
int temp_pixel2 = 0;
int temp_red2 = 0;
int temp_green2 = 0;
int temp_blue2 = 0;

void setup() {
  mySerial.begin(9600);
  pixels.begin(); //Initializes NeoPixel library
  
  //Set each pixel to the saved colors. Should be white on first power up.
  pixels.setPixelColor(0, pixels.Color(red0,green0,blue0)); 
  pixels.setPixelColor(1, pixels.Color(red1,green1,blue1));
  pixels.setPixelColor(2, pixels.Color(red2,green2,blue2));
  pixels.show(); // This sends the updated pixel color to the hardware.
  delay(100);
}

void loop() {
    
  //18 bytes are sent: Add End of Transmission (EoT) character
  if(mySerial.available() > 18){ //Does everything need to be nested in this? I don't think so...
    
      start: //Rainbow sequence breaks to this location
    
      //Parse all serial data into integers, separated by non-digit characters
      pixel0 = mySerial.parseInt();
    
      if( pixel0 == 0 ){ //Only updates color values if a normal string is sent
        red0 = mySerial.parseInt();
        green0 = mySerial.parseInt();
        blue0 = mySerial.parseInt();
    
        pixel1 = mySerial.parseInt();
        red1 = mySerial.parseInt();
        green1 = mySerial.parseInt();
        blue1 = mySerial.parseInt();
    
        pixel2 = mySerial.parseInt();
        red2 = mySerial.parseInt();
        green2 = mySerial.parseInt();
        blue2 = mySerial.parseInt();
      }
      else{ //
        temp_red0 = mySerial.parseInt();
        temp_green0 = mySerial.parseInt();
        temp_blue0 = mySerial.parseInt();
    
        temp_pixel1 = mySerial.parseInt();
        temp_red1 = mySerial.parseInt();
        temp_green1 = mySerial.parseInt();
        temp_blue1 = mySerial.parseInt();
    
        temp_pixel2 = mySerial.parseInt();
        temp_red2 = mySerial.parseInt();
        temp_green2 = mySerial.parseInt();
        temp_blue2 = mySerial.parseInt();
      }
        
      pixels.setPixelColor(0, pixels.Color(red0,green0,blue0)); // Changes color with app control
      pixels.setPixelColor(1, pixels.Color(red1,green1,blue1)); // Changes color with app control
      pixels.setPixelColor(2, pixels.Color(red2,green2,blue2)); // Changes color with app control
      pixels.show(); // This sends the updated pixel color to the hardware. 
      delay(50); 
    
      //Save color values to EEPROM (100,000 read/write cycles > Lasts 137 years at 2 cycles per day)
      //.update only writes if the value is different. Will prevent some unnecessary write cycles.
      //How will the rainbow sequence affect this? Do not mess with rgb colors. Use a different method to invoke rainbow. Use pixel numbers?
      EEPROM.update(0,red0);
      EEPROM.update(1,green0);
      EEPROM.update(2,blue0);
      EEPROM.update(3,red1);
      EEPROM.update(4,green1);
      EEPROM.update(5,blue1);
      EEPROM.update(6,red2);
      EEPROM.update(7,green2);
      EEPROM.update(8,blue2);
      EEPROM.update(9,pixel0);
      } 
   
  //Set to Rainbow Mode
  if( pixel0 == 1 ){ //"if rainbow button is pressed"
    //Rainbow Start
    uint16_t i, j;
    for(j=0; j<256; j++) {
      for(i=0; i<pixels.numPixels(); i++) {
        pixels.setPixelColor(i, Wheel((i+j) & 255));
      }
      pixels.show();
      delay(50);
      if(mySerial.available()){goto start;} //This breaks the rainbow sequence PERFECTLY.
    }
    //Rainbow End
  }
  
  //Turn on/off Strobe (Freezes LEDs after break, first pixel doesn't strobe or light)
  if( pixel0 == 2 ){ //if Strobe: This should repeatedly wipe the lamp and reset it to the previously set colors. Doubt Rainbow will work.
    while(1){ //Until interrupted by serial data.
      //Turn off
      colorWipe(pixels.Color(0, 0, 0), 10); // Off
      delay(50); //Strobe frequency (0-255)    
      
      //Turn on
      pixels.setPixelColor(0, pixels.Color(red0,green0,blue0));
      pixels.setPixelColor(1, pixels.Color(red1,green1,blue1));
      pixels.setPixelColor(2, pixels.Color(red2,green2,blue2));
      pixels.show(); // This sends the updated pixel color to the hardware.
      delay(50);
      
      //Break
      if(mySerial.available()){goto start;} //This breaks the sequence. Taken from rainbow break
    }
  }
  
  if(pixel0 == 3){ //if Test
    // Some example procedures showing how to display to the pixels:
    colorWipe(pixels.Color(255, 0, 0), 100); // Red
    colorWipe(pixels.Color(0, 255, 0), 100); // Green
    colorWipe(pixels.Color(0, 0, 255), 100); // Blue
    colorWipe(pixels.Color(255, 255, 0), 100); //Yellow
    delay(50);
    if(mySerial.available()){goto start;} //Break
    
    //TheaterChase White Start
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, pixels.Color(255, 255, 255));    //turn every third pixel on
          if(mySerial.available()){goto start;} //Break
        }
        pixels.show();
        delay(50);
     
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
          if(mySerial.available()){goto start;} //Break
        }
      }
    }
    //TheaterChase White End
        
    //TheaterChase Red Start
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, pixels.Color(255, 0, 0));    //turn every third pixel on
          if(mySerial.available()){goto start;} //Break
        }
        pixels.show();
        delay(50);
     
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
          if(mySerial.available()){goto start;} //Break
        }
      }
    }
    //TheaterChase Red End
    
    //TheaterChase Green Start
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, pixels.Color(0, 255, 0));    //turn every third pixel on
          if(mySerial.available()){goto start;} //Break
        }
        pixels.show();
        delay(50);
     
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
          if(mySerial.available()){goto start;} //Break
        }
      }
    }
    //TheaterChase Green End
    
    //TheaterChase Blue Start
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, pixels.Color(0, 0, 255));    //turn every third pixel on
          if(mySerial.available()){goto start;} //Break
        }
        pixels.show();
        delay(50);
     
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
          if(mySerial.available()){goto start;} //Break
        }
      }
    }
    //TheaterChase Blue End
    
    //TheaterChase Yellow Start
    for (int j=0; j<10; j++) {  //do 10 cycles of chasing
      for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, pixels.Color(255, 255, 0));    //turn every third pixel on
          if(mySerial.available()){goto start;} //Break
        }
        pixels.show();
        delay(50);
     
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
          if(mySerial.available()){goto start;} //Break
        }
      }
    }
    //TheaterChase Yellow End
    
    //Rainbow Start
    uint16_t i, j;
    for(j=0; j<256; j++) {
      for(i=0; i<pixels.numPixels(); i++) {
        pixels.setPixelColor(i, Wheel((i+j) & 255));
      }
      pixels.show();
      delay(50);
      if(mySerial.available()){goto start;} //Break
    }
    //Rainbow End
    
    //Rainbow Cycle Start
    uint16_t k, m;
    for(m=0; m<256*5; m++) { // 5 cycles of all colors on wheel
      for(k=0; k< pixels.numPixels(); k++) {
        pixels.setPixelColor(k, Wheel(((k * 256 / pixels.numPixels()) + m) & 255));
        if(mySerial.available()){goto start;} //Break
      }
      pixels.show();
      delay(50);
    }
    //Rainbow Cycle End
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, c);
      pixels.show();
      delay(wait);
  }
}

/*void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels.show();
    delay(wait);
    //if(mySerial.available()){goto start;} //This breaks the rainbow sequence PERFECTLY.
  }
}

//Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, c);    //turn every third pixel on
      }
      pixels.show();
     
      delay(wait);
     
      for (int i=0; i < pixels.numPixels(); i=i+3) {
        pixels.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        pixels.show();
       
        delay(wait);
       
        for (int i=0; i < pixels.numPixels(); i=i+3) {
          pixels.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}*/

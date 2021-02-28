#include <FlashStorage.h>
#include <MKRGSM.h>
#include <Arduino_MKRGPS.h>
#include <TinyGPS++.h>

// define pins
#define Input1Pin 2
#define Input2Pin 3
#define VoltagePin A0
#define Output1Pin 0
#define Output2Pin 1

// initialize the library instance
GPRS gprs;
GSM gsmAccess;
GSM_SMS sms;
TinyGPSPlus gps;

// GPS helper variables
double myLatitude;
double myLongitude;
double myLocation[2];
int Radius;

//Data to save to persistent memory
typedef struct {
  boolean init; // whether unit is initialized
  char number[20]; // master number
  char pin[4]; // PIN
  boolean geo; // whether geofence is on
  int radius; // geofence radius
  double location[2]; // gps coordinates
} bbData;

// initialize flash storage space
FlashStorage(myFlash, bbData);

// initialize an instance of type bbData to store data
bbData myBB;

// help message
char helpMsg[] = "BilgeBuddy help:\n"
"H - display this message\n"
"G - toggle GeoFence\n"
"O - turn on output\n"
"V - display voltage\n"
"S - set/reset master number\n"
"R - customize GeoFence radius";

//The connectNetwork() function is used for the board data connection
void connectNetwork() {
  bool connected = false;

  // Start GSM connection
  while (!connected) {
    if (gsmAccess.begin() == GSM_READY) {
      connected = true;
    } else {
      Serial.println("GSM Not connected");
      delay(1000);
    }
  }
}

// function prototypes
void doCommand(char command);
void SendSMS(char txtMsg[]);
void numberSet(char number[20]);
void numberReSet(char number[20]);
void outputOn(void);
void geoToggle(void);
void voltageShow(void);
void radiusSet(void);
void measureLocation(int readings);

void setup() {
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  // initialize serial1 for GPS
  Serial1.begin(9600);
  while (!Serial) {}

  // connect to GSM network
  connectNetwork();

  // load data from flash storage into ram
  myBB = myFlash.read();
}

void loop() {
  // array for sender number
  char senderNumber[20];

  // If there are any SMSs available()
  if (sms.available()) {

    // save remote number to array
    sms.remoteNumber(senderNumber, 20);

    // if unit has NOT yet been initialized
    if (!myBB.init) {
      // if command is 'S', set sender number
      if (sms.peek() == 'S')
        numberSet(senderNumber); // set sender number as master number
    }

    // if unit HAS been initialized
    else {
      // check that sender number matches master number
      for (int i = 0; i<20; i++) {
        // if it doesn't match
        if (senderNumber[i] != myBB.number[i]) {
          // and if the command is 'S', reset number
          if (sms.peek() == 'S')
            numberReSet(senderNumber);
          else
            break;
          }
        }
      doCommand(sms.peek());
      }
    // Delete message from modem memory
    sms.flush();
  }

  if (myBB.geo == true) {
    while (Serial1.available() > 0){
      gps.encode(Serial1.read());
    }
    measureLocation(5);
    //Serial.println("Location:\n" + String(myLatitude));
    //Calculate distance between set location and current location
    double Distance =
    gps.distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      myLocation[0],
      myLocation[1]);
    if (Distance > myBB.radius){
      sms.beginSMS(myBB.number);
      sms.print("Warning! GeoFence Breached! Distance from location is ");
      sms.print(Distance);
      sms.print(" Meters.");
      sms.endSMS();

      delay(30000);
    }
  }

  // Automated input/output interface

  //wait a second
  delay(1000);
}

/***** FUNCTIONS *****/

void doCommand(char command) {
  switch (command) {
        case 'H':
          SendSMS(helpMsg);
          break;
        case 'G':
          geoToggle();
          break;
        case 'O':
          outputOn();
          break;
        case 'V':
          voltageShow();
          break;
        case 'S':
          SendSMS("Number already set");
          break;
        case 'R':
          radiusSet();
          break;
        default:
          SendSMS("Invalid command.");
          break;
  }
}


//Send SMS
void SendSMS(char txtMsg[]){
  sms.beginSMS(myBB.number);
  sms.print(txtMsg);
  sms.endSMS();
}


// set (i.e. initially) master number
void numberSet(char number[20]) {
  // load digits into myBB struct
  for (int i = 0; i<20; i++) {
    myBB.number[i] = number[i];
  }

  // make sure SMS buffer is empty
  sms.flush();

  // prompt for PIN
  SendSMS("Please set 4-character PIN");

  // Get PIN from SMS
  if (sms.available()) {
    for (int i = 0; i<4; i++) {
      myBB.pin[i] = sms.read();
    }
  }

  // clear buffer
  sms.flush();

  // prompt for confirmation
  SendSMS("Please confirm PIN");

  // get PIN from SMS
  if (sms.available()) {
    for (int i = 0; i<4; i++) {
      // make sure PINs match
      if (myBB.pin[i] != sms.read()) {
        SendSMS("PIN's do not match");
        sms.flush();
        break;
      }
      else {
        continue;
      }
    }
  }
  // write number and PIN to flash
  myFlash.write(myBB);
  SendSMS("Device initialized!");
}

void numberReSet(char number[20]) {
  // prompt for PIN
  SendSMS("Please enter PIN");

  // get PIN from SMS
  if (sms.available()) {
    // check PIN
    for (int i = 0; i<4; i++) {
      if (myBB.pin[i] != sms.read()) {
        SendSMS("Invalid PIN");
        sms.flush();
        break;
      }
      else {
        // if PIN is correct
        for (int i = 0; i<20; i++) {
          // set load sender number into SRAM
          myBB.number[i] = number[i];
        }
        // clear buffer
        sms.flush();
        sms.beginSMS(myBB.number);
        sms.print("Reset number to: ");
        sms.print(myBB.number);
        sms.endSMS();
      }
    }
  }

  // load new number to flash storage
  myFlash.write(myBB);
}

void outputOn(void) {
  // prompt for output duration
  SendSMS("Turn on for how many seconds?");
  // clear text buffer
  sms.flush();
  // initialize input array
  char input[3];
  // get input from SMS
  if (sms.available()) {
    for (int i=0; i<3; i++) {
      input[i] = sms.read();
    }
    // convert input to int
    int seconds = atoi(input);
    // turn on output
    digitalWrite(Output1Pin, HIGH);
    // wait specified duration
    delay(seconds * 1000);
    // turn off output
    digitalWrite(Output1Pin, LOW);
    SendSMS("Output Off");
  }
}

void geoToggle(void) {
  // switch geofence boolean
  myBB.geo = !myBB.geo;
  // if GeoFence is now on
  if (myBB.geo) {
    SendSMS("GeoFence On");
    // get location
    measureLocation(5);
    // write location to data
    myBB.location[0] = myLocation[0];
    myBB.location[1] = myLocation[1];
    // backup location in flash
    myFlash.write(myBB);
  }
  else {
    SendSMS("GeoFence Off");
  }

}

void radiusSet(void) {
  SendSMS("Please enter new GeoFence radius:");
  // clear text buffer
  sms.flush();
  // initialize input array
  char input[3];
  // get input from SMS
  if (sms.available())
  {
    for (int i = 0; i < 3; i++)
    {
      input[i] = sms.read();
    }
    // convert input to int
    int feet = atoi(input);
    
    // write to memory
    myBB.radius = feet;
    myFlash.write(myBB);
  }
}

void voltageShow(void) {
  //Send battery voltage
  float voltage = analogRead(VoltagePin) / 74.20; //GND = 0, 5V = 1024 (13.8V Max)
  //Serial.println(voltage);
  sms.beginSMS(myBB.number); //dont always have this
  sms.print("Battery Voltage: ");
  sms.print(voltage);
  sms.print(" V");
  sms.endSMS();
}

void measureLocation(int readings) {
  unsigned long timeout = millis();
  int i = 0;
  boolean Success = false;

  Serial.println("Searching for satellites...");
  while (Success == false) {
    while (Serial1.available() > 0){
      gps.encode(Serial1.read());
    }

    Serial.println(gps.satellites.value());

    //Wait for satellites to be found
    if (gps.satellites.value() > 4) {
      Serial.println("Measuring Location...");
      //take multiple readings
      while (i < readings) {
        myLatitude = myLatitude + gps.location.lat();
        Serial.println(myLatitude);
        myLongitude = myLongitude + gps.location.lng();
        i++;
        delay(5000);
      }
      Success = true;
    }
    delay(5000);
  }
  //Average location readings
  myLocation[0] = myLatitude / readings;
  myLocation[1] = myLongitude / readings;
}

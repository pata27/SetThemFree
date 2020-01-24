//Importing librairies
#include <Arduino.h>

#include <FastCRC.h>

FastCRC8 CRC8;

//Variables can be change to activate or desactivate some options

<<<<<<< Updated upstream
bool powerCutdown = false;
bool capacitiveSen = false;
bool button = true;
=======
#define POWERSENS //put double slash in front off this line to turn off, if not, will be on
#define BUTTON  //put double slash in front off this line to turn off, if not, will be on
//#define CAPACITIV   //put double slash in front off this line to turn off, if not, will be on
#define NFCBOARD
const byte rightUid[] = {0x00, 0x00, 0x00, 0x00};
>>>>>>> Stashed changes

bool Turbo = 1; //1 for on, 0 for off
bool seconddigit = 1; //Still unknown function
bool fastAcceleration = 1; //1 for on, 0 for off
bool KPH = 1; //1 for KPH, 0 for MPH
bool fifthdigit = 0; //Still unknown function
bool Light = 0; //1 for on, 0 for off
bool LightBlink = 0; //1 for on, 0 for off
bool ESCOn = 1; //1 for on, 0 for off
int SpeedLimit = 255; //Beetwen 0 and 255
<<<<<<< Updated upstream
int sensivity = 200; //To ajust the sensivity of the capacitive Sensor
//Do not change variables after that if you don't know what you're doing
=======
const int sensivity = 200; //To ajust the sensivity of the capacitive Sensor
>>>>>>> Stashed changes

//Do not change variables after that if you don't know what you're doing

//Pin definitions for externals things
int buzzer = A0;
int ledPin = 13;
int buttonPin = 2;
int powerLoss = A4;

<<<<<<< Updated upstream
=======
#ifdef CAPACITIV
#include <CapacitiveSensor.h>
>>>>>>> Stashed changes
CapacitiveSensor capSens = CapacitiveSensor(4, 6);

<<<<<<< Updated upstream
=======
#ifdef NFCBOARD
#include <PN5180.h>
#include <PN5180ISO15693.h>
#define PN5180_NSS  10
#define PN5180_BUSY 9
#define PN5180_RST  7
PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);
#endif

#if defined(CAPACITIV) && defined(BUTTON)
#warning "Two similar types of inputs at the same time ?"
#endif
>>>>>>> Stashed changes

//Program variables
int lastButtonState = 1; //Used to record the last button state
volatile int buttonFlag; //Use in the interup for the button
int debounceTime = 20;  //Time of debouncing, valid for button and capacitive sensor
int debounceCapacitive = 0; //Used to debounce the capacitive sensor
int debounceButton = 0;
int counter = 0;  //Used to count 500 loops (of 1ms) before sending trame to the scooter
bool change; //Used to bypass the counter if any changes in the 4th bytes have been done, to be send to the scooter as quick as possible
int speeddef = SpeedLimit;

int forth;  //Decimal value of the 4th byte
byte code[6] = {0xA6, 0x12, 0x02};  //Trame send to unlock the scooter, the 3 other bytes are added later on the code
byte bye[] = {0xA6, 0x12, 0x02, 0x00, 0x00, 0xDF};  //Trame send to lock the scooter if powerdown sensor is used, avoid 20s of latency between turn off and actual shutdown

void setup() {
  Serial.begin(9600); //Initialize the serial communication with the scooter
  calculateforth(); //Calculate the 4th byte depending on the options choosed
<<<<<<< Updated upstream
  pinMode(ledPin, OUTPUT);
  if (button == true) {
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), ISR_button, CHANGE); //Interupt is used to see if the button is pressed (if button used)
  }
  tone(buzzer, 440, 40); //Make a bip with the buzzer at startup (if beeper wired)
=======
  pinMode(LEDPIN, OUTPUT);
#ifdef BUTTON
  pinMode(BUTTONPIN, INPUT_PULLUP);
#endif
  tone(BUZZER, 440, 40); //Make a bip with the buzzer at startup (if beeper wired)
#ifdef NFCBOARD
  nfc.begin();
  nfc.reset();
  uint8_t productVersion[2];
  nfc.readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion));
  if (0xff == productVersion[1]) { // if product version 255, the initialization failed
    tone(BUZZER, 220, 200);
    delay(400);
    tone(BUZZER, 220, 200);
    delay(400);
    tone(BUZZER, 220, 200);
    while (1);
  }
  uint8_t firmwareVersion[2];
  nfc.readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion));
  uint8_t eepromVersion[2];
  nfc.readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion));
  nfc.setupRF();
  uint32_t loopCnt = 0;
  bool errorFlag = false;
  bool uidIsOk = false;
  uint8_t uid[8];
  
  while (uidIsOk == false) {
    uidIsOk == true;
    for (int i = 0;  i < sizeof(uid); i++)
    {
      if ( rightUid[i] != uid[i] )
      {
        break;
      }
    }
    if (errorFlag) {
      uint32_t irqStatus = nfc.getIRQStatus();
      nfc.reset();
      nfc.setupRF();
      errorFlag = false;
    }
    ISO15693ErrorCode rc = nfc.getInventory(uid);
    if (ISO15693_EC_OK != rc) {
      errorFlag = true;
      return;
    }
  }
#endif
>>>>>>> Stashed changes
}

void loop() {
  long pressed = 0;
  if (capacitiveSen == true) { //Pretty explicit
    capacitive_routine();
  }
  if (button == true) {
    button_routine();
  }
  if (powerCutdown == true) {
    power_routine();
  }
  if (counter == 500 || change == true) { // If 500ms passed or a changed made
    Serial.write(code, sizeof(code)); //Send the trame to the scooter
    counter = 0; //Reset the time counter
    change = false; // Reset of the change value
  }
  delay(1);
  counter++; //Increase the counter
}


void calculateforth() {
  forth = 0; //Calculation of the value of the 4th byte
  if (Turbo == 1) {
    forth = forth + 128;
  }
  if (seconddigit == 1) {
    forth = forth + 64;
  }
  if (fastAcceleration == 1) {
    forth = forth + 32;
  }
  if (KPH == 1) {
    forth = forth + 16;
  }
  if (fifthdigit == 1) {
    forth = forth + 8;
  }
  if (Light == 1) {
    forth = forth + 4;
  }
  if (LightBlink == 1) {
    forth = forth + 2;
  }
  if (ESCOn == 1) {
    forth++;
  }
  code[3] = forth; //Adding the values to the trame
  code[4] = SpeedLimit;
  code[5] = CRC8.maxim(code, 5);
  change = true; //Allowing to bypass the 500ms counter so that the change is done as quick as possible
}

void button_routine()
{
  if (digitalRead(buttonPin) == LOW) { //Still debouncing
    debounceCapacitive++;
    if (debounceButton == 600) {
      longPress();
    }
  } else {
    if (debounceButton > debounceTime) {
      shortPress();
    }
    debounceButton = 0;
  }
}

void power_routine()
{
  if (analogRead(powerLoss) >= 1023) { //If the power is going down on the arduino, the cap (4.5V) that usaly give 980 at reeding will overflow the input, causing it to read the max value
    Serial.write(bye, sizeof(bye)); //Sending the locking sequence
    digitalWrite(ledPin, HIGH); //Light up the led, just for fun, and to finish drawing the caps down
    while (true) {} //Go to sleep good boy
  }
}

void capacitive_routine()
{
  long pressed =  capSens.capacitiveSensor(30); //Read the value of the capacitive sensor
  if (pressed > sensivity) { //Still debouncing
    debounceCapacitive++;
    if (debounceCapacitive == 600) {
      longPress();
    }
  } else {
    if (debounceCapacitive > debounceTime) {
      shortPress();
    }
    debounceCapacitive = 0;
  }
}

void ISR_button() //Put the flag to one if button changed state
{
  buttonFlag = 1;
}

void longPress() {
  Turbo = !Turbo; //Toogle turbo
  fastAcceleration = !fastAcceleration;
  if (SpeedLimit == speeddef) { //if speed is alredy what wanted, put it to 20
    SpeedLimit = 20;
    calculateforth();
    tone(buzzer, 440, 40);
    delay(60);
    tone(buzzer, 440, 40);
  } else { //Otherwise, put it to defined
    SpeedLimit = speeddef;
    calculateforth();
    tone(buzzer, 880, 40);
    delay(60);
    tone(buzzer, 880, 40);
  }
}

void shortPress() {
  Light = !Light; //Same as button, changing state and beeping
  calculateforth();
  if (Light == 1) {
    tone(buzzer, 880, 40);
  } else {
    tone(buzzer, 440, 40);
  }
}

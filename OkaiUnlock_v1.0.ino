//Importing librairies
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <FastCRC.h>
#include <CapacitiveSensor.h>

FastCRC8 CRC8;

//Variables can be change to activate or desactivate some options

bool powerCutdown = false;
bool capacitiveSen = false;
bool button = true;
bool NFCEn = false;

byte uidok[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //put the uid of your nfc card here if you plan to use one (4bytes for a mifare classic, 7 for an mifare ultralight)

bool Turbo = 1; //1 for on, 0 for off
bool seconddigit = 1; //Still unknown function
bool fastAcceleration = 1; //1 for on, 0 for off
bool KPH = 1; //1 for KPH, 0 for MPH
bool fifthdigit = 0; //Still unknown function
bool Light = 0; //1 for on, 0 for off
bool LightBlink = 0; //1 for on, 0 for off
bool ESCOn = 1; //1 for on, 0 for off
int SpeedLimit = 255; //Beetwen 0 and 255
//Do not change variables after that if you don't know what you're doing


//Pin definitions for externals things
int buzzer = A0;
int ledPin = 13;
int buttonPin = 3;
int powerLoss = A4;
#define PN532_SCK  (5)
#define PN532_MOSI (6)
#define PN532_SS   (7)
#define PN532_MISO (8)
CapacitiveSensor capSens = CapacitiveSensor(10, 11);


//Program variables
int lastButtonState = 1; //Used to record the last button state
long unsigned int lastPress;  //Used to debounce the button
volatile int buttonFlag; //Use in the interup for the button
int debounceTime = 20;  //Time of debouncing, valid for button and capacitive sensor
int debounceCapacitive = 0; //Used to debounce the capacitive sensor
int counter = 0;  //Used to count 500 loops (of 1ms) before sending trame to the scooter
bool change; //Used to bypass the counter if any changes in the 4th bytes have been done, to be send to the scooter as quick as possible
bool nfcok = false; //used to see if the correct NFC card has already been scaned if nfc is used

int forth;  //Decimal value of the 4th byte
byte code[6] = {0xA6, 0x12, 0x02};  //Trame send to unlock the scooter, the 3 other bytes are added later on the code
byte bye[] = {0xA6, 0x12, 0x02, 0x00, 0x00, 0xDF};  //Trame send to lock the scooter if powerdown sensor is used, avoid 20s of latency between turn off and actual shutdown

void setup() {
  Serial.begin(9600); //Initialize the serial communication with the scooter
  calculateforth(); //Calculate the 4th byte depending on the options choosed
  pinMode(ledPin, OUTPUT);
  if (button == true) {
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), ISR_button, CHANGE); //Interupt is used to see if the button is pressed (if button used)
  }

  Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS); //Declaration of the NFC module (even if not used
  nfc.begin();
  if (NFCEn == true) {
    uint32_t versiondata = nfc.getFirmwareVersion(); //Try to get firmware version of the NFC reader
    if (! versiondata) { //If no nfc reader seen and NFC was wanted make an error code with the led and go to while(1)
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
      digitalWrite(ledPin, HIGH);
      while (1);
    }
  }
  if (NFCEn == true) {
    nfc.setPassiveActivationRetries(0xFF); //Still NFC shitty init
  }
  if (NFCEn == true) {
    nfc.SAMConfig();
  }
  tone(buzzer, 440, 40); //Make a bip with the buzzer at startup (if beeper wired)
  if (NFCEn == true) { //if nfc enable
    while (nfcok == false) { //read NFC tags until the right tag has been found
      boolean success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
      if (success) {
        bool same = true;
        for (int i = 0; i < sizeof(uid); i++) { //Compare the array of the scan tag to the one known
          if (uid[i] != uidok[i]) {
            same = false; //if one value of the array is not corresponding, put this to false
          }
        }
        if (same == true) { //If one value of the array was false, this will be false too, otherwise, it will allow to get out of the while loop
          nfcok = true;
        }
        delay(1000); //Wait for the next tag scan
      }
      else
      {
        // PN532 probably timed out waiting for a card
      }
    }
  }
}

void loop() {
  long start = millis();
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
  if ((NFCEn == false || (NFCEn == true && nfcok == true)) && (counter == 500 || change == true)) { // If No nfc and 500ms passed or a changed made; or if nfc, correct tag was previously scan and 500ms or a change. (For future use of a locking functions, otherwise no need to check is tag was ok as if it was not, was still locked in the while condition)
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
  if (((millis() - lastPress) > debounceTime && buttonFlag)) //Debouncing the button
  {
    lastPress = millis();   //update lastPress
    if (digitalRead(buttonPin) == 0 && lastButtonState == 1)   //if button is pressed and was released last change
    {
      Light = !Light; //Change the state of the Light variable
      calculateforth(); //calculate the 4th byte with the new value and beeping acordingly to the state
      if (Light == 1) {
        tone(buzzer, 880, 40);
      } else {
        tone(buzzer, 440, 40);
      }
      lastButtonState = 0;    //record the lastButtonState
    }

    else if (digitalRead(buttonPin) == 1 && lastButtonState == 0)   //if button is not pressed, and was pressed last change
    {
      lastButtonState = 1;    //record the lastButtonState
    }
    buttonFlag = 0;
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
  if (debounceCapacitive == debounceTime) { //Debouncing
    Light = !Light; //Same as button, changing state and beeping
    calculateforth();
    if (Light == 1) {
      tone(buzzer, 880, 40);
    } else {
      tone(buzzer, 440, 40);
    }
  }
  if (pressed > 999) { //Still debouncing
    debounceCapacitive++;
  }
  else
  {
    debounceCapacitive = 0;
  }
}

void ISR_button() //Put the flag to one if button changed state
{
  buttonFlag = 1;
}

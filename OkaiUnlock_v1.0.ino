//Importing librairies
#include <Arduino.h>
#include <FastCRC.h>

FastCRC8 CRC8;

//Variables can be change to activate or desactivate some options

#define VOI1 //can be define as VOI1, VOI2 or BIRDZERO


#define POWERSENS //put double slash in front off this line to turn off, if not, will be on
#if defined(VOI2) || defined(BIRDZERO)
#define BUTTON  //put double slash in front off this line to turn off, if not, will be on
//#define CAPACITIV   //put double slash in front off this line to turn off, if not, will be on
#endif

#if defined(VOI2) || defined(BIRDZERO)
bool Turbo = 1; //1 for on, 0 for off
bool seconddigit = 1; //Still unknown function
bool fastAcceleration = 1; //1 for on, 0 for off
bool KPH = 1; //1 for KPH, 0 for MPH
bool fifthdigit = 0; //Still unknown function
bool Light = 0; //1 for on, 0 for off
bool LightBlink = 0; //1 for on, 0 for off
bool ESCOn = 1; //1 for on, 0 for off
int SpeedLimit = 255; //Beetwen 0 and 255
#endif

const int sensivity = 200; //To ajust the sensivity of the capacitive Sensor
//Do not change variables after that if you don't know what you're doing


//Pin definitions for externals things
#define BUZZER A0
#define LEDPIN 13
#define BUTTONPIN 2
#define POWERLOSS A4

#ifdef CAPACITIV
#include <CapacitiveSensor.h>
CapacitiveSensor capSens = CapacitiveSensor(4, 6);
#endif

//Checks if options are OK
#if !defined(VOI2) && !defined(VOI1) && !defined(BIRDZERO)
#error "No scooter type defined or incorrect type defined"
#endif
#if defined(CAPACITIV) && defined(BUTTON)
#warning "Two similar types of inputs at the same time ?"
#endif

//Program variables
int lastButtonState = 1; //Used to record the last button state
volatile int buttonFlag; //Use in the interup for the button
int debounceTime = 20;  //Time of debouncing, valid for button and capacitive sensor
int debounceCapacitive = 0; //Used to debounce the capacitive sensor
int debounceButton = 0;
int counter = 0;  //Used to count 500 loops (of 1ms) before sending trame to the scooter
bool change; //Used to bypass the counter if any changes in the 4th bytes have been done, to be send to the scooter as quick as possible

#if defined(VOI2) || defined(BIRDZERO)
int speeddef = SpeedLimit;

int forth;  //Decimal value of the 4th byte
byte code[6] = {0xA6, 0x12, 0x02};  //Trame send to unlock the scooter, the 3 other bytes are added later on the code
byte bye[] = {0xA6, 0x12, 0x02, 0x00, 0x00, 0xDF};  //Trame send to lock the scooter if powerdown sensor is used, avoid 20s of latency between turn off and actual shutdown
#endif
#ifdef VOI1
byte code[9] = {0x60, 0x03, 0x01, 0xE1, 0x00, 0x01, 0xF1, 0xDD, 0xB0};
byte bye[9] = {0x60, 0x03, 0x01, 0xE1, 0x00, 0x01, 0xF0, 0x1D, 0x71};
#endif

void setup() {
  Serial.begin(9600); //Initialize the serial communication with the scooter
  #if defined(VOI2) || defined(BIRDZERO)
  calculateforth(); //Calculate the 4th byte depending on the options choosed
  #endif
  pinMode(LEDPIN, OUTPUT);
#ifdef BUTTON
  pinMode(BUTTONPIN, INPUT_PULLUP);
#endif
  tone(BUZZER, 440, 40); //Make a bip with the buzzer at startup (if beeper wired)
}

void loop() {
  long pressed = 0;
#if defined(CAPACITIV) && (defined(VOI2) || defined(BIRDZERO))
  capacitive_routine();
#endif
#if defined(BUTTON) && (defined(VOI2) || defined(BIRDZERO))
  button_routine();
#endif
#ifdef POWERSENS
  power_routine();
#endif
  if (counter == 500 || change == true) { // If 500ms passed or a changed made
    Serial.write(code, sizeof(code)); //Send the trame to the scooter
    counter = 0; //Reset the time counter
    change = false; // Reset of the change value
  }
  delay(1);
  counter++; //Increase the counter
}


#if defined(VOI2) || defined(BIRDZERO)
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
#endif

#ifdef BUTTON
void button_routine()
{
  if (digitalRead(BUTTONPIN) == LOW) { //Still debouncing
    debounceButton++;
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
#endif

#ifdef POWERSENS
void power_routine()
{
  if (analogRead(POWERLOSS) >= 1023) { //If the power is going down on the arduino, the cap (4.5V) that usaly give 980 at reeding will overflow the input, causing it to read the max value
    Serial.write(bye, sizeof(bye)); //Sending the locking sequence
    digitalWrite(LEDPIN, HIGH); //Light up the led, just for fun, and to finish drawing the caps down
    while (true) {} //Go to sleep good boy
  }
}
#endif

#ifdef CAPACITIV
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
#endif


#if (defined(BUTTON) || defined(CAPACITIV)) && (defined(VOI2) || defined(BIRDZERO))
void longPress() {
  Turbo = !Turbo; //Toogle turbo
  fastAcceleration = !fastAcceleration;
  if (SpeedLimit == speeddef) { //if speed is alredy what wanted, put it to 20
    SpeedLimit = 20;
    calculateforth();
    tone(BUZZER, 440, 40);
    delay(60);
    tone(BUZZER, 440, 40);
  } else { //Otherwise, put it to defined
    SpeedLimit = speeddef;
    calculateforth();
    tone(BUZZER, 880, 40);
    delay(60);
    tone(BUZZER, 880, 40);
  }
}

void shortPress() {
  Light = !Light; //Same as button, changing state and beeping
  calculateforth();
  if (Light == 1) {
    tone(BUZZER, 880, 40);
  } else {
    tone(BUZZER, 440, 40);
  }
}
#endif

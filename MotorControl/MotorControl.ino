/*
  ----------------------------
   Arduino Power Desktop PLUS
  ----------------------------
  Summary: Controls the direction and speed of the motor, supports programmable height
  
  Description: The code waits for a button to be pressed (UP or DOWN), 
  and accordingly powers the motor in the appropriate direction and with the appropriate speed.
  On my particular setup (a heavy desktop with 3 monitors), when going UP I need 100% of the power, speed and torque,
  however, when going down I need a bit less since gravity helps, to keep the speed up and down at a similar rate.
  You may want to tweak the variables PWM_SPEED_UP and PWM_SPEED_DOWN to adjust it to your desktop load, the allowed values
  are 0 to 255. 255 being Maximum Speed and 0 being OFF.

  You may use this code and all of the diagrams and documentations completely free. Enjoy!
  
  Author: Cesar Moya
  Date:   July 8th, 2020
  URL:    https://github.com/cesar-moya/arduino-power-desktop
*/
#include <EEPROM.h>

#define BUTTON_DOWN 2
#define BUTTON_UP 3
#define enA 6
#define in1 7
#define in2 8
#define LED_SYS 9
#define enB 10
#define in3 11
#define in4 12

//Struct to store the various necessary variables to persist the autoRaise/autoLower programs to EEPROM
struct StoredProgram
{
  bool isUpSet = false; //whether the UP program has been recorded
  bool isDownSet = false; // whether the DOWN program has been recorded
  float timeUp = 0; //time in milliseconds recorded to RAISE the desk
  float timeDown = 0; // time in milliseconds recorded to LOWER the desk
};

StoredProgram savedProgram;
int EEPROM_ADDRESS = 0;
bool BUTTON_UP_STATE = LOW;
bool BUTTON_DOWN_STATE = LOW;
long BUTTON_WAIT_TIME = 500; //the small delay before starting to go up/down for smoothness on any button

int PRG_PRECLICKS = 3;
long PRG_PRECLICK_THRESHOLD = 1000; //the threshold for the PRG_PRECLICKS before PRG_HOLD_THRESHOLD kicks in
long PRG_HOLD_THRESHOLD = 2000;
long ENTER_PRGMODE_THRESHOLD = 2000; //the time to enter program mode

int btnUpClicks = 0;
long btnUpFirstClickTime = 0;
bool prgUpActivated = false;
long prgModeHoldTime = 0;

//Using custom values to ensure no more than 24v are delivered to the motors given my desk load.
//feel free to play with these numbers but make sure to stay within your motor's rated voltage.
const int PWM_SPEED_UP = 245; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 220; //0 - 255, controls motor speed when going DOWN

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(LED_SYS, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  readFromEEPROM();
}

void loop() {
  //If both buttons are pressed and held, check for entering program mode, if this func returns true, skip anything else in 
  //the main loop
  if (isHandlingProgramMode()){  
    return;
  }

  //If the threshold for the first 2 clicks expired, reset the clicks back to zero.
  if (btnUpFirstClickTime > 0 && (millis() - btnUpFirstClickTime) >= PRG_PRECLICK_THRESHOLD)
  {
    Serial.println("BUTTON UP | PRECLICK_THRESHOLD expired, clicks = 0");
    btnUpClicks = 0;
    btnUpFirstClickTime = 0;
  }

  //If button has just been pressed, we count the presses, and we also handle if it's kept hold to raise desk / enter program
  if (!BUTTON_UP_STATE && debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.print("BUTTON UP | Pressed ["); Serial.print(btnUpClicks); Serial.println("] times");
    BUTTON_UP_STATE = HIGH;
    if (btnUpClicks == 0)
    {
      btnUpFirstClickTime = millis();
    }
    btnUpClicks++;

    long pressTime = millis();
    long elapsed = 0;
    while(digitalRead(BUTTON_UP) && !prgUpActivated){
      elapsed = millis() - pressTime;
      Serial.print("BUTTON UP | Holding | elapsed: "); Serial.print(elapsed);
      Serial.print(" | Clicks: "); Serial.println(btnUpClicks);
      if (elapsed >= BUTTON_WAIT_TIME){ //small delay before starting to work for smoothness
        if (btnUpClicks == PRG_PRECLICKS && elapsed >= PRG_HOLD_THRESHOLD)  //if 2 clicks + hold for 2 secs, enter auto-raise
        {
          prgUpActivated = true;
          break;
        }else{
          goUp();
        }  
      }

      //If you press down while holding UP, you indicate desire to enter program mode, break the loop to stop going 
      //UP, the code will catch the 2 buttons being held separately
      if(debounceRead(BUTTON_DOWN, LOW)){
        Serial.println("BUTTON UP | Button DOWN pressed, breaking loop");
        break;
      }
      
    }

    if(prgUpActivated){
      Serial.print("BUTTON UP | Going up automatically | Elapsed to activate:");
      Serial.println(elapsed);
      autoRaiseDesk(elapsed);
      prgUpActivated = false;
      Serial.println("BUTTON UP | PrgUp Deactivated");
    }

    stopMoving();
  }
  else if (BUTTON_UP_STATE && !debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.println("BUTTON UP | Released");
    BUTTON_UP_STATE = LOW;
  }
  
}

bool isHandlingProgramMode(){
  if (debounceRead(BUTTON_DOWN, LOW) && debounceRead(BUTTON_UP, LOW))
  {
    Serial.println("BOTH BUTTONS PRESSED");
    if (prgModeHoldTime == 0)
      prgModeHoldTime = millis();

    //Enter program mode
    if ((millis() - prgModeHoldTime) >= ENTER_PRGMODE_THRESHOLD)
    {
      Serial.println("PROGRAM MODE | Entered Program Mode");
      long enterTime = millis();
      long lastBlink = 0;
      long recStart = 0;
      bool recUp = false;
      bool recDown = false;
      bool btnUpPressed = digitalRead(BUTTON_UP);

      while (true)
      {
        long elapsed = (millis() - enterTime);
        if (!recUp && !recDown)
          programModeBlink(elapsed, lastBlink);

        //when entering this loop. button up WAS already pressed, we need to wait until it's released, then start recording
        //if button wasn't pressed and now IS pressed, start recording
        if (!btnUpPressed && debounceRead(BUTTON_UP, btnUpPressed))
        {
          btnUpPressed = true;
          recUp = true;
          recStart = millis();
        } //if button was pressed and now is released
        else if (btnUpPressed && !debounceRead(BUTTON_UP, btnUpPressed))
        {
          btnUpPressed = false;
          if (recUp)
          {
            recUp = false;
            long timeUp = millis() - recStart;
            saveToEEPROM_TimeUp(timeUp);
            Serial.print("Recorded Program UP: ");
            Serial.println(timeUp);
            break;
          }
        }

        if (recUp)
        {
          Serial.print("PROGRAM MODE | Recording | elapsed: ");
          Serial.println(millis() - recStart);
          goUp();
        }

      } //end of while / program mode

      Serial.println("PROGRAM MODE | Exited");
      digitalWrite(LED_SYS, LOW);
    }

    return true;
  }
  else
  {
    prgModeHoldTime = 0;
    return false;
  }
}

void programModeBlink(long elapsed, long lastBlink){
  long nextBlink = elapsed / 100;
  if (nextBlink > lastBlink)
  {
    lastBlink = nextBlink;
    if (lastBlink % 2 == 0)
    {
      digitalWrite(LED_SYS, HIGH);
    }
    else
    {
      digitalWrite(LED_SYS, LOW);
    }
  }
}

//Need to debounce the initial button reads to prevent flickering
bool debounceRead(int buttonPin, bool state)
{
  bool stateNow = digitalRead(buttonPin);
  if (state != stateNow)
  {
    delay(10);
    stateNow = digitalRead(buttonPin);
  }
  return stateNow;
}

/****************************************
  LOWER / RAISE DESK FUNCTIONS
****************************************/
// Tries to raise the desk automatically using the previously programmed values
// Only attempts to raise it if the desk is currently LOWERED and if a program exists
void autoRaiseDesk(long alreadyElapsed)
{
  Serial.print("IsUpSet?");
  Serial.println(savedProgram.isUpSet);
  if (savedProgram.isUpSet)
  {
    //we subtract the amount of ms that already elapsed to enter auto-mode (about 2 seconds in default configs)
    long timeToRaise = savedProgram.timeUp - alreadyElapsed; 
    long startTime = millis();
    while((millis() - startTime) < timeToRaise){
      goUp();
      //if any button is pressed during program play, cancel and invalidate
      if (debounceRead(BUTTON_UP, LOW) || debounceRead(BUTTON_DOWN, LOW))
      {
        break;
      }
    }
    stopMoving();
  }
  else
  {
    Serial.println("Error: Up Program not set");
    warningBlink();
  }
  digitalWrite(LED_SYS, LOW);
}

// Tries to lower the desk automatically using the previously programmed values
// Only attempts to raise it if the desk is currently RAISED and if a program exists
void autoLowerDesk()
{
  Serial.print("IsDownSet?");
  Serial.println(savedProgram.isDownSet);
  if (savedProgram.isDownSet)
  {
    //Get a backup copy of the program (in case power goes out mid-raise)
    StoredProgram prog = getEEPROMCopy();
    bool isCancelled = false;
    long startTime = millis();
    while((millis() - startTime) < prog.timeDown){
      goDown();
      //if any button is pressed during program play, cancel and invalidate
      if (debounceRead(BUTTON_UP, LOW) || debounceRead(BUTTON_DOWN, LOW))
      {
        isCancelled = true;
        stopMoving();
        break;
      }
    }
    stopMoving();
    if(!isCancelled){
      //saveToEEPROM_DeskLowered(prog);  
    }
  }
  else
  {
    Serial.println("Warning: Can't lower desk on current conditions");
    warningBlink();
  }
  digitalWrite(LED_SYS, LOW);
}

//Send PWM signal to L298N enX pin (sets motor speed)
void goUp()
{
  Serial.print("UP:"); Serial.println(PWM_SPEED_UP);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_SYS, HIGH);

  //Motor A: Turns in (LH) direction
  analogWrite(enA, PWM_SPEED_UP);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  //Motor B: Turns in OPPOSITE (HL) direction
  analogWrite(enB, PWM_SPEED_UP);
  digitalWrite(in4, HIGH);
  digitalWrite(in3, LOW);
}

//Send PWM signal to L298N enX pin (sets motor speed)
void goDown()
{
  Serial.print("DOWN:");Serial.println(PWM_SPEED_DOWN);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_SYS, HIGH);

  //Motor A: Turns in (HL) Direction
  analogWrite(enA, PWM_SPEED_DOWN);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  //Motor B: Turns in OPPOSITE (LH) direction
  analogWrite(enB, PWM_SPEED_DOWN);
  digitalWrite(in4, LOW);
  digitalWrite(in3, HIGH);
}

void stopMoving()
{
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_SYS, LOW);
  Serial.println("Idle...");
}

/****************************************
  EEPROM FUNCTIONS
****************************************/
//Saves the Time it took to raise the desk (timeUP) and sets the desk as Raised
void saveToEEPROM_TimeUp(long timeUp)
{
  thinkingBlink();
  Serial.println("Saving UP to EEPROM");
  savedProgram.timeUp = (float)timeUp;
  savedProgram.isUpSet = true;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
  successBlink();
}

//Saves the Time it took to raise the desk (timeUP) and sets the desk as Lowered (!Raised)
void saveToEEPROM_TimeDown(long timeDown)
{
  thinkingBlink();
  Serial.println("Saving DOWN to EEPROM");
  savedProgram.timeDown = (float)timeDown;
  savedProgram.isDownSet = true;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
  successBlink();
}

StoredProgram getEEPROMCopy(){ //TODO: Delete
  StoredProgram prog;
  prog.timeUp = savedProgram.timeUp;
  prog.isUpSet = savedProgram.isUpSet;
  prog.timeDown = savedProgram.timeDown;
  prog.isDownSet = savedProgram.isDownSet;
  return prog;
}

void readFromEEPROM()
{
  thinkingBlink(); //TODO: Remove
  Serial.println("Reading from EEPROM");
  EEPROM.get(EEPROM_ADDRESS, savedProgram);
  Serial.println("TimeUp:");
  long timeUpInt = (long)savedProgram.timeUp;
  Serial.println(timeUpInt);
  Serial.println("TimeDown:");
  long timeDownInt = (long)savedProgram.timeDown;
  Serial.println(timeDownInt);
  Serial.println("IsUPSet?");
  Serial.println(savedProgram.isUpSet);
  Serial.println("IsDownSet?");
  Serial.println(savedProgram.isDownSet);
  successBlink();
}

/****************************************
  BLINK FUNCTIONS
****************************************/
void successBlink()
{
  digitalWrite(LED_SYS, LOW);
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(LED_SYS, HIGH);
    delay(500);
    digitalWrite(LED_SYS, LOW);
    delay(500);
  }
}

void warningBlink()
{
  digitalWrite(LED_SYS, LOW);
  for (int i = 1; i <= 9; i++)
  {
    digitalWrite(LED_SYS, HIGH);
    delay(50);
    digitalWrite(LED_SYS, LOW);
    delay(50);
    if (i > 0 && i % 3 == 0)
    {
      delay(200);
    }
  }
}

void thinkingBlink()
{
  digitalWrite(LED_SYS, LOW);
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(LED_SYS, HIGH);
    delay(30);
    digitalWrite(LED_SYS, LOW);
    delay(30);
  }
}

void builtInBlink(int secs)
{
  digitalWrite(LED_BUILTIN, LOW);
  for (int i = 0; i < secs; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

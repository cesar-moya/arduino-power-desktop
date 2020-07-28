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

//Program Mode SET/ACTIVATE variables
int  PRG_ACTIVATE_PRECLICKS = 3;//number of clicks before checking for hold to activate auto-raise/lower
long PRG_ACTIVATE_PRECLICK_THRESHOLD = 1000; //the threshold for the PRG_ACTIVATE_PRECLICKS before PRG_ACTIVATE_HOLD_THRESHOLD kicks in
long PRG_ACTIVATE_HOLD_THRESHOLD = 2000;  //threshold that needs to elapse holding a button (up OR down) after preclicks has been satisfied to enter auto-raise / auto-lower
long PRG_SET_THRESHOLD = 2000; //the time to enter program mode
long holdButtonsStartTime = 0; //the time when the user began pressing and holding both buttons, attempting to enter program mode

//Button UP variables
int btnUpClicks = 0;
long btnUpFirstClickTime = 0;
bool autoRaiseActivated = false;

//Button DOWN variables
int btnDownClicks = 0;
long btnDownFirstClickTime = 0;
bool autoLowerActivated = false;

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
  //If both buttons are pressed and held, check for entering program mode, 
  //if this func returns true we want to skip anything else in the main loop
  if (handleProgramMode()){  
    return;
  }
  //Check if the user didn't click at least twice in less than a second, reset counters if necessary
  checkButtonClicksExpired();

  //Handle press and hold of buttons to raise/lower, and check if enter auto-raise and auto-lower
  handleButtonUp();
  handleButtonDown();
}

/****************************************
  MAIN CONTROL FUNCTIONS
****************************************/

//If you didn't enter auto-raise or auto-lower by doing the magic combination (2 clicks in under a second + click and hold 2 secs)
//then this function resets the clicks back to zero and the timer. handles both UP and DOWN buttons
void checkButtonClicksExpired(){
  //If the threshold for the first 2 clicks expired, reset the clicks back to zero.
  if (btnUpFirstClickTime > 0 && (millis() - btnUpFirstClickTime) >= PRG_ACTIVATE_PRECLICK_THRESHOLD)
  {
    Serial.println("BUTTON UP | PRECLICK_THRESHOLD expired, clicks = 0");
    btnUpClicks = 0;
    btnUpFirstClickTime = 0;
  }

  //If the threshold for the first 2 clicks expired, reset the clicks back to zero.
  if (btnDownFirstClickTime > 0 && (millis() - btnDownFirstClickTime) >= PRG_ACTIVATE_PRECLICK_THRESHOLD)
  {
    Serial.println("BUTTON DOWN | PRECLICK_THRESHOLD expired, clicks = 0");
    btnDownClicks = 0;
    btnDownFirstClickTime = 0;
  }
}

void handleButtonUp(){
  //If button has just been pressed, we count the presses, and we also handle if it's kept hold to raise desk / enter program
  if (!BUTTON_UP_STATE && debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.print("BUTTON UP | Pressed [");
    Serial.print(btnUpClicks);
    Serial.println("] times");
    BUTTON_UP_STATE = HIGH;
    if (btnUpClicks == 0)
    {
      btnUpFirstClickTime = millis();
    }
    btnUpClicks++;

    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_UP) && !autoRaiseActivated)
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON UP | Holding | elapsed: ");
      Serial.print(elapsed);
      Serial.print(" | Clicks: ");
      Serial.println(btnUpClicks);
      
      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME)
      {
        //if 2 clicks + hold for 2 secs, enter auto-raise
        if (btnUpClicks >= PRG_ACTIVATE_PRECLICKS && elapsed >= PRG_ACTIVATE_HOLD_THRESHOLD) 
        {
          autoRaiseActivated = true;
          break;
        }
        else
        {
          goUp();
        }
      }

      //If you press down while holding UP, you indicate desire to enter program mode, break the loop to stop going
      //UP, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_DOWN, LOW))
      {
        Serial.println("BUTTON UP | Button DOWN pressed, breaking loop");
        break;
      }
    }

    if (autoRaiseActivated)
    {
      Serial.print("BUTTON UP | Going up automatically | Elapsed to activate:");
      Serial.println(elapsed);
      autoRaiseDesk(elapsed);
      autoRaiseActivated = false;
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

void handleButtonDown()
{
  //If button has just been pressed, we count the presses, and we also handle if it's kept hold to lower desk / enter program
  if (!BUTTON_DOWN_STATE && debounceRead(BUTTON_DOWN, BUTTON_DOWN_STATE))
  {
    Serial.print("BUTTON DOWN | Pressed [");
    Serial.print(btnDownClicks);
    Serial.println("] times");
    BUTTON_DOWN_STATE = HIGH;
    if (btnDownClicks == 0)
    {
      btnDownFirstClickTime = millis();
    }
    btnDownClicks++;

    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_DOWN) && !autoLowerActivated)
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON DOWN | Holding | elapsed: ");
      Serial.print(elapsed);
      Serial.print(" | Clicks: ");
      Serial.println(btnDownClicks);
      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME) 
      {
        //if 2 clicks + hold for 2 secs, enter auto-lower
        if (btnDownClicks >= PRG_ACTIVATE_PRECLICKS && elapsed >= PRG_ACTIVATE_HOLD_THRESHOLD) 
        {
          autoLowerActivated = true;
          break;
        }
        else
        {
          goDown();
        }
      }

      //If you press UP while holding DOWN, you indicate desire to enter program mode, break the loop to stop going
      //DOWN, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_UP, LOW))
      {
        Serial.println("BUTTON DOWN | Button UP pressed, breaking loop");
        break;
      }
    }

    if (autoLowerActivated)
    {
      Serial.print("BUTTON DOWN | Going DOWN automatically | Elapsed to activate:");
      Serial.println(elapsed);
      autoLowerDesk(elapsed);
      autoLowerActivated = false;
      Serial.println("BUTTON DOWN | PrgDown Deactivated");
    }

    stopMoving();
  }
  else if (BUTTON_DOWN_STATE && !debounceRead(BUTTON_DOWN, BUTTON_DOWN_STATE))
  {
    Serial.println("BUTTON DOWN | Released");
    BUTTON_DOWN_STATE = LOW;
  }
}

//Checks if the user is requesting to enter program mode (by pressing and holding both buttons), if so, returns true, otherwise, false
bool handleProgramMode(){
  if (debounceRead(BUTTON_DOWN, LOW) && debounceRead(BUTTON_UP, LOW))
  {
    Serial.println("BOTH BUTTONS PRESSED");
    if (holdButtonsStartTime == 0)
      holdButtonsStartTime = millis();

    //Enter program mode
    if ((millis() - holdButtonsStartTime) >= PRG_SET_THRESHOLD)
    {
      Serial.println("PROGRAM MODE | Entered Program Mode");
      long enterTime = millis();
      long recStart = 0;
      bool recUp = false;
      bool recDown = false;
      bool btnUpPressed = digitalRead(BUTTON_UP);
      bool btnDownPressed = digitalRead(BUTTON_DOWN);

      while (true)
      {
        long elapsed = (millis() - enterTime);
        if (!recUp && !recDown)
          programBlink(elapsed, 100);

        //HANDLE PROGRAM - RAISE
        //when entering this loop. button UP WAS already pressed, we need to wait until it's released, then start recording
        //if button wasn't pressed and now IS pressed, start recording
        if(!recDown){
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
        }
        
        //HANDLE PROGRAM - LOWER
        //when entering this loop. button DOWN WAS already pressed, we need to wait until it's released, then start recording
        //if button wasn't pressed and now IS pressed, start recording
        if(!recUp){
          if (!btnDownPressed && debounceRead(BUTTON_DOWN, btnDownPressed))
          {
            btnDownPressed = true;
            recDown = true;
            recStart = millis();
          } //if button was pressed and now is released
          else if (btnDownPressed && !debounceRead(BUTTON_DOWN, btnDownPressed))
          {
            btnDownPressed = false;
            if (recDown)
            {
              recDown = false;
              long timeDown = millis() - recStart;
              saveToEEPROM_TimeDown(timeDown);
              Serial.print("Recorded Program DOWN: ");
              Serial.println(timeDown);
              break;
            }
          }

          if (recDown)
          {
            Serial.print("PROGRAM MODE | Recording Down| elapsed: ");
            Serial.println(millis() - recStart);
            goDown();
          }
        }

      } //end of while / program mode

      Serial.println("PROGRAM MODE | Exited");
      digitalWrite(LED_SYS, LOW);
    }

    return true;
  }
  else
  {
    holdButtonsStartTime = 0;
    return false;
  }
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
    Serial.print("AUTORAISE | PRG_TimeUp: "); Serial.print(savedProgram.timeUp);
    Serial.print(" | TimeToRaise: "); Serial.println(timeToRaise);
    long startTime = millis();
    long elapsed = (millis() - startTime);
    bool btnUpState = digitalRead(BUTTON_UP);
    bool btnDownState = digitalRead(BUTTON_DOWN);

    while(elapsed < timeToRaise){
      programBlink(elapsed, 50);
      goUp();

      //if any button is pressed during program play, cancel and invalidate
      if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){
        Serial.println("Program Cancelled by user, BUTTON UP");
        break;
      }else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
        btnUpState = LOW;
      }

      if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
        Serial.println("Program Cancelled by user, BUTTON DOWN");
        break;
      }
      else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
        btnDownState = LOW;
      }

      elapsed = (millis() - startTime);
    }
    stopMoving();
    long stopTime = millis();
    long totalElapsed = stopTime - startTime;
    Serial.print("AUTORAISE | Auto-TotalElapsed:"); Serial.println(totalElapsed);
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
void autoLowerDesk(long alreadyElapsed)
{
  Serial.print("IsDownSet?");
  Serial.println(savedProgram.isDownSet);
  if (savedProgram.isDownSet)
  {
    long timeToLower = savedProgram.timeDown - alreadyElapsed;
    Serial.print("AUTOLOWER | PRG_TimeDown: "); Serial.print(savedProgram.timeDown);
    Serial.print(" | TimeToLower: "); Serial.println(timeToLower);
    long startTime = millis();
    long elapsed = (millis() - startTime);
    bool btnUpState = digitalRead(BUTTON_UP);
    bool btnDownState = digitalRead(BUTTON_DOWN);
    
    while (elapsed < timeToLower)
    {
      programBlink(elapsed, 50);
      goDown();

      //if any button is pressed during program play, cancel and invalidate
      if (!btnUpState && debounceRead(BUTTON_UP, btnUpState)){
        Serial.println("Program Cancelled by user, BUTTON UP");
        break;
      }
      else if (btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
        btnUpState = LOW;
      }

      if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
        Serial.println("Program Cancelled by user, BUTTON DOWN");
        break;
      }
      else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
        btnDownState = LOW;
      }

      elapsed = (millis() - startTime);
    }
    stopMoving();
    long stopTime = millis();
    long totalElapsed = stopTime - startTime;
    Serial.print("AUTOLOWER | Auto-TotalElapsed:"); Serial.println(totalElapsed);
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
  long timeUpInt = (long)savedProgram.timeUp;
  Serial.print("TimeUp: ");
  Serial.print(timeUpInt);
  
  long timeDownInt = (long)savedProgram.timeDown;
  Serial.print(" | TimeDown: ");
  Serial.print(timeDownInt);
  Serial.print(" | IsUPSet: ");
  Serial.print(savedProgram.isUpSet);
  Serial.print(" | IsDownSet: ");
  Serial.println(savedProgram.isDownSet);
  successBlink();
}

/****************************************
  BLINK & MISC FUNCTIONS
****************************************/

//pass in the elapsed time in ms and the blink frequency, if you don't know then just pass 100 to blinkFreq
void programBlink(long elapsed, long blinkFreq){
  long blinkNo = elapsed / blinkFreq;
  if (blinkNo % 2 == 0)
    digitalWrite(LED_SYS, HIGH);
  else
    digitalWrite(LED_SYS, LOW);
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

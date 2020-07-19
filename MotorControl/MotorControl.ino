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
#define BUTTON_DOWN_PRG 4
#define BUTTON_UP_PRG 5
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
  bool isDeskRaised = false; //if true, the desk has been raised by using the autoRaise feature. same/opposite for lowered
};

StoredProgram savedProgram;
int EEPROM_ADDRESS = 0;
bool btnUpPRGState = LOW;
bool btnDownPRGState = LOW;

//Using custom values to ensure no more than 24v are delivered to the motors given my desk load.
//feel free to play with these numbers but make sure to stay within your motor's rated voltage.
const int PWM_SPEED_UP = 245; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 220; //0 - 255, controls motor speed when going DOWN

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_UP_PRG, INPUT);
  pinMode(BUTTON_DOWN_PRG, INPUT);
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
  handleButtonProgramUp();
  handleButtonProgramDown();

  //Handle Button Up
  if (debounceRead(BUTTON_UP, LOW) == HIGH)
  {
    //since you affected the position of the desk the recorded program isn't correct anymore
    invalidateEEPROM();
    //We already debounced, so we capture the press
    while (digitalRead(BUTTON_UP))
    {
      goUp();
    }
    //After it finished holding, we stop movement
    stopMoving();
  }

  //Handle Button Down
  if(debounceRead(BUTTON_DOWN, LOW) == HIGH){
    //since you affected the position of the desk the recorded program isn't correct anymore
    invalidateEEPROM();
    //We already debounced, so we capture the press
    while(digitalRead(BUTTON_DOWN)){
      goDown();
    }
    //After it finished holding, we stop movement
    stopMoving();
  }
  //stopMoving();
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
void handleButtonProgramUp()
{
  //If it's pressed, check if it's held
  if (debounceRead(BUTTON_UP_PRG, btnUpPRGState) == HIGH && btnUpPRGState == LOW)
  {
    btnUpPRGState = HIGH;
    Serial.println("BUTTON UP | PROGRAM | Pressed");
    handleButtonProgramUpHold();
  } //If it was a quick press and release, play the saved program
  else if (debounceRead(BUTTON_UP_PRG, btnUpPRGState) == LOW && btnUpPRGState == HIGH)
  {
    Serial.println("BUTTON UP | PROGRAM | Released, Auto-Raising Desk");
    btnUpPRGState = LOW;
    autoRaiseDesk();
  }
}

void handleButtonProgramUpHold()
{
  int count = 0;
  //This is already debounced frm the previous function
  while (digitalRead(BUTTON_UP_PRG))
  {
    digitalWrite(LED_SYS, HIGH);
    Serial.println("BUTTON UP | PROGRAM | Pressed");
    delay(10);
    count++;
    //If button was held for more than 1 second, enter program mode
    if (count >= 70)
    {
      Serial.println("BUTTON UP | PROGRAM | Entered program mode");
      bool saveAndExit = false;
      while (true) //Begin Program Mode
      {
        Serial.println("BUTTON UP | PROGRAM | Waiting for instruction...");
        digitalWrite(LED_SYS, HIGH);
        delay(50);
        digitalWrite(LED_SYS, LOW);
        delay(50);

        //While in program mode, wait for a Button UP to be debouncedly pressed
        if (debounceRead(BUTTON_UP, LOW) == HIGH)
        {
          long startTime = millis();
          saveAndExit = true;
          //While button up is held, record the time elapsed and raise the desk
          while (digitalRead(BUTTON_UP))
          {
            Serial.println("BUTTON UP | PROGRAM | Recording UP...");
            goUp();
          }
          stopMoving();
          
          //If recording happened, save the resulting time to EEPROM and exit
          if (saveAndExit)
          {
            long endTime = millis();
            int elapsed = int(endTime - startTime);
            Serial.print("BUTTON UP | PROGRAM | Elapsed: ");
            Serial.println(elapsed);
            saveToEEPROM_TimeUp(elapsed);
            Serial.println("BUTTON UP | PROGRAM | Saved to EEPROM, Exiting Program Mode");
            break; //End Program Mode
          }
        }
      }
      //So that it doesn't go to play the recently saved program immediately after saving
      btnUpPRGState = LOW;
      count = 0;
    }
  }
}

void handleButtonProgramDown()
{
  //If it's pressed, check if it's held
  if (debounceRead(BUTTON_DOWN_PRG, btnDownPRGState) == HIGH && btnDownPRGState == LOW)
  {
    btnDownPRGState = HIGH;
    Serial.println("BUTTON DOWN | PROGRAM | Pressed");
    handleButtonProgramDownHold();
  } //If it was a quick press and release, play the saved program
  else if (debounceRead(BUTTON_DOWN_PRG, btnDownPRGState) == LOW && btnDownPRGState == HIGH)
  {
    Serial.println("BUTTON DOWN | PROGRAM | Released, Auto-Lowering Desk");
    btnDownPRGState = LOW;
    autoLowerDesk();
  }
}

void handleButtonProgramDownHold(){
  int count = 0;
  //This is already debounced frm the previous function
  while (digitalRead(BUTTON_DOWN_PRG))
  {
    digitalWrite(LED_SYS, HIGH);
    Serial.println("BUTTON DOWN | PROGRAM | Pressed");
    delay(10);
    count++;
    //If button was held for more than 1 second, enter program mode
    if (count >= 70)
    {
      Serial.println("BUTTON DOWN | PROGRAM | Entered program mode");
      bool saveAndExit = false;
      while (true) //Begin Program Mode
      {
        Serial.println("BUTTON DOWN | PROGRAM | Waiting for instruction...");
        digitalWrite(LED_SYS, HIGH);
        delay(50);
        digitalWrite(LED_SYS, LOW);
        delay(50);

        //While in program mode, wait for a BUTTON DOWN to be debouncedly pressed
        if (debounceRead(BUTTON_DOWN, LOW) == HIGH)
        {
          long startTime = millis();
          saveAndExit = true;
          //While BUTTON DOWN is held, record the time elapsed and lower the desk
          while (digitalRead(BUTTON_DOWN))
          {
            Serial.println("BUTTON DOWN | PROGRAM | Recording DOWN...");
            goDown();
          }
          stopMoving();

          //If recording happened, save the resulting time to EEPROM and exit
          if (saveAndExit)
          {
            long endTime = millis();
            int elapsed = int(endTime - startTime);
            Serial.print("BUTTON DOWN | PROGRAM | Elapsed: ");
            Serial.println(elapsed);
            saveToEEPROM_TimeDown(elapsed);
            Serial.println("BUTTON DOWN | PROGRAM | Saved to EEPROM, Exiting Program Mode");
            break; //End Program Mode
          }
        }
      }
      //So that it doesn't go to play the recently saved program immediately after saving
      btnDownPRGState = LOW;
      count = 0;
    }
  }
}

// Tries to raise the desk automatically using the previously programmed values
// Only attempts to raise it if the desk is currently LOWERED and if a program exists
void autoRaiseDesk()
{
  Serial.print("IsUpSet?");
  Serial.println(savedProgram.isUpSet);
  Serial.print("IsDeskRaised?");
  Serial.println(savedProgram.isDeskRaised);
  if (savedProgram.isUpSet && !savedProgram.isDeskRaised)
  {
    //Get a backup copy of the program (in case power goes out mid-raise)
    StoredProgram prog = getEEPROMCopy();
    //We invalidate the program until we're certain the desk was fully lowered, then we re-save it
    invalidateEEPROM();
    bool isCancelled = false;
    long startTime = millis();
    while((millis() - startTime) < prog.timeUp){
      goUp();
      //if any button is pressed during program play, cancel and invalidate
      if (debounceRead(BUTTON_UP, LOW) || debounceRead(BUTTON_DOWN, LOW) || 
          debounceRead(BUTTON_UP_PRG, LOW) || debounceRead(BUTTON_DOWN_PRG, LOW))
      {
        isCancelled = true;
        stopMoving();
        break;
      }
    }
    stopMoving();
    //Only recuperate the values if the program wasn't cancelled
    if(!isCancelled){
      saveToEEPROM_DeskRaised(prog);
    }
  }
  else
  {
    Serial.println("Warning: Can't raise desk on current conditions");
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
  Serial.print("IsDeskRaised?");
  Serial.println(savedProgram.isDeskRaised);
  if (savedProgram.isDownSet && savedProgram.isDeskRaised)
  {
    //Get a backup copy of the program (in case power goes out mid-raise)
    StoredProgram prog = getEEPROMCopy();
    //We invalidate the program until we're certain the desk was fully lowered, then we re-save it
    invalidateEEPROM();
    bool isCancelled = false;
    long startTime = millis();
    while((millis() - startTime) < prog.timeDown){
      goDown();
      //if any button is pressed during program play, cancel and invalidate
      if (debounceRead(BUTTON_UP, LOW) || debounceRead(BUTTON_DOWN, LOW) || 
          debounceRead(BUTTON_UP_PRG, LOW) || debounceRead(BUTTON_DOWN_PRG, LOW))
      {
        isCancelled = true;
        stopMoving();
        break;
      }
    }
    stopMoving();
    if(!isCancelled){
      saveToEEPROM_DeskLowered(prog);  
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
  Serial.println("UP");
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
  Serial.println("DOWN");
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
  savedProgram.isDeskRaised = true;
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
  savedProgram.isDeskRaised = false;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
  successBlink();
}

void saveToEEPROM_DeskRaised(StoredProgram prog)
{
  Serial.println("Saving Desk is Raised to EEPROM");
  savedProgram.timeUp = prog.timeUp;
  savedProgram.isUpSet = prog.isUpSet;
  savedProgram.timeDown = prog.timeDown;
  savedProgram.isDownSet = prog.isDownSet;
  savedProgram.isDeskRaised = true;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
}

void saveToEEPROM_DeskLowered(StoredProgram prog)
{
  Serial.println("Saving Desk is Lowered to EEPROM");
  savedProgram.timeUp = prog.timeUp;
  savedProgram.isUpSet = prog.isUpSet;
  savedProgram.timeDown = prog.timeDown;
  savedProgram.isDownSet = prog.isDownSet;
  savedProgram.isDeskRaised = false;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
}

StoredProgram getEEPROMCopy(){
  StoredProgram prog;
  prog.timeUp = savedProgram.timeUp;
  prog.isUpSet = savedProgram.isUpSet;
  prog.timeDown = savedProgram.timeDown;
  prog.isDownSet = savedProgram.isDownSet;
  prog.isDeskRaised = savedProgram.isDeskRaised;
  return prog;
}

// Since this circuit doesn't have a way to detect if a motor has stalled then the recorded program 
// can only work if you exclusively use the program buttons after recording. if you press the manual
// up/down buttons then the recording is invalid as the height has changed and using the autoRaise/autoLower
// functions could try to spin the motors on a stall position, possibly causing damage.
void invalidateEEPROM()
{
  Serial.println("Invalidating EEPROM Program");
  savedProgram.timeUp = 0;
  savedProgram.isUpSet = false;
  savedProgram.timeDown = 0;
  savedProgram.isDownSet = false;
  savedProgram.isDeskRaised = false;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
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
  Serial.println("IsDeskRaised?");
  Serial.println(savedProgram.isDeskRaised);
  successBlink(); //TODO: Remove this
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

/*
  -----------------------
   Arduino Power Desktop
  -----------------------
  Summary: Controls the direction and speed of the motor.
  
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

#define enB 9
#define in3 6
#define in4 7
#define BUTTON_DOWN 3
#define BUTTON_UP 4

int btnDownPressed = 0;
int btnUpPressed = 0;

boolean goingDown = false;
boolean goingUp = false;

const int PWM_SPEED_UP = 255; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 200; //0 - 255, controls motor speed when going DOWN
const int PWM_ZERO = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_UP, INPUT); 
}

void loop() {
  btnDownPressed = digitalRead(BUTTON_DOWN);
  btnUpPressed = digitalRead(BUTTON_UP);

  if(btnDownPressed == true && btnUpPressed == true){
    //if both buttons are pressed at the same time, we cancel everything and ignore
    Serial.println("BOTH BUTTONS PRESSED");
    stopMoving();
  }else{
    if(btnDownPressed){
      goDown();
    }else if(goingDown){
      stopMoving();
    }else if(btnUpPressed){
      goUp();
    }else if(goingUp){
      stopMoving();
    }else{
      //If nothing is pressed, idle and turn off LED
      stopMoving();
      Serial.println("Waiting...");
    }
  }
}

void goDown(){
  Serial.println("DOWN");
  goingDown = true;
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(enB, PWM_SPEED_DOWN); //Send PWM signal to L298N enB pin (sets motor speed)
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void goUp(){
  Serial.println("UP");
  goingUp = true;
  analogWrite(enB, PWM_SPEED_UP); //Send PWM signal to L298N enB pin (sets motor speed)
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
}

void stopMoving(){
  goingDown = false;
  goingUp = false;
  analogWrite(enB, PWM_ZERO);
  digitalWrite(LED_BUILTIN, LOW);
}

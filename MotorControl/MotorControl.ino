#define in1 5
#define in2 6
//#define in3 10
//#define in4 11
#define BUTTON_DOWN 2
//#define BUTTON_UP 3

int btnDownPressed = 0;
int btnUpPressed = 0;
bool goingDown = false;
bool goingUp = false;

const int PWM_SPEED_UP = 255; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 255; //0 - 255, controls motor speed when going DOWN
const int PWM_ZERO = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  //pinMode(BUTTON_UP, INPUT);

  //Motor 1 (out1/out2 - in1/in2)
  Serial.println("Stop M1, High Side Brake");
  digitalWrite(in2, LOW);
  digitalWrite(in1, HIGH);

//  Serial.println("Stop M1, Low Side Brake");
//  digitalWrite(in2, HIGH);
//  digitalWrite(in1, LOW);

  Serial.println("Setup Complete");
}

void loop() {
  //Serial.println("In Loop...");
  //See truth table: https://www.pololu.com/file/0J710/A4990-Datasheet.pdf
  //Motor 1 (out1/out2 - in1/in2)
//  Serial.println("Clockwise");
//  digitalWrite(in2, HIGH);
//  analogWrite(in1, 255); //speed is directly proportional to number. i.e. 0 = stop, 255 = max speed

  //Motor 1 (out1/out2 - in1/in2)
//  Serial.println("Counter-Clockwise");
//  analogWrite(in2, 255); //speed is now inverse to the number. i.e. 0 = max speed, 255 = stop
//  digitalWrite(in1, LOW);
  
  
//  btnDownPressed = digitalRead(BUTTON_DOWN);
  //btnUpPressed = digitalRead(BUTTON_UP);

//  if(btnDownPressed == true && btnUpPressed == true){
//    //if both buttons are pressed at the same time, we cancel everything and ignore
//    Serial.println("BOTH BUTTONS PRESSED");
//    stopMoving();
//  }else{
//    if(btnDownPressed){
//      goDown();
//    }else if(goingDown){
//      stopMoving();
//    }else if(btnUpPressed){
//      goUp();
//    }else if(goingUp){
//      stopMoving();
//    }else{
//      //If nothing is pressed, idle and turn off LED
//      stopMoving();
//      Serial.println("Waiting...");
//    }
//    }
}

//void goUp(){
//  Serial.println("UP");
//  goingUp = true;
//  digitalWrite(LED_BUILTIN, HIGH);
//  //Motor A
//  digitalWrite(in1, LOW);
//  digitalWrite(in2, PWM_SPEED_UP);
//  
//  //Motor B
//  digitalWrite(in3, LOW);
//  digitalWrite(in4, PWM_SPEED_UP);
//}

void goDown(){
  Serial.println("DOWN");
  goingDown = true;
  digitalWrite(LED_BUILTIN, HIGH);
  
  //Motor A
  analogWrite(in1, LOW);
  digitalWrite(in2, PWM_SPEED_DOWN);
  
//  //Motor B
//  analogWrite(in3, PWM_SPEED_DOWN);
//  digitalWrite(in4, LOW);
}

void stopMoving(){
  goingDown = false;
  goingUp = false;
  Serial.println("Stopped Moving");
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
//  digitalWrite(in3, LOW);
//  digitalWrite(in4, LOW);
  digitalWrite(LED_BUILTIN, LOW);
}

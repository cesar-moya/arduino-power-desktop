#define enB 9
#define in3 6
#define in4 7
#define BUTTON_DOWN 3
#define BUTTON_UP 4

int btnDownPressed = 0;
int btnUpPressed = 0;

boolean goingDown = false;
boolean goingUp = false;

//About Max Speed: 40(12v,0.1mA) | 127(24.7v,0.25mA) | 185(26v,0.27mA) | 115 = 24.1v
const int PWM_SPEED_UP = 255; //0 - 255, controls motor speed
const int PWM_SPEED_DOWN = 200; //0 - 255, controls motor speed
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
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void goUp(){
  Serial.println("UP");
  goingUp = true;
  analogWrite(enB, PWM_SPEED_UP); //Send PWM signal to L298N enB pin (sets motor speed)
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
}

void stopMoving(){
  goingDown = false;
  goingUp = false;
  analogWrite(enB, PWM_ZERO);
  digitalWrite(LED_BUILTIN, LOW);
}

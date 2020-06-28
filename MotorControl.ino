#define enA 9
#define in1 6
#define in2 7
#define BUTTON_DOWN 3
#define BUTTON_UP 4

int btnDownState = 0;
boolean goingDown = false;
int btnUpState = 0;
boolean goingUp = false;
int PWM_MAXSPEED = 255; //0 - 255, controls motor speed
int PWM_ZERO = 0; //0 - 255, controls motor speed

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_UP, INPUT); 
}

void loop() {
  btnDownState = digitalRead(BUTTON_DOWN); //do we need to debounce?
  btnUpState = digitalRead(BUTTON_UP); //do we need to debounce?

  if(btnDownState == true && btnUpState == true){
    //if both buttons are pressed at the same time, we cancel everything and ignore
    Serial.println("BOTH BUTTONS PRESSED");
    stopMoving();
  }else{
    if(btnDownState){
      goDown();
    }else if(goingDown){
      stopMoving();
    }else if(btnUpState){
      goUp();
    }else if(goingUp){
      stopMoving();
    }else{
      analogWrite(enA, PWM_ZERO);
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Waiting...");
    }
  }
}

void goDown(){
  Serial.println("DOWN");
  goingDown = true;
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(enA, PWM_MAXSPEED); //Send PWM signal to L298N EnableA pin (sets motor speed)
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
}

void goUp(){
  Serial.println("UP");
  goingUp = true;
  analogWrite(enA, PWM_MAXSPEED); //Send PWM signal to L298N EnableA pin (sets motor speed)
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
}

void stopMoving(){
  goingDown = false;
  goingUp = false;
  analogWrite(enA, PWM_ZERO);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

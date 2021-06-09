#include <SoftwareSerial.h>

SoftwareSerial esp8266(2, 3); //Pin 2 y 3 Arduino RX y TX. TX y RX esp8266.
#define DEBUG true

int motor1pinF = 4;
int motor1pinB = 5;
int motor2pinF = 6;
int motor2pinB = 7;
int ledPin = 8;
int buzzerPin = 9;
int pingPin = 10;
int speedPin = 11;
int infraPin = 12;
int echoPin = 13;


unsigned long lastCommand;
unsigned long currentMillis;
unsigned long lastMillis;

unsigned long lastBuzzer;
unsigned long lastObjectFront;
unsigned long lastObjectBack;
bool frontObject;
bool backObject;
bool evadingFront, evadingBack;

int objectDistance = 25;
int waitTime = 25;
enum state {
    FRONT,
    BACK,
    LEFT,
    RIGHT,
    STOP
}
carState;

char input;
char format[4] = "car=";
int formatPos = 0;

void setup() {

    pinMode(ledPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(motor1pinF, OUTPUT);
    pinMode(motor1pinB, OUTPUT);
    pinMode(motor2pinF, OUTPUT);
    pinMode(motor2pinB, OUTPUT);
    pinMode(speedPin, OUTPUT);

    Serial.begin(115200);
    esp8266.begin(57600); //Baud rate esp8266

    //Esp8266 setup
    esp8266Serial("AT+RST\r\n", 4000, DEBUG); // Reset
    esp8266Serial("AT+CWMODE=1\r\n", 3000, DEBUG); //Station mode Operation
    esp8266Serial("AT+CWJAP=\"SSID\",\"PASSWORD\"\r\n", 5000, DEBUG);
    while (!esp8266.find("OK")) {}
    esp8266Serial("AT+CIFSR\r\n", 3000, DEBUG); //IP
    esp8266Serial("AT+CIPMUX=1\r\n", 3000, DEBUG);
    esp8266Serial("AT+CIPSERVER=1,80\r\n", 3000, DEBUG);

    lastCommand = millis();
    lastMillis = millis();
    carState = STOP;
    input = "";
    lastBuzzer = millis();
    lastObjectFront = millis();
    lastObjectBack = millis();
    
    frontObject = false;
    backObject = false;
    evadingFront = false;
    evadingBack = false;

    digitalWrite(ledPin, HIGH);
    analogWrite(speedPin, 150);
}

void loop() {
    while (esp8266.available()) {
      input = (char) esp8266.read();
      if(formatPos >= 0 && formatPos < 4){
        if(format[formatPos] == input) formatPos++;
        else formatPos = 0;
      }else if(formatPos == 4){
        switch(input){
          case 'F':
          case 'B':
          case 'L':
          case 'R':
          case 'S':
            moveCar(input);
            break;
        }
        if (DEBUG) Serial.println(input);
        formatPos = 0;
      }
    }

    //Move car if stopped and object in front or back
    if (carState == STOP) {
        if (isFrontObject(pingPin, objectDistance, waitTime)) {
            evadingFront = true;
            moveCar('B');
        }
        if (isBackObject(pingPin, waitTime)) {
            evadingBack = true;
            moveCar('F');
        }
    }

    //Stop car after evading object
    if(evadingFront && !isFrontObject(pingPin, objectDistance, waitTime)) {
      moveCar('S');
      evadingFront = false; 
    }
    if(evadingBack && !isBackObject(pingPin, waitTime)) {
      moveCar('S');
      evadingBack = false;
    }

    //Stop if object in front detected
    if (carState == FRONT) {
        if (isFrontObject(pingPin, objectDistance, waitTime)) {
            moveCar('S');
            buzz(buzzerPin);
        }
    }

    //Stop if object in back detected
    if (carState == BACK) {
        if (isBackObject(pingPin, waitTime)) {
            moveCar('S');
            buzz(buzzerPin);
        }
    }
    
    currentMillis = millis();
    
    //Stop buzzer after 40 millis
    if(currentMillis - lastBuzzer > 40){
        digitalWrite(buzzerPin,LOW);
    }
    
    //Stop if no input
    if (currentMillis - lastCommand > 5000) {
        moveCar('S');
        lastCommand = millis();
    }


}

//Checks if there is an object in front
bool isFrontObject(int pin, int distance, int wait) {
    int cm;
    cm = ping(pingPin);
    if(cm <= distance){
      currentMillis = millis();
      if(frontObject){
        if(currentMillis-lastObjectFront > wait) return true;
      }else{
        frontObject = true;
        lastObjectFront = currentMillis;
      }
    }else{
      frontObject = false;
    }
    return false;
}

//Checks if there is an object in the back
bool isBackObject(int pin, int wait) {
    bool isObject;
    isObject = !digitalRead(infraPin);
    if(isObject){
      currentMillis = millis();
      if(backObject){
        if(currentMillis-lastObjectBack > wait) return true;
      }else{
        backObject = true;
        lastObjectBack = currentMillis;
      }
    }else{
      backObject = false;
    }
    return false;
}

//Buzzing sound
void buzz(int pin) {
    lastBuzzer = millis();
    digitalWrite(pin, HIGH);
}

//Moves car in given direction
void moveCar(char dir) {
    lastCommand = millis();
    if (dir == 'F') {
        carState = FRONT;
        moveForward();
    } else if (dir == 'B') {
        carState = BACK;
        moveBackward();
    } else if (dir == 'L') {
        carState = LEFT;
        turnLeft();
    } else if (dir == 'R') {
        carState = RIGHT;
        turnRight();
    } else if (dir == 'S'){
        carState = STOP;
        stopMovement();
    }
}

//Pings ultrasonic sensor and returns distance to object
int ping(int pingPin) {
    long duration, cm;
    pinMode(pingPin, OUTPUT);
    digitalWrite(pingPin, LOW);
    delayMicroseconds(2);
    digitalWrite(pingPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(pingPin, LOW);

    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);

    cm = duration / 29 / 2;
    return cm;
}

void moveForward() {
    digitalWrite(motor1pinF, HIGH);
    digitalWrite(motor1pinB, LOW);

    digitalWrite(motor2pinF, HIGH);
    digitalWrite(motor2pinB, LOW);
}

void moveBackward() {
    digitalWrite(motor1pinF, LOW);
    digitalWrite(motor1pinB, HIGH);

    digitalWrite(motor2pinF, LOW);
    digitalWrite(motor2pinB, HIGH);
}

void turnLeft() {
    digitalWrite(motor1pinF, LOW);
    digitalWrite(motor1pinB, HIGH);

    digitalWrite(motor2pinF, HIGH);
    digitalWrite(motor2pinB, LOW);
}

void turnRight() {
    digitalWrite(motor1pinF, HIGH);
    digitalWrite(motor1pinB, LOW);

    digitalWrite(motor2pinF, LOW);
    digitalWrite(motor2pinB, HIGH);
}

void stopMovement() {
    digitalWrite(motor1pinF, LOW);
    digitalWrite(motor1pinB, LOW);

    digitalWrite(motor2pinF, LOW);
    digitalWrite(motor2pinB, LOW);
}

//Sends command to esp8266
String esp8266Serial(String command,
    const int timeout, boolean debug) {
    String response = "";
    esp8266.print(command);
    long int time = millis();
    while ((time + timeout) > millis()) {
        while (esp8266.available()) {
            char c = esp8266.read();
            response += c;
        }
    }
    if (debug) {
        Serial.print(response);
    }
    return response;
}

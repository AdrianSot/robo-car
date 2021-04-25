#include <SoftwareSerial.h>

SoftwareSerial esp8266(2, 3); //Pin 2 y 3 Arduino RX y TX. TX y RX esp8266.
#define DEBUG true

int motor1pinF = 4;
int motor1pinB = 5;
int motor2pinF = 6;
int motor2pinB = 7;
int buzzerPin = 8;
int ledPin = 9;
int pingPin = 10;
int echoPin = 11;

unsigned long lastCommand;
unsigned long currentMillis;
unsigned long lastMillis;

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

String input;

void setup() {

    pinMode(ledPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(motor1pinF, OUTPUT);
    pinMode(motor1pinB, OUTPUT);
    pinMode(motor2pinF, OUTPUT);
    pinMode(motor2pinB, OUTPUT);

    Serial.begin(115200);
    esp8266.begin(57600); //Baud rate esp8266

    //Esp8266 setup
    esp8266Serial("AT+RST\r\n", 5000, DEBUG); // Reset
    esp8266Serial("AT+CWMODE=1\r\n", 5000, DEBUG); //Station mode Operation
    esp8266Serial("AT+CWJAP=\"UbeeD566-2.4G\",\"6F38FAD566\"\r\n", 5000, DEBUG);
    while (!esp8266.find("OK")) {}
    esp8266Serial("AT+CIFSR\r\n", 5000, DEBUG); //IP
    esp8266Serial("AT+CIPMUX=1\r\n", 5000, DEBUG);
    esp8266Serial("AT+CIPSERVER=1,80\r\n", 5000, DEBUG);

    lastCommand = millis();
    lastMillis = millis();
    carState = STOP;
    input = "";

    digitalWrite(ledPin, HIGH);
}

void loop() {
    //Input from esp8266
    while (esp8266.available()) {
        input += (char) esp8266.read();
    }

    //Process input and check for command
    if (input.length() > 200) {
        int index = input.indexOf("car");
        if (index != -1) {
            if (index + 6 < input.length()) {
                String c1 = input.substring(index, index + 3);
                String c2 = input.substring(index + 4, index + 6);
                if (DEBUG) {
                    Serial.println(c1);
                    Serial.println(c2);
                }
                moveCar(c2);
                input = input.substring(index + 7);
            } else input = input.substring(index);
        } else input = "";
    }

    //Stop if object detected
    if (carState == FRONT) {
        if (isFrontObject(pingPin, objectDistance, waitTime)) {
            carState = STOP;
            stopMovement();
            buzz(buzzerPin);
        }
    }

    //Stop if no input
    currentMillis = millis();
    if (currentMillis - lastCommand > 5000) {
        stopMovement();
        lastCommand = millis();
    }

}

//Checks if there is an object in front
bool isFrontObject(int pin, int distance, int wait) {
    int cm;
    for (int i = 0; i < wait; i++) {
        delay(1);
        cm = ping(pingPin);
        if (cm > distance) {
            return false;
        }
    }
    return true;
}

//Buzzing sound
void buzz(int pin) {
    digitalWrite(pin, HIGH);
    delay(15);
    digitalWrite(pin, LOW);
    delay(25);
    digitalWrite(pin, HIGH);
    delay(15);
    digitalWrite(pin, LOW);
}

//Moves car in given direction
void moveCar(String dir) {
    lastCommand = millis();
    if (dir == "FW") {
        carState = FRONT;
        moveForward();
    } else if (dir == "BW") {
        carState = BACK;
        moveBackward();
    } else if (dir == "LT") {
        carState = LEFT;
        turnLeft();
    } else if (dir == "RT") {
        carState = RIGHT;
        turnRight();
    } else {
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

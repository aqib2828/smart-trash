#include <MQTT.h>
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C_Spark.h>
#define HOST_PORT 1883
#define MQTT_QoS 0
char *MQTT_HOST = "xzecbm.messaging.internetofthings.ibmcloud.com";
char *MQTT_CLIENT = "d:xzecbm:Photon:Aqib-IoT";
char *MQTT_USERNAME = "use-token-auth";
char *MQTT_PASSWORD = "2tYJllr6V_7NW0BM-K";
char *EVENT_TOPIC = "iot-2/evt/TrashLevel/fmt/json";
char *COMMAND_TOPIC = "iot-2/cmd/testcommand/fmt/json";
MQTT client( MQTT_HOST, HOST_PORT, callback );
char payload[80];
StaticJsonDocument<200> CommdJSONDocument;
LiquidCrystal_I2C *lcd; //SDA - D0 , SCL - D1
Servo myservo;
const int pingTrigHand = A4; //Trig needs PWM | TrigA/EchoA dectects motion
above lid
const int pingEchoHand = A2;
const int pingTrigTrash = A5; //TrigB/EchoB detects trash level
const int pingEchoTrash = A3;
int servoPin = D2;
int ldrPin = A1;
int greenLED = 6;
int pos = 180 ;
int lightValue, trashPercent;
int payloadCounter = 0;
int counter = 0;
long durationHand, distOfHand, average;
long mean[3]; //array to store 3 values
char percent[4] = "";
void setup() {
  Particle.variable("TrashLevel", &trashPercent, INT);
  //Serial.begin(9600);
  client.connect( MQTT_CLIENT , MQTT_USERNAME , MQTT_PASSWORD );
  client.subscribe( COMMAND_TOPIC );
  lcd = new LiquidCrystal_I2C(0x27, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  Time.zone(-4);
  if (Time.weekday() == 2 || Time.weekday() == 4 || Time.weekday() == 6) {
    lcd->setCursor(0, 0);
    lcd->print("Today is trash");
    lcd->setCursor(0, 1);
    lcd->print("collection day");
    delay(10000);
    lcd->clear();
  }
  lcd->setCursor(0, 0);
  lcd->print("Trash Level:");
  pinMode(greenLED, OUTPUT);
  pinMode(pingTrigTrash, OUTPUT);
  pinMode(pingEchoTrash, INPUT);
  pinMode(pingTrigHand, OUTPUT);
  pinMode(pingEchoHand, INPUT);
  myservo.attach(servoPin);
  myservo.write(0);
  delay(1000);
  myservo.detach();
}
void loop() {
  lightValue = analogRead(ldrPin);
  if (lightValue > 400) {
    digitalWrite(greenLED, HIGH);
    digitalWrite(pingTrigTrash, LOW); //sets the Trigger pin detecting the
    trash percentage to low
    delayMicroseconds(5);
    digitalWrite(pingTrigTrash, HIGH);
    delayMicroseconds(10);
    digitalWrite(pingTrigTrash, LOW);
    long giveupTime = millis() + 100;
    while (digitalRead(pingEchoTrash) != HIGH && millis() < giveupTime) {}
    long startTime = micros();
    giveupTime = millis() + 100;
    while (digitalRead(pingEchoTrash) != LOW && millis() < giveupTime) {}
    long endTime = micros();
    long durationTrash = endTime - startTime;
    double trashDistance = durationTrash / 29.0 / 2.0;
    trashDistance = constrain(trashDistance, 4.0, 28.0);
    trashPercent = round(((28.0 - trashDistance) / 24.0) * 100);
    sprintf(payload, "{ \"TrashLevel\": \"%d\" }", trashPercent);
    if (trashPercent == 100) {
      client.publish(EVENT_TOPIC, const_cast<char*> (payload) );
    }
    client.publish(EVENT_TOPIC, const_cast<char*> (payload) );
    client.loop();
    lcd->setCursor(0, 1);
    sprintf(percent, "%03d", trashPercent);
    lcd->print(percent);
    lcd->print("% Full");
    delay(100);
    for (int i = 0; i <= 2; i++) {
      detection();
      mean[i] = distOfHand;
      delay(50);
    }
    distOfHand = (mean[0] + mean[1] + mean [2]) / 3;
    if (distOfHand < 30) {
      myservo.attach(servoPin);
      delay(1);
      myservo.write(180);
      delay(3500);
      for (pos = 180; pos >= 0; pos -= 2) {
        myservo.write(pos);
        delay(15);
      }
      delay(1000);

      myservo.detach();
    }
  }
  else {
    digitalWrite(greenLED, LOW);
  }
}
void detection() {
  digitalWrite(pingTrigHand, LOW);
  delayMicroseconds(5);
  digitalWrite(pingTrigHand, HIGH);
  delayMicroseconds(15);
  digitalWrite(pingTrigHand, LOW);
  pinMode(pingEchoHand, INPUT);
  durationHand = pulseIn(pingEchoHand, HIGH); //durationB if the uds doesn't
  work
  distOfHand = (durationHand / 2) / 29.1;
}
void callback( char* topic, byte* payload, unsigned int length ) {
  char p[length + 1];
  memcpy( p, payload, length );
  p[length] = NULL;
  String message( p );
  deserializeJson(CommdJSONDocument, p);
  const char* Command1Value = CommdJSONDocument["testcommand"];
  if (!strcmp(Command1Value, "hundred"))
  {
    myservo.attach(servoPin);
    myservo.write(180);
  }
}

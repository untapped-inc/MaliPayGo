#include <ArduinoJson.h>
#include <SD.h>
#include <WiFiNINA.h>
#include "WiFiCredentials.h"
#include <EEPROM.h>
#include <ArduinoHttpClient.h>
 
//September 2019: This code is a stopgap for the hardware in Mali. It is intended to count pulses flowing through the  flowmeter, enable/disable the relay based on current volume, and transmit volume consumed to a remote API endpoint

#define FLOWMETER_PIN A0
#define ORP_PIN A1
#define RELAY_PIN 13
//this is how many units the analog read must differ between readings in order to be considered a rising or falling edge
#define FLOWMETER_THRESHOLD 100
#define URL "maliapi.azurewebsites.net"
#define DEVICE_ID 1
#define REMAINING_VOLUME_ADDRESS 0
#define VSLP_ADDRESS 20
#define ERROR_CODE -9999
#define SUCCESSFUL_RESPONSE_CODE 200

//the number of milliliters that have been detected by the sensor per pulse (rising and falling edge)
const int ML_PER_PULSE = 500;

//variable to store the flowmeter readings for future comparison
float previousFlowmeter = -9999;


WiFiClient wifi;
HttpClient client = HttpClient(wifi, URL, 80);
int status = WL_IDLE_STATUS;

long startTime = 0;
long startingMillis;

long previousORPSampleTime;
long previousWifiTime;
//total milliseconds per ORP sample 5 minutes
const long ORP_SAMPlE_MILLIS = 20000; //todo: set this back after testing to 300000;
// interval at which to communicate (milliseconds)
const int WIFI_INTERVAL = 10000;

//variables to monitor water consumption
long pulseA = 0;
long pulseB = 0;
bool isPulseA = true;
int remainingVolume = 0;
//this variable tracks how much water has been used since the last API post was successful
int volumeSinceLastPost = 0;

//used for JSON Deserilization
StaticJsonDocument<1024> jsonDoc1;
StaticJsonDocument<1024> jsonDoc2;

void setup() {
  Serial.begin(9600);
  pinMode(FLOWMETER_PIN, INPUT_PULLUP);
  pinMode(ORP_PIN, INPUT);
  Serial.println("Starting");
  status = WiFi.begin(WifiSSID, WifiPassword);
  
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    delay(500);
    Serial.println("Waiting for connection");
  }
  
  initTimekeeping();
  
  //data stored in eeprom - pull this data out of memory
  EEPROM.get(REMAINING_VOLUME_ADDRESS, remainingVolume);
  EEPROM.get(VSLP_ADDRESS, volumeSinceLastPost);
  
  pinMode(RELAY_PIN, OUTPUT);
  //make sure that the water is off when we start
  turnWaterOff();

  previousWifiTime = previousORPSampleTime;
  
  //setup a timer interrupt to read from the analog pin - based on examples at https://forum.arduino.cc/index.php?topic=614128.0 and https://forum.arduino.cc/index.php?topic=616813.0
  TCB1.CTRLB = TCB_CNTMODE_INT_gc; // timer compare mode
  TCB1.CCMP = 125000; //The processor is 250k Hz. This number is the number of clock cycles per interrupt (i.e. 125,000 = 2 HZ)
  TCB1.INTCTRL = TCB_CAPT_bm; // Enable the interrupt
  TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm; // Use Timer A as clock, enable timer
}

void loop() {
  long loopTime = millis();

  //convert pulses to liters and deduct from the balance
  if (isPulseA){
    //switch which variables is being written to, so that incoming pulses aren't lost during calculation and transmission
    isPulseA = !isPulseA;
    volumeSinceLastPost += (pulseA * ML_PER_PULSE);
    //reset pulses
    pulseA = 0;
  }else{
    isPulseA = !isPulseA;
    volumeSinceLastPost += (pulseB * ML_PER_PULSE);
    pulseB = 0;
  }
  Serial.print("VSLP: ");
  Serial.println(volumeSinceLastPost);
  remainingVolume -= volumeSinceLastPost;


  //write the remaining volume to memory in case the device crashes
  EEPROM.put(REMAINING_VOLUME_ADDRESS, remainingVolume);

  //turn off the pump/valve if the balance is empty
  if (remainingVolume <= 0 ){
    turnWaterOff();
  }else{
    turnWaterOn();
  }
  
  //check to make sure that enough time has elapsed before attempting communication
  if (loopTime - previousWifiTime > WIFI_INTERVAL){
    previousWifiTime = loopTime;
    //send the volume consumed 
    String requestBody = 
    "{\n    \"clientReadings\": {\n        \"sensorA\": [],\n        \"sensorB\": []\n },\n    \"deviceId\": " + String(DEVICE_ID) + ",\n    \"millilitersConsumed\" : " + String(volumeSinceLastPost) + "\n}";
          
    //send the request
    int responseFromServer = postData(requestBody);
    if (responseFromServer != ERROR_CODE){
      //if the POST succeeds, then overwrite the total ML with whatever the server tells us
      remainingVolume = responseFromServer;
      //reset the volume since last post
      volumeSinceLastPost = 0;
    }
  }
  
  delay(300);
}


/***Helper Functions***/

long getCurrentTime(){
  return startTime + ( millis()-startingMillis);
}

void initTimekeeping(){
  //get the starting time
  while(startTime == 0){
    Serial.println("Getting Time");
    startTime = WiFi.getTime();
  }
  startingMillis = millis();
}

//returns the ORP meter's reading in millivolts
float getORPReading(){
  //it's just an analog read
  return analogRead(ORP_PIN);
}

void turnWaterOff(){
  digitalWrite(RELAY_PIN, HIGH);
}

void turnWaterOn(){
  digitalWrite(RELAY_PIN, LOW);
}

//This function is used to count pulses coming in from the FD-Q Flowmeter
ISR(TCB1_INT_vect)
{
    //read from the flowmeter
    float currentFlowmeter = analogRead(FLOWMETER_PIN);
    Serial.println(currentFlowmeter);
    //check for either rising or falling edge
    if (previousFlowmeter != -9999 && (abs(currentFlowmeter - previousFlowmeter) > FLOWMETER_THRESHOLD)){
      //add either pulse A or Pulse B
      if (isPulseA){
        pulseA++;
      }else{
        pulseB++;
      }
    }
    //set this variable for future comparisons
    previousFlowmeter = currentFlowmeter;

   // Clear interrupt flag
   TCB1.INTFLAGS = TCB_CAPT_bm;
}

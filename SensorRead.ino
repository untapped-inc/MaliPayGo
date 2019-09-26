#include <ArduinoJson.h>
#include <SD.h>
#include <WiFiNINA.h>
#include "WiFiCredentials.h"
#include <EEPROM.h>
 
//September 2019: This code is a stopgap for the hardware in Mali. It is intended to count pulses flowing through the  flowmeter, enable/disable the relay based on current volume, and transmit volume consumed to a remote API endpoint

#define FLOWMETER_PIN A0
#define ORP_PIN A1
#define RELAY_PIN 13
//this is how many units the analog read must differ between readings in order to be considered a rising or falling edge
#define FLOWMETER_THRESHOLD 100
#define URL "maliapi.azurewebsites.net"
#define DEVICE_ID 1
#define REMAINING_VOLUME_ADDRESS 0

//the number of milliliters that have been detected by the sensor per pulse (rising and falling edge)
const int ML_PER_PULSE = 500;

//variable to store the flowmeter readings for future comparison
float previousFlowmeter = -9999;

WiFiClient client;
int status = WL_IDLE_STATUS;

long startTime = 0;
long startingMillis;

long previousORPSampleTime;
long previousWifiTime;
//total milliseconds per ORP sample 5 minutes
const long ORP_SAMPlE_MILLIS = 20000; //todo: set this back after testing to 300000;
// interval at which to communicate (milliseconds)
const int WIFI_INTERVAL = 1000;

//set to true when it's time to send new sensor data up
bool newDataExists = false;

//variables to monitor water consumption
long pulseA = 0;
long pulseB = 0;
int remainingVolume = 0;
bool isPulseA = true;

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
  remainingVolume = 20000;

  pinMode(RELAY_PIN, OUTPUT);
  //make sure that the water is off when we start
  turnWaterOff();
  //track time to make sure we only sample once per 5 minutes
  previousORPSampleTime = millis();
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
    remainingVolume -= (pulseA * ML_PER_PULSE);
    //reset pulses
    pulseA = 0;
  }else{
    isPulseA = !isPulseA;
    remainingVolume -= (pulseB * ML_PER_PULSE);
    pulseB = 0;
  }

  //write the remaining volume to memory in case the device crashes
  EEPROM.put(REMAINING_VOLUME_ADDRESS, remainingVolume);

  //turn off the pump/valve if the balance is empty
  if (remainingVolume <= 0 ){
    Serial.println("turning relay off");
    Serial.println(remainingVolume);
    turnWaterOff();
  }else{
    turnWaterOn();
  }
  
  /*
    //this deals with communication when the water is not flowing
    //check to make sure that enough time has elapsed before attempting communication
    if (loopTime - previousWifiTime > WIFI_INTERVAL){
      previousWifiTime = loopTime;
      //if there is new data, send it, otherwise check the API
      if (!newDataExists){
        //sends the GET HTTP request
        //getData();
      }else{
        //todo: send the data - this needs to be implemented in future versions
        //String flowmeterData = readFile(FLOWMETER_FILENAME);
        //String orpData = readFile(ORP_FILENAME);
        /*Serial.print("flowmeter Data: ");
        Serial.println(flowmeterData);
        

        //orpData.trim();

        String orpData = "";
               
        unsigned long createdAtTime = getCurrentTime();


        //todo: remove this variable after testing is done
        currentWaterAmount = 12345;
        
        String requestBody = 
        "{\"clientReadings\":{\"sensorA\": [],\"sensorB\": [ " + orpData + "] },\"clientDevice\":{ \"id\": "
        + String(DEVICE_ID) +
        ",\"current_water_amount\": " + currentWaterAmount +
        ",\"max_water_amount\":{ \"value\": " + maxWaterAmount + ", \"created_at\": " + createdAtTime + " }}}";
        
       //Serial.print("request body: ");
       //Serial.println(requestBody);
        postData(requestBody);

        //clean up the old data
        //SD.remove(FLOWMETER_FILENAME);
        //July 10, 2019: this file isn't being overwritten because we can't send it up to the endpoint yet
        //SD.remove(ORP_FILENAME);
        newDataExists = false;
      }
    }
    //check for the response to the HTTP GET Request - this sets the max water and current water variables
    checkResponse();
    

    //decide if we should turn the water on
    if (currentWaterAmount > 0){
      turnWaterOn();
    }
  }  

  //ORP readings every 5 minutes
  if (loopTime - previousORPSampleTime > ORP_SAMPlE_MILLIS){
    int ORPReading = getORPReading(); //in mv
    previousORPSampleTime = loopTime;
    //grab reading from ORP Sensor
    appendSensor(ORPReading, false);
    newDataExists = true;
  } */

  delay(300);
}


/***Helper Functions***/

String readFile(const char *filename){
  Serial.println(filename);
  String fileContents;
  File sensorFile = SD.open(filename);
  if (sensorFile){
    while(sensorFile.available()){
     fileContents += (char)sensorFile.read();
      //Serial.write(sensorFile.read());
    }
    sensorFile.close();
  }else{
    Serial.println("Failed to open for read");
  }
  return fileContents;
}

void appendSensor(unsigned long value, bool isSensorA){
  //todo: in future versions, allow the ORP to be appended
  File sensorFile;
  /*
  if (isSensorA){
    sensorFile =  SD.open(FLOWMETER_FILENAME, FILE_WRITE);
  }else{
    sensorFile = SD.open(ORP_FILENAME, FILE_WRITE);
  }
  */
  //if (sensorFile){
    char buffer[1000];
    long unixTime = getCurrentTime();
    sprintf(buffer, "{\"created_at\": %lu, \"value\": %lu }", unixTime, value); 
    //Serial.println(buffer);
    //sensorFile.println(buffer);
  /*}else{
    Serial.println("Failed to open for print");
  }
  sensorFile.close();*/
}


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

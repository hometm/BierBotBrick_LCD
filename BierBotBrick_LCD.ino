//OTA (Laden über WIFi) Passwort: admin

#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>


#define LED_PIN D4
#define BEEP_PIN D5
#define BTN1_PIN 0
#define BTN2_PIN 0
#define CYCLE_SLEEP 100 

//CFG
#define URL "europe-west1-bierbot-cloud.cloudfunctions.net"
#define URL_PATH "/iot_staging"
#define HTTP_PORT 443
#define APIKEY  "[BRICK API KEY]"
#define CHIPID  "foo123"
#define TYPE  "display"
#define BRAND "oss"
#define VERSION  "0.0.1"
#define HTTP_REQU_LEN 200
#define HOSTNAME "BierTimer"
#define ALARM1 120 // 1st Alarm before next step
#define ALARM1_DURATION 2
#define ALARM2 30 // 2nd Alarm before next step
#define ALARM2_DURATION 5
const char* SSID = "[NETWORK SSID]";
const char* PSK = "[NETWORK WPS KEY]";


LiquidCrystal_I2C lcd(0x27, 20, 4);
StaticJsonDocument<200> docReq;
StaticJsonDocument<1000> docRes;
char httpRequest[HTTP_REQU_LEN];
WiFiClientSecure clientSecure;
bool ledState;
float tempPri=0;
float tempSek=0;
float tempMlt=0;
float tempHlt=0;
String tempUnit="";
String nextStep;
String currStep;
int targetTemp;
int timeRemaining;
String timeRemainingUnit;

uint32_t nextReadMS;
uint32_t readCycleMS=5000;

enum LED{LED_ON, LED_OFF, LED_BLINK}led;
enum TEMP_SENSOR{PRI, PRI_SEK, MLT_HLT, NOTEMP}tSensoren;

void setup() 
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(BEEP_PIN, OUTPUT);

  Serial.begin(115200);

  //LCD init
  lcd.init();
  lcd.clear();
  lcd.backlight();

  //WIFI
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(SSID, PSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Connected to ");
  Serial.println(SSID);

  //OTA Load via WIFI
  ArduinoOTA.onStart([]() {
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH)
    type = "sketch";
  else // U_SPIFFS
    type = "filesystem";
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  //Config http post
  docReq["apikey"] = APIKEY;
  docReq["type"] = TYPE;
  docReq["brand"] = BRAND;
  docReq["version"] = VERSION;
  docReq["chipid"] = CHIPID;
  serializeJson(docReq,httpRequest, HTTP_REQU_LEN);
  
  clientSecure.setInsecure();//skip verification

  tSensoren=NOTEMP;
}

void loop() 
{
  ArduinoOTA.handle();

  // Read process values
  if (millis() > nextReadMS)
  {
    clientSecure.connect(URL, HTTP_PORT);
    String Req=String("POST ") + URL_PATH + " HTTP/1.1\r\n" +
      "Host: " + URL + "\r\n" +
      "Connection: close\r\n" +
      "Content-Type: application/json\r\n" +
      "Content-Length: " + String(httpRequest).length() + "\r\n" +
      "\r\n" +
      httpRequest + "\r\n";
    clientSecure.print(Req);

    while (clientSecure.connected()) 
    {
      String line = clientSecure.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    String answer="";
    while (clientSecure.available()) 
    {
      char c = clientSecure.read();
      answer+=c;
    }
    Serial.println(answer);
    clientSecure.stop();

    DeserializationError error = deserializeJson(docRes, answer);
    if (error) 
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return;
    }

    // Sead sensoren and static data only at first run
    if(tSensoren==NOTEMP)
    {
      JsonVariant mlt = docRes["brews"][0]["currentTemperature"]["mlt"];
      JsonVariant hlt = docRes["brews"][0]["currentTemperature"]["hlt"];
      JsonVariant pri = docRes["brews"][0]["currentTemperature"]["primary"];
      JsonVariant sec = docRes["brews"][0]["currentTemperature"]["secondary"];
      tempUnit = docRes["config"]["temperatureUnit"].as<String>();
      
      if (!mlt.isNull() && !hlt.isNull())
        tSensoren=MLT_HLT;
      else if(!pri.isNull() && !sec.isNull())  
        tSensoren=PRI_SEK;
      else if(!pri.isNull())  
        tSensoren=PRI;
    }
    if (tSensoren==MLT_HLT)    
    {
      tempMlt=docRes["brews"][0]["currentTemperature"]["mlt"];
      tempHlt=docRes["brews"][0]["currentTemperature"]["hlt"];
    }
    else if(tSensoren==PRI_SEK)
    {
      tempSek=docRes["brews"][0]["currentTemperature"]["secendary"];
      tempPri=docRes["brews"][0]["currentTemperature"]["primary"];
    }
    else if(tSensoren==PRI)
    {
      tempPri=docRes["brews"][0]["currentTemperature"]["primary"];
    }
    currStep=docRes["brews"][0]["currentTemperature"]["nextEvents"][0]["type"].as<String>();
    if(currStep.length()>=20)
      currStep=currStep.substring(0,19);
    else
      for (int i=currStep.length();i++;i<20)
        currStep+=" ";
    nextStep=docRes["brews"][0]["currentTemperature"]["nextEvents"][1]["type"].as<String>();
    if(nextStep.length()>=20)
      nextStep=nextStep.substring(0,19);
    else 
      for (int i=nextStep.length();i++;i<20)
        nextStep+=" ";
    targetTemp=docRes["brews"][0]["targetTemperatureC"];
    int timeRemainingS=docRes["brews"][0]["currentTemperature"]["nextEvents"][0]["timeRemainingS"];
    int timeRemaining= timeRemainingS;
    if(timeRemaining>3600*24)
    {
      timeRemaining=timeRemaining/60/24;
      timeRemainingUnit="h";
    }
    else if(timeRemaining>3600)
    {
      timeRemaining=timeRemaining/60;
      timeRemainingUnit="m";
    }
    else
    {
      timeRemainingUnit="s";
    }

    readCycleMS= docRes["nextRequestMs"];
    nextReadMS=millis() + readCycleMS;
  

/*
    if(timeRemainingS<= ALARM1 &&  timeRemainingS > ALARM1 - ALARM1_DURATION )
    {
      tone(BEEP_PIN, 1000);
      led=LED_BLINK;
    }
    else if(timeRemainingS-ALARM2 && timeRemainingS>ALARM2-ALARM2_DURATION )
    {
      tone(BEEP_PIN, 2000 );
      led=LED_ON;
    }
    else
    {
      noTone(BEEP_PIN);
      led=LED_OFF;
    }
    */

    lcd.setCursor(0,0);
    lcd.print(currStep);
    lcd.setCursor(0,1);
    {
      char targ[4];
      sprintf(targ, "%3d", targetTemp);
      char tim[5];
      sprintf(tim, "%4d", timeRemaining);
      lcd.print("SetP:"+String(targ)+tempUnit+" Rem:"+String(tim)+timeRemainingUnit);
    }
    lcd.setCursor(0,2);
    if(tSensoren==MLT_HLT)
    {
      char ml[4];
      sprintf(ml, "%3d", tempMlt);
      char hl[4];
      sprintf(hl, "%3d", tempHlt);
      lcd.print("Mlt:"+String(ml)+tempUnit+" Hlt:"+String(hl)+tempUnit);
    }
    else if (tSensoren==PRI_SEK)
    {
      char pr[4];
      sprintf(pr, "%3d", tempPri);
      char se[4];
      sprintf(se, "%3d", tempSek);
      lcd.print("Pri:"+String(pr)+tempUnit+" Sek:"+String(se)+tempUnit);
    }
    else if (tSensoren==PRI)
    {
      char pr[4];
      sprintf(pr, "%3d", tempPri);
      lcd.print("Pri:"+String(pr)+tempUnit);
    }
    lcd.setCursor(0,3);
    lcd.print(nextStep);
  }

/*
  if(led==LED_BLINK)
    ledState!=ledState;
  else if(led==LED_ON)
    ledState=true;
  else
    ledState=false;  
  digitalWrite(LED_PIN, ledState);
*/

  delay(CYCLE_SLEEP);
}


#include <WiFi.h>     
#include <DNSServer.h>
#include <ESP32WebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Incubator.h"
#include <BlynkSimpleEsp32.h>

Incubator Incubator;
Adafruit_SSD1306 display(OLED_RESET);
ESP32WebServer server(80);

//Variables
float t_avg = 0.0;
float t_set = 0.0;
float t_1 = 0.0;
float t_2 = 0.0;
float t_h = 0.0;
float co2_avg = 0.0;
float co2 = 0.0;
float co2_set = 0.0;

//BLYNK APP==========================================================================================================
char auth[] = "2ad9f7eadf954d90bc1f028a0a4d5ce1";

//Check for errors
void test_error(){
  //Temperature
  if(Incubator.system_notifications)
  {
    if (Incubator.ERRORS.temp_sensor_1 == 10){
      if(Incubator.send_mail){Blynk.email(Incubator.mail.c_str(), "Subject: Temp sensor ERROR", "Temperature sensor 1 stoped working..."+String(t_1));};
      if(Incubator.send_push){Blynk.notify("Temperature sensor 1 stoped working!");};
    }
    if (Incubator.ERRORS.temp_sensor_2 == 10){
      if(Incubator.send_mail){Blynk.email(Incubator.mail.c_str(), "Subject: Temp sensor ERROR", "Temperature sensor 2 stoped working..."+String(t_2));};
      if(Incubator.send_push){Blynk.notify("Temperature sensor 2 stoped working!");};
    }
    if (Incubator.ERRORS.heater_sensor == 10){
      if(Incubator.send_mail){Blynk.email(Incubator.mail.c_str(), "Subject: Temp sensor ERROR", "Temperature heater sensor stoped working...");};
      if(Incubator.send_push){Blynk.notify("Temperature heater sensor stoped working!");};
    }
    if (Incubator.ERRORS.alert == 1){
      Incubator.ERRORS.alert = 0;
      if(Incubator.send_mail){Blynk.email(Incubator.mail.c_str(), "Subject: Critical ERROR", "Critical error. System reboot!");};
      if(Incubator.send_push){Blynk.notify("Critical error! System reboot!");};
    }
    if (Incubator.ERRORS.overheat == 1){
      if(Incubator.send_mail){Blynk.email(Incubator.mail.c_str(), "Subject: Overheat", "System is overheating!");};
      if(Incubator.send_push){Blynk.notify("System is overheating!");};
    }
  }
  //CO2
}

//Send captions
void sendCaptions()
{
  Blynk.virtualWrite(V6, "TEMPERATURE");
  Blynk.virtualWrite(V7, "CO2 LEVEL");
  Blynk.virtualWrite(V18, "TEMPERATURE ERRORS");
  Blynk.virtualWrite(V19, "CO2 ERRORS");
  Blynk.virtualWrite(V20, "DOOR OPEN");
  Blynk.virtualWrite(V23, "SYSTEM ERRORS");
}
//Write incubator name
BLYNK_READ(V13)
{
  String str = "INCUBATOR: " + Incubator.get_incubator_name();
  Blynk.virtualWrite(V13, str);
}
//Write average T
BLYNK_READ(V0)
{
  Blynk.virtualWrite(V0, t_avg);
}
//Write sensor T1
BLYNK_READ(V1)
{
  Blynk.virtualWrite(V1, t_1);
}
//Write sensor T2
BLYNK_READ(V2)
{
  Blynk.virtualWrite(V2, t_2);
}
//Write sensor heater
BLYNK_READ(V3)
{
  Blynk.virtualWrite(V3, t_h);
}
//Write set temperature
BLYNK_READ(V4)
{
  Blynk.virtualWrite(V4, t_set);
}
//Read set temperature
BLYNK_WRITE(V5)
{
  float x = param[0].asFloat();
  t_set = x;
}
//Write average co2
BLYNK_READ(V8)
{
  Blynk.virtualWrite(V8, co2_avg);
}
//Read set co2
BLYNK_WRITE(V9)
{
  float x = param[0].asFloat();
  co2_set = x;
}
//Write set co2
BLYNK_READ(V10)
{
  Blynk.virtualWrite(V10, co2_set);
}
//Write co2
BLYNK_READ(V11)
{
  Blynk.virtualWrite(V11, co2);
}
//Read mail
BLYNK_WRITE(V13)
{
  String x = param[0].asString();
  Incubator.set_mail(x);
}
//Read push notifications settings
BLYNK_WRITE(V14)
{
  int x = param[0].asInt();
  Incubator.set_push_notifications(x);
}
//Read set temp notifications
BLYNK_WRITE(V15)
{
  int x = param[0].asInt();
  Incubator.set_temp_notifications(x);
}
//Read set co2 notifications
BLYNK_WRITE(V16)
{
  int x = param[0].asInt();
  Incubator.set_co2_notifications(x);
}
//Read set door notifications
BLYNK_WRITE(V17)
{
  int x = param[0].asInt();
  Incubator.set_door_notifications(x);
}
//Read set mail notifications
BLYNK_WRITE(V21)
{
  int x = param[0].asInt();
  Incubator.set_mail_notifications(x);
}
//Read set system notifications
BLYNK_WRITE(V22)
{
  int x = param[0].asInt();
  Incubator.set_system_notifications(x);
}
//========================================================================================================

void setup() {
  Serial.begin(115200);
  Incubator.Init(&display);

  server.on ( "/", handleRoot );
  server.on ( "/getValues", handleValues );
  server.on ( "/setTemp", handleTemp );
  server.on ( "/setCO2", handleCO2 );
  server.on ( "/settings", handleSettings );
  server.on ( "/return", handleReturn );
  server.on ( "/resetWiFi", handleReset);
  server.on ( "/save", handleSaveSettings);
  server.begin();

  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
  sendCaptions();
}

void loop() {
  Incubator.switchState();
  updateVariables();
  server.handleClient();
  Blynk.run();
  setVariables();
  test_error();
}

//FUNCTIONS====================================================================================================================
void updateVariables(){
  t_set = Incubator.get_T_set();
  t_avg = Incubator.get_T_avg();
  t_1 = Incubator.get_T_1();
  t_2 = Incubator.get_T_2();
  t_h = Incubator.get_T_heater(); 
  co2 = Incubator.get_CO2();
  co2_avg = Incubator.get_CO2_avg();
  co2_set = Incubator.get_CO2_set();
}

void setVariables(){
  if(t_set - Incubator.get_T_set() != 0)
  {
    Incubator.set_T(t_set);
  }
  if(co2_set - Incubator.get_CO2_set() != 0)
  {
    Incubator.set_co2(co2_set);
  }
}

//WEB INTERFACE=================================================================================================================
void handleRoot(){ 
  server.send ( 200, "text/html", Incubator.getPage() );
}

void handleValues()
{
    String sen = "{\"t_avg\":\""+String(t_avg)+"\",\"t_set\":\""+String(t_set)+"\",\"t_1\":\""+String(t_1)+"\",\"t_2\":\""+String(t_2)+"\",\"t_h\":\""+String(t_h)+"\",\"co2_avg\":\""+String(co2_avg)+"\",\"co2_set\":\""+String(co2_set)+"\",\"co2_1\":\""+String(co2_avg)+"\",\"co2_2\":\""+String(co2_avg)+"\"}";
    server.send ( 200, "text/plane", sen );
}

void handleTemp() {
  String Value = server.arg("temp"); 
  if (Value.length() > 0) {
    Incubator.set_T(Value.toFloat());
  }
  server.send ( 200, "text/plane", "" );
}

void handleCO2() {
  String Value = server.arg("co2"); 
  if (Value.length() > 0) {
    Incubator.set_co2(Value.toFloat());
  }
  server.send ( 200, "text/plane", "" );
}

void handleSettings(){
  server.send ( 200, "text/html", Incubator.getSettings() );
}

void handleReturn(){
  server.send ( 200, "text/html", Incubator.getPage() );
}

void handleReset(){
  server.send ( 200, "text/html", Incubator.getReset() );
  delay(1000);
  Incubator.reset_WiFi();
}

void handleSaveSettings() {
  String Value = server.arg("name"); 
  if (Value.length() > 0) {
    Incubator.set_incubator_name(Value);
  }
  server.send ( 200, "text/html", Incubator.getPage() );
}



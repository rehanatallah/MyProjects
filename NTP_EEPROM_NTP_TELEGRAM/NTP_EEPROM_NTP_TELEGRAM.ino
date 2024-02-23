#include <WiFi.h>    
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "EEPROMFeatures.h"
#include "NTPClient.h"
#include "telegram_bot.h"
#include "ESPTelnetStream.h"

ESPTelnetStream telnet;

// WiFI_Set Up
const char* ssid = "SSID";
const char* password = "Password";
void wifi_setup(){
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

//custom lib telebot
TelegramBot telegramBot;
String telegram_url = telegramBot.getTelegramUrl();
String query = telegramBot.getQuery();

//custom lib EEPROM
EEPROMFeatures EEPROMF(0,512);

// custom lib NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"pool.ntp.org", 28800, 60000);

// variable tambahan untuk get waktu
uint8_t dayIndex;
String dayStr;  
uint8_t monthIndex;
String monthStr;  
uint8_t yearIndex;
String yearStr;
//waktu saat ini
String waktu;
String hari;
String tanggal;
String bulan;
String tahun;
// waktu last online
String waktu_LO;
String hari_LO;
String tanggal_LO;
String bulan_LO;
String tahun_LO;

String pending_LO;

char msg_LO[128];
char msg_BO[128];
char msg_LP[128];
String send_LO;
String send_BO;
String send_LP;

// time update
void du(){
  timeClient.update();
  delay(1000);

  waktu = timeClient.getFormattedTime();

  dayIndex = timeClient.getFormattedDate().lastIndexOf('-') + 1;
  dayStr = timeClient.getFormattedDate().substring(dayIndex);  
  monthIndex = timeClient.getFormattedDate().indexOf('-') + 1;
  monthStr = timeClient.getFormattedDate().substring(monthIndex, monthIndex + 2);  
  yearIndex = timeClient.getFormattedDate().indexOf('-');
  yearStr = timeClient.getFormattedDate().substring(0, yearIndex);

  hari = timeClient.getDayName(timeClient.getDay());
  tanggal = dayStr;
  bulan = timeClient.getMonthName(monthStr.toInt());
  tahun = yearStr;
}

void setup() {
  Serial.begin(115200);
  wifi_setup();
  if(EEPROMF.begin() == false){
    ESP.restart();
  }
  timeClient.begin();
  delay(1000);
  du();
  delay(1000);
  send_LO = EEPROMF.readString(0);
  send_LP = EEPROMF.readString(128);
  snprintf(msg_BO, sizeof(msg_BO), "System back online at %s on %s %s %s %s", waktu.c_str(), hari.c_str(), tanggal.c_str(), bulan.c_str(), tahun.c_str());
  send_BO = String (msg_BO);
  Serial.println(send_LP);
  Serial.println(send_LO);
  Serial.println(send_BO);
  telegramBot.sendMessageToTelegram(telegram_url, query, send_LP);    
  telegramBot.sendMessageToTelegram(telegram_url, query, send_LO);    
  telegramBot.sendMessageToTelegram(telegram_url, query, send_BO);    
  
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Zetta IoT");
  // No authentication by default
  ArduinoOTA.setPassword("ZETTAIOTADMIN");
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  digitalWrite(BUILTIN_LED, LOW);
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);

  if (WiFi.status() == WL_CONNECTED && pending_LO != ""){
    Serial.println("");
    Serial.println("reconnected");
    // Kirim pesan yang gagal dikirim
    telegramBot.sendMessageToTelegram(telegram_url, query, pending_LO);    
    Serial.println(pending_LO);
    snprintf(msg_BO, sizeof(msg_BO), "System back online at %s on %s %s %s %s", waktu.c_str(), hari.c_str(), tanggal.c_str(), bulan.c_str(), tahun.c_str());
    send_BO = String (msg_BO);
    telegramBot.sendMessageToTelegram(telegram_url, query, send_BO);    
    Serial.println(send_BO);
    pending_LO = "";
  }
  du();
  if (WiFi.status() == WL_CONNECTED){
    waktu_LO = waktu;
    hari_LO = hari;
    tanggal_LO = tanggal;
    bulan_LO = bulan;
    tahun_LO = tahun;
  }  
  snprintf(msg_LO, sizeof(msg_LO), "System last online at %s on %s %s %s %s", waktu_LO.c_str(), hari_LO.c_str(), tanggal_LO.c_str(), bulan_LO.c_str(), tahun_LO.c_str());
  snprintf(msg_LP, sizeof(msg_LP), "System last onpower at %s on %s %s %s %s", waktu.c_str(), hari.c_str(), tanggal.c_str(), bulan.c_str(), tahun.c_str());

  if (WiFi.status() != WL_CONNECTED){
    pending_LO = String (msg_LO);
    Serial.println("reconnecting");
  }
  EEPROMF.writeString(0, msg_LO);
  EEPROMF.writeString(128, msg_LP);
  delay(5000);
}
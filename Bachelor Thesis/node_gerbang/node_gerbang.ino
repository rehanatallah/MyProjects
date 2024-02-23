#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SPI.h>

//deklarasi wifi & MQTT
const char* ssid = "Al-Debaran";
const char* password = "........";
const char* mqtt_server = "broker.mqtt-dashboard.com";
WiFiClient espClient;
PubSubClient client (espClient);
char payloadValue[20];  // Variabel untuk menyimpan nilai payload

//Deklarasi variabel & pin RFID
#define RST_PIN D0 // Configurable, see typical pin layout above
#define SS_PIN D8 // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
byte buffer[18];
byte size = sizeof(buffer);
byte blockAddr;
String valueRFID = "";
String buffer1 = "";

//servo
Servo portal;

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Memeriksa panjang pesan
  if (length > sizeof(payloadValue) - 1) {
    length = sizeof(payloadValue) - 1;  // Batasi panjang pesan
    Serial.println("Pesan terlalu panjang, dipotong sesuai kapasitas");
  }

  //Hanya menimpan pesan berupa "aman" atau "TERJADI PENCURIAN"
  if (strncmp((char*)payload, "aman", 4) == 0 || strncmp((char*)payload, "TERJADI PENCURIAN", 17) == 0) {
    // Memindahkan nilai payload ke variabel payloadValue
    strncpy(payloadValue, (char*)payload, length);
    payloadValue[length] = '\0';  // Menambahkan null-terminating karakter
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("parkir/gerbang/connect", "gerbang connected");
      // ... and resubscribe
      client.subscribe("parkir/gerbang/pencurian");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
    }
  }
}

void RF() {
  String UID = "";
  //autentikasi RFID
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent())return;
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())return;

  //Show some details of the PICC (that is: the tag/card)
  //Check for compatibility
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  if ( piccType != MFRC522::PICC_TYPE_MIFARE_MINI
       && piccType != MFRC522::PICC_TYPE_MIFARE_1K
       && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Hanya bisa digunakan dengan kartu MIFARE Classic."));
    return;
  }
  digitalWrite(BUILTIN_LED, LOW);

  //Pemanggilan fungsi baca UID RFID dan mendapatkan nilai.
  int val1 = (mfrc522.uid.uidByte[0]);
  int val2 = (mfrc522.uid.uidByte[1]);
  int val3 = (mfrc522.uid.uidByte[2]);
  int val4 = (mfrc522.uid.uidByte[3]);
  String valA = String(val1, HEX);
  String valB = String(val2, HEX);
  String valC = String(val3, HEX);
  String valD = String(val4, HEX);
  UID = valA + valB + valC + valD;
  UID.toUpperCase();
  Serial.print(F("UID:"));
  Serial.println(UID);
  Serial.println();
  buffer1 = UID;

  //Menghentikan proses pembacaan setelah selesai di scan
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(500);
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

//fungsi membuka portal
void bukaportal() {
  int pos = 180;
  portal.write(pos);
  delay(5000);
}

//fungsi menutup portal
void tutupportal() {
  int pos = 90;
  portal.write(pos);
  delay(500);
}

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  SPI.begin();
  portal.attach(D1);
  mfrc522.PCD_Init(); //Init MFRC522 card
}

void loop() {
  tutupportal();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  client.publish("parkir/gerbang/connect", "gerbang connected");
  Serial.println(payloadValue);

  delay(1000);
  RF();

  if (strncmp(payloadValue, "aman", 4) == 0) {
    if (buffer1 != "") {
      if (strstr(buffer1.c_str(), "93CB721A")) {
        Serial.println("Kartu dikenali, Silahkan masuk");
        delay(1000);
        bukaportal();
        tutupportal();
      }
      else if (strstr(buffer1.c_str(), "A3E7681A")) {
        Serial.println("Kartu dikenali, Silahkan masuk");
        delay(1000);
        bukaportal();
        tutupportal();
      }
      else {
        Serial.println("Maaf kartu tidak dikenali, mohon daftar dahulu ke admin.");
        digitalWrite(BUILTIN_LED, LOW);
        delay(500);
        digitalWrite(BUILTIN_LED, HIGH);
        delay(500);
        digitalWrite(BUILTIN_LED, LOW);
        delay(500);
        digitalWrite(BUILTIN_LED, HIGH);
      }
      buffer1 = "";
    }
    return;
  }

  if (strncmp(payloadValue, "TERJADI PENCURIAN", 17) == 0) {
    tutupportal();
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH);
    //    delay(100);
    Serial.println("TERJADI PENCURIAN, LOCKDOWN");
    return;
  }

  //  if (strncmp(payloadValue, "diatasi", 7) == 0) {
  //    Serial.println("Pencurian telah diatasi.");
  //    Serial.println("Terimakasih telah menunggu!");
  //    client.publish("parkir/gerbang/status", "aman", true);
  //    return;
  //  }

  //  if (payloadValue[0] == '\0') {
  //    if (buffer1 != "") {
  //      if (strstr(buffer1.c_str(), "93CB721A")) {
  //        Serial.println("Kartu dikenali, Silahkan masuk");
  //        delay(1000);
  //        bukaportal();
  //        tutupportal();
  //      }
  //      else if (strstr(buffer1.c_str(), "A3E7681A")) {
  //        Serial.println("Kartu dikenali, Silahkan masuk");
  //        delay(1000);
  //        bukaportal();
  //        tutupportal();
  //      }
  //      else {
  //        Serial.println("Maaf kartu tidak dikenali, mohon daftar dahulu ke admin.");
  //      }
  //      buffer1 = "";
  //    }
  //    return;
  //  }
  else {
    return;
  }
}

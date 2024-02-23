#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HCSR04.h>
#include <PubSubClient.h>

//WiFi dan MQTT
const char* ssid = "Al-Debaran";
const char* password = "........";
const char* mqtt_server = "broker.mqtt-dashboard.com";
WiFiClient espClient;
PubSubClient client(espClient);

char payloadValue[64];  // Variabel untuk menyimpan nilai payload
char msgA[128];
char msgU[128];
char pendingMessage[128];
String pendingMessage1;
String pendingMessage2;

//Deklarasi variabel & pin RFID
#define RST_PIN D3 // Configurable, see typical pin layout above
#define SS_PIN D4 // Configurable, see typical pin layout above
HCSR04 hc(D2, D1); //initialisation class HCSR04 (trig pin , echo pin)
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

byte buffer[18] = {0};
byte size = sizeof(buffer);
byte blockAddr;
String valueRFID = "";
String buffer1 = "";
uint8_t n = 0;

String nilA;
String nilB;
String nilC;
String uID;

String curiA;
String curiB;
String curiC;
String curiUID;

float units;
String jarak;

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

  // Memindahkan nilai payload ke variabel payloadValue
  strncpy(payloadValue, (char*)payload, length);
  payloadValue[length] = '\0';  // Menambahkan null-terminating karakter
}

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("parkir/slot1/connect", "slot 1 connected");
      // ... and resubscribe
      client.subscribe("parkir/slot1/konfirmasi");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
    }
  }
}

//Fungsi untuk melakukan autentikasi menggunakan kunci B yang digunakan untuk Read
void kunci(byte trailerBlock) {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  Serial.println(F("Authenticating using key A..."));
  //Untuk cek status autentikasi pada block yang dipilih
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    if (status == 4) {
      size = sizeof(buffer);
    }
    return;
  }
}

//Fungsi untuk melakukan pembacaan dan menampilkan data pada block yang dipilih dari RFID Card
void baca(byte blockAddr) {
  //Untuk cek status pembacaan pada block yang dipilih
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data pada blok "));
  Serial.print(blockAddr);
  Serial.println(F(" : "));
  //Untuk membaca dan menyimpan data 1 block pada variabel buffer
  mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  //Untuk menampilkan data pada variabel buffer
  dump_byte_array(buffer, 16);
  Serial.println();
  //Untuk looping data pada 1 block dan disimpan pada variabel valueRFID
  for (uint8_t i = 0; i < 16; i++) {
    valueRFID += (char)buffer[i];
  }
  valueRFID.trim();
  Serial.print(valueRFID);
  Serial.println();
}

//Fungsi untuk menampilkan data pada variabel buffer
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void RF() {
  //autentikasi RFID
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent())return;
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())return;
  digitalWrite(BUILTIN_LED, LOW);
  delay(300);

  //Show some details of the PICC (that is: the tag/card)
  //Check for compatibility
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  if ( piccType != MFRC522::PICC_TYPE_MIFARE_MINI
       && piccType != MFRC522::PICC_TYPE_MIFARE_1K
       && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Hanya bisa digunakan dengan kartu MIFARE Classic."));
    digitalWrite(BUILTIN_LED, HIGH);
    return;
  }

  //Pemanggilan fungsi baca UID RFID dan mendapatkan nilai.
  int val1 = (mfrc522.uid.uidByte[0]);
  int val2 = (mfrc522.uid.uidByte[1]);
  int val3 = (mfrc522.uid.uidByte[2]);
  int val4 = (mfrc522.uid.uidByte[3]);
  String valA = String(val1, HEX);
  String valB = String(val2, HEX);
  String valC = String(val3, HEX);
  String valD = String(val4, HEX);
  uID = valA + valB + valC + valD;
  uID.toUpperCase();
  Serial.print(F("UID:"));
  Serial.println( uID);
  if ( uID == buffer1) {
    buffer1 = "";
    nilA = "";
    nilB = "";
    nilC = "";
  }
  else if (buffer1 == "") {
    if ( uID != buffer1) {
      buffer1 =  uID;
      n = 0;
    }
  }
  else if (buffer1 != "") {
    if ( uID != buffer1) {
      Serial.println("kartu tidak dikenali");
      digitalWrite(BUILTIN_LED, HIGH);
      return;
    }
  }

  // Serial.println(buffer1);
  //Pemanggilan fungsi baca RFID pada block 4,5,6 dan mendapatkan nilai
  //Tiap penggabungan pada masing-masing variabel nil[x] memiliki jatah 1 blok address dari valueRFID
  kunci(7);
  delay(1000);
  baca(4);
  delay(1000);
  nilA = String(valueRFID[0]) + String(valueRFID[1]) +
         String(valueRFID[2]) + String(valueRFID[3]) +
         String(valueRFID[4]) + String(valueRFID[5]) +
         String(valueRFID[6]) + String(valueRFID[7]) +
         String(valueRFID[8]) + String(valueRFID[9]) +
         String(valueRFID[10]) + String(valueRFID[11]) +
         String(valueRFID[12]) + String(valueRFID[13]) +
         String(valueRFID[14]) + String(valueRFID[15]);
  Serial.println();
  baca(5);
  nilB = String(valueRFID[16]) + String(valueRFID[17]) +
         String(valueRFID[18]) + String(valueRFID[19]) +
         String(valueRFID[20]) + String(valueRFID[21]) +
         String(valueRFID[22]) + String(valueRFID[23]) +
         String(valueRFID[24]) + String(valueRFID[25]) +
         String(valueRFID[26]) + String(valueRFID[27]);
  Serial.println();
  baca(6);
  nilC =  String(valueRFID[32]) + String(valueRFID[33]) +
          String(valueRFID[34]) + String(valueRFID[35]) +
          String(valueRFID[36]) + String(valueRFID[37]) +
          String(valueRFID[38]) + String(valueRFID[39]) +
          String(valueRFID[40]) + String(valueRFID[41]) +
          String(valueRFID[42]) + String(valueRFID[43]);
  //  Serial.println(nilA);
  //  Serial.println(nilB);
  //  Serial.println(nilC);

  //Menghentikan proses pembacaan setelah selesai di scan
  digitalWrite(BUILTIN_LED, LOW);
  delay(3000);
  digitalWrite(BUILTIN_LED, HIGH);
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  valueRFID = "";
}

//Fungsi untuk melakukan pembacaan sensor
void sense() {
  // Serial.print("Reading: ");
  units = hc.dist();
  Serial.print(units);
  Serial.println(" cm");
  jarak = String(units);
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //Deklarasi objek dari library
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
}

void loop() {
  //connect mqtt
  if (!client.connected()) {
    reconnect();
  }
  if (client.connected() && pendingMessage1 != ""){
    // Kirim pesan yang gagal dikirim
    client.publish("parkir/slot1/pencurian", pendingMessage1.c_str());
    pendingMessage1 = "";
  }
  if (client.connected() && pendingMessage2 != ""){
    // Kirim pesan yang gagal dikirim
    client.publish("parkir/slot1/pencurian", pendingMessage2.c_str());
    pendingMessage2 = "";
  }

  // Pemanggilan fungsi RFID
  delay(1000);
  RF();

   //Pemanggilan fungsi baca sensor
  sense();

  client.loop();
  Serial.println(payloadValue);
  client.publish("parkir/slot1/connect", "slot 1 connected");
  client.publish("parkir/slot1/sensor", jarak.c_str());

  if (strncmp(payloadValue, "normal", 6) == 0) {
    if (buffer1 != "" && units <= 30 && n == 0) {
      return;
    }
    if (buffer1 != "" && units >= 30 && n == 0)  {
      Serial.println("terjadi pencurian");
      //mengirim pesan melalui mqtt
      curiA = String (nilA);
      curiB = String (nilB);
      curiC = String (nilC);
      curiUID = String (uID);
      snprintf(msgU, sizeof(msgU), "Apa Mobil milik %s dengan nompol %s keluar?", curiA.c_str(), curiC.c_str());
      snprintf(msgA, sizeof(msgA), "TERJADI PENCURIAN UID: %s Milik: %s Nompol: %s", curiUID.c_str(), curiA.c_str(), curiC.c_str());
      if (client.connected()) {
        client.publish("parkir/slot1/konfirmasi", msgU, true);
        delay(2000);
      }
      if (!client.connected()) {
        snprintf(pendingMessage, sizeof(pendingMessage), "Kejadian saat disconnect UID: %s Milik: %s Nompol: %s", curiUID.c_str(), curiA.c_str(), curiC.c_str());
        if (pendingMessage1 == ""){
          pendingMessage1 = String (pendingMessage);
        }
        else if (pendingMessage2 == ""){
          pendingMessage2 = String (pendingMessage);
        }
      }
      n = 1;
      buffer1 = "";
      nilA = "";
      nilB = "";
      nilC = "";
      uID = "";
    }
    return;
  }

  if (!client.connected()) {
    return;
  }
  if (strncmp(payloadValue, "ya", 2) == 0) {
    client.publish("parkir/slot1/konfirmasi", "Terimakasih Konfirmasinya!");
    memset(msgA, 0, sizeof(msgA));
    memset(msgU, 0, sizeof(msgU));
    return;
  }
  if (strncmp(payloadValue, "bukan", 5) == 0) {
    Serial.print("Publish message: ");
    Serial.println(msgA);
    client.publish("parkir/gerbang/pencurian", "TERJADI PENCURIAN", true);
    client.publish("parkir/slot1/pencurian", msgA, true);
    client.publish("parkir/slot1/konfirmasi", "Terimakasih Konfirmasinya!");
    client.loop();
    memset(msgA, 0, sizeof(msgA));
    memset(msgU, 0, sizeof(msgU));
    return;
  }
  if (strncmp(payloadValue, "Apa", 3) == 0) {
    delay(10000);
    client.loop();
    if (strncmp(payloadValue, "Apa", 3) == 0) {
      client.publish("parkir/gerbang/pencurian", "TERJADI PENCURIAN", true);
      client.publish("parkir/slot1/pencurian", msgA, true);
      client.publish("parkir/slot1/konfirmasi", "normal", true);
      client.loop();
      memset(msgA, 0, sizeof(msgA));
      memset(msgU, 0, sizeof(msgU));
      return;
    }
    else {
      return;
    }
  }
  else {
    client.publish("parkir/slot1/konfirmasi", "normal", true);
    return;
  }
}

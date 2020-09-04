// Device not found, scanning again
#include <Arduino.h>
#include <Update.h>

#define SerialMon Serial
#define SerialAT Serial1

#define DEBUG_PRINT(...) { SerialMon.print(millis()); SerialMon.print(" - "); SerialMon.println(__VA_ARGS__); }
#define DEBUG_FATAL(...) { SerialMon.print(millis()); SerialMon.print(" - FATAL: "); SerialMon.println(__VA_ARGS__); delay(1000); ESP.restart(); }

#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb
#include "TinyGsmClient.h"
TinyGsm modem(SerialAT);

void printDeviceInfo(){
  Serial.println();
  Serial.println("--------------------------");
  Serial.println(String("Build:    ") +  __DATE__ " " __TIME__);
  Serial.println(String("Flash:    ") + ESP.getFlashChipSize() / 1024 + "K");
  Serial.println(String("ESP sdk:  ") + ESP.getSdkVersion());
  Serial.println(String("Chip rev: ") + ESP.getChipRevision());
  Serial.println(String("Free mem: ") + ESP.getFreeHeap());
  Serial.println("--------------------------");
}

bool atCheck(int timeout) {
  SerialAT.println("AT");
  SerialAT.setTimeout(timeout);
  String respondSerialAT = SerialAT.readString();
  Serial.println("respondSerialAT :\t" + respondSerialAT);
  if (respondSerialAT.length() <= 0) {
    return false;
  }
  return respondSerialAT.indexOf("OK") >= 0;
}

uint32_t scanBaudRate() {
  static uint32_t rates[] = {115200, 9600, 4800, 57600};

  for (unsigned i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
    delay(500);
    uint32_t rate = rates[i];
    Serial.print(String("\r "));
    Serial.println("[scanBaudRate] make sure in " + String(rate));

    static const int RXPin = 27, TXPin = 26;
    SerialAT.begin(rate, SERIAL_8N1, TXPin, RXPin);

    unsigned int counterAT = 0;
    for (unsigned int i = 0; i < 15; i++) {
      if (atCheck(500)) {
        Serial.println("[scanBaudRate] >> counterAT " + String(counterAT));
        counterAT++;
      }
    }

    if (counterAT > 5) {
      return rate;
    }
  }
}

void startOtaUpdate(
  String protocol, 
  String host, 
  String url, 
  int port
  ){
  /*
  host : mamunsyuhada.pogungengineering.ai
  url : /file/firmware/2020-08-25_10I27I28.bin
  port : 5443
  */ 

  Serial.println("protocol : " + protocol);
  Serial.println("host : " + host);
  Serial.println("url : " + url);
  Serial.println("port : " + String(port));

  DEBUG_PRINT(String("Connecting to ") + host + ":" + port);

  Client* client = NULL;
  if (protocol == "http") {
    client = new TinyGsmClient(modem);
    if (!client->connect(host.c_str(), port)) {
      DEBUG_FATAL(F("Client not connected"));
    }
  }
  else if (protocol == "https") {
    client = new TinyGsmClientSecure(modem);
    if (!client->connect(host.c_str(), port)) {
      DEBUG_FATAL(F("Client not connected"));
    }
  }
  else {
    DEBUG_FATAL(String("Unsupported protocol: ") + protocol);
  }

  DEBUG_PRINT(String("Requesting ") + url);

  client->print(String("GET ") + url + " HTTP/1.0\r\n"
               + "Host: " + host + "\r\n"
               + "Connection: keep-alive\r\n"
               + "\r\n");

  long timeout = millis();
  while (client->connected() && !client->available()) {
    if (millis() - timeout > 10000L) {
      DEBUG_FATAL("Response timeout");
    }
  }

  // Collect headers
  String md5;
  int contentLength = 0;

  while (client->available()) {
    String line = client->readStringUntil('\n');
    line.trim();
    //SerialMon.println(line);    // Uncomment this to show response headers
    line.toLowerCase();
    if (line.startsWith("content-length:")) {
      contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
    } else if (line.startsWith("x-md5:")) {
      md5 = line.substring(line.lastIndexOf(':') + 1);
    } else if (line.length() == 0) {
      break;
    }
  }
  Serial.println("contentLength : " + String(contentLength));

  if (contentLength <= 0) {
    DEBUG_FATAL("Content-Length not defined");
  }

  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    Update.printError(SerialMon);
    DEBUG_FATAL("OTA begin failed");
  }

  Serial.println("md5:" + md5);

  if (md5.length()) {
    DEBUG_PRINT(String("Expected MD5: ") + md5);
    if(!Update.setMD5(md5.c_str())) {
      DEBUG_FATAL("Cannot set MD5");
    }
  }

  DEBUG_PRINT("Flashing...");

  // The next loop does approx. the same thing as Update.writeStream(http) or Update.write(http)

  int written = 0;
  int progress = 0;
  uint8_t buff[256];

  while (client->connected() && written < contentLength) {
    timeout = millis();
    while (client->connected() && !client->available()) {
      delay(1);
      if (millis() - timeout > 100000L) {
        DEBUG_FATAL("Timeout");
      }
    }

    int len = client->read(buff, sizeof(buff));
    if (len <= 0) continue;

    Update.write(buff, len);
    written += len;

    int newProgress = (written*100)/contentLength;
    if (newProgress - progress >= 5 || newProgress == 100) {
      progress = newProgress;
      SerialMon.print(String("\r ") + progress + "%");
    }
  }
  SerialMon.println();

  if (written != contentLength) {
    Update.printError(SerialMon);
    DEBUG_FATAL(String("Write failed. Written ") + written + " / " + contentLength + " bytes");
  }

  if (!Update.end()) {
    Update.printError(SerialMon);
    DEBUG_FATAL(F("Update not ended"));
  }

  if (!Update.isFinished()) {
    DEBUG_FATAL(F("Update not finished"));
  }

  DEBUG_PRINT("========= Update successfully completed. Rebooting. =========");
  delay(5000);
  ESP.restart();
}

String cmd_at(String atcommand, uint8_t time_out){
  SerialAT.setTimeout(time_out);
  atcommand.toUpperCase();
  atcommand = atcommand + "\r\n";
  SerialAT.print(atcommand);
  String respond = SerialAT.readString();
  respond.replace("\n\r", "");
  return respond;
}

String eraseSpaceAre(String res){
  res.replace("\n","");
  res.replace("\r","");
  
  return res;
}

String getccid(){
  String respond = cmd_at("AT+CCID", 100);
  if(respond.indexOf("OK") >= 0){
    respond = respond.substring(respond.indexOf("\n"), respond.lastIndexOf("OK"));
    respond = eraseSpaceAre(respond);
    
    return respond;
  }
  return "";
}

void trigger_sim(){
  Serial.println("trigger_sim");
  pinMode(22, OUTPUT);

  digitalWrite(22, LOW);
  delay(1000);
  digitalWrite(22, HIGH);
  delay(1000);
  digitalWrite(22, LOW);
}

void setup() {
  delay(2000);

  SerialMon.begin(115200);
  delay(10);
  printDeviceInfo();

  trigger_sim();

  SerialMon.println("  Firmware A is running--Firmware 1");
  SerialMon.println("--------------------------");

  SerialMon.println("  Scan Baud Rate  ");
  SerialMon.println("----" + String(scanBaudRate()) + "----");

  DEBUG_PRINT(F("Starting OTA update in 5 seconds..."));
  delay(5000);


  DEBUG_PRINT(F("Checking Network..."));
  while (true){
    if(modem.isNetworkConnected()){
      break;
    }
    else{
      DEBUG_PRINT(F("Waiting for network..."));
      bool network = modem.waitForNetwork(5000L);
      Serial.println("Network : " + String(network));
      Serial.println("getccid : " + getccid());
      if (network == true) {
        break;
      }
      DEBUG_PRINT(F("Network failed to connect"));
    }
  }
  

  DEBUG_PRINT(F("Get CCID..."));
  Serial.println(getccid());

  DEBUG_PRINT(F("Get Operator..."));
  Serial.println(modem.getOperator());

  DEBUG_PRINT(F("Connecting to GPRS"));
  unsigned int i = 0;
  while(true){
    if (modem.gprsConnect("internet", "", "")==true) {
      DEBUG_PRINT(F("Connected to GPRS"));  
      // delay(1000);
      break;
    }
    Serial.print(String("\r "));
    DEBUG_PRINT(F("Not connected APN"));

    if(i>10){
      DEBUG_FATAL(F("counted until 10 times"));
    }
    
    i++;
  }
  i = 0;

  /*
  host : mamunsyuhada.pogungengineering.ai
  url : /file/firmware/2020-08-25_10I27I28.bin
  port : 5443
  */
  startOtaUpdate(
    "http",
    "mamunsyuhada.pogungengineering.ai",
    "/file/firmware/2020-08-25_10I27I28.bin",
    5443
  );
}

void loop() {
  delay(1000);
  DEBUG_PRINT("---Loop---");
}
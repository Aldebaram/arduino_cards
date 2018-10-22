




/*
  Many thanks to nikxha from the ESP8266 forum
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>




/* wiring the MFRC522 to ESP8266 (ESP-12)
  Signal       MFRC522    WeMos D1 mini        NodeMcu        Generic
  RST/Reset   RST             D3 [1]            D3 [1]       GPIO-0 [1]
  SPI SS      SDA[3]          D8 [2]            D8 [2]       GPIO-15 [2]
  SPI MOSI    MOSI            D7                D7           GPIO-13
  SPI MISO    MISO            D6                D6           GPIO-12
  SPI SCK     SCK             D5                D5           GPIO-14
*/

#define RST_PIN  5  // RST-PIN für RC522 - RFID - SPI - Modul GPIO5 
#define SS_PIN  SS  // SDA-PIN für RC522 - RFID - SPI - Modul GPIO4 

// CONFIGURATIONS
const char *ssid =  "Arduino";     // change according to your Network - cannot be longer than 32 characters!
const char *pass =  "farofa123"; // change according to your Network
String jsonstart = "{\"cards\":[";
String jsonend = "]}";
char  hex_num[8];
char  teste[8];
String  CARD_id;
String  NewCARD_id;
String  cardJson = "";
String primeiro = "";
String segundo = "";
ESP8266WebServer server(8081);



MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance


void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void setup() {
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  Serial.begin(115200);    // Initialize serial communications
  delay(250);
  Serial.println(F("Booting...."));
   yield();
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522

  WiFi.begin(ssid, pass);

  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries < 10)) {
    retries++;
    delay(500);
     yield();
    //Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected"));
    // Print the IP address
    Serial.print("Use this URL : ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }

    Serial.println(F("Ready!"));
    Serial.println(F("======================================================"));
    Serial.println(F("Scan for Card and print UID:"));


  }


  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");


  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;    //Declare object of class HTTPClient

    http.begin("http://192.168.43.253:8080/json/arduino/updatearduino/1");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    String ip = WiFi.localIP().toString();
    int httpCode = http.PUT("{\"name\":\"Cadastro\",\"ip\":\"" + ip + ":8081\"}");   //Send the request
    String payload = http.getString();                  //Get the response payload
    Serial.println("POST do ip do arduino");
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload

    http.end();  //Close connection

  } else {

    Serial.println("Error in WiFi connection");

  }


}

void loop() {
  server.handleClient();
  digitalWrite(D4, LOW);
  digitalWrite(D6, LOW);
  String classe = getClassId();
  int classeNivel = getClassNivel();
  Serial.println(classeNivel);



  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Helper routine to dump a byte array as hex values to Serial
  for (int x = 0; x < 8; x++) {
    hex_num[x] = NULL;
  }
  CARD_id = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    hex_num[i] = (mfrc522.uid.uidByte[i]);
    CARD_id += (mfrc522.uid.uidByte[i]);
    // //Serial.print(hex_num[i],HEX);
  }
    Serial.println(CARD_id);
    String cartao = getPessoaId(CARD_id);   // pega o cartão
    Serial.println(cartao);
    if(cartao != "null"){ //// Verifica cartão
      int pessoaNivel = getPessoaNivel(CARD_id);// pega nivel de acesso do cartão
      Serial.println(pessoaNivel);
      if(pessoaNivel>=classeNivel){// valida nivel de acesso do cartão
      
  if (cardJson.indexOf(CARD_id) == -1) {
    Serial.println();
    //Serial.print(CARD_id);
    Serial.println();
    digitalWrite(D4, HIGH);
    
    if (segundo == "") {
      segundo += "\"" + CARD_id + "\"";
    } else {
      primeiro += "\"" + CARD_id + "\",";
    }
    cardJson = primeiro + segundo;
    //Serial.print(cardJson);
    registroEntrada(cartao,classe);
    abreporta();
  } else {
    registroSaida(cartao,classe);
    abreporta();
    cardJson.replace(("\"" + CARD_id + "\","), "");
    cardJson.replace((",\"" + CARD_id + "\""), "");
    cardJson.replace(("\"" + CARD_id + "\""), "");
    primeiro.replace(("\"" + CARD_id + "\","), "");
    segundo.replace(("\"" + CARD_id + "\""), "");
    digitalWrite(D6, HIGH);
    //Serial.print(cardJson);
  }
  }else{///quando o cartão não tem o nivel de acesso
    Serial.println("acesso nao autorizado");
    }
  }else{
    Serial.println("cartao nao identificado");
    //quando cartao for igual a null
    }

    
  server.on("/class", [cardJson]() {
    server.send(200, "application/json", jsonstart + cardJson + jsonend );
  });
  server.on("/card", [cardJson]() {
    server.send(200, "application/json", "{\"card\":" + CARD_id + "}" );
  });
  delay(3000);
}



String getClassId() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    String ip = WiFi.localIP().toString();
    http.begin("http://192.168.43.253:8080/json/arduino/getsala/" + ip + ":" + "8081");      //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      String payload = http.getString();   //Get the request response payload                    //Print the response payload
      payload.replace("{\"salaId\":", "");
      payload.replace("}", "");
      return (payload);
    }
    http.end();   //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }
}

int getClassNivel() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    String ip = WiFi.localIP().toString();
    http.begin("http://192.168.43.253:8080/json/arduino/salanivel/" + ip + ":" + "8081");      //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      String payload = http.getString();   //Get the request response payload                    //Print the response payload
      payload.replace("{\"nivel\":", "");
      payload.replace("}", "");
      return (payload.toInt());
    }
    http.end();   //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }
}

 String getPessoaId(String cartao) {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://192.168.43.253:8080/json/arduino/getpessoa/" + cartao);      //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      Serial.println("GET cartao");
      String payload = http.getString();   //Get the request response payload                    //Print the response payload
      payload.replace("{\"id\":", "");
      payload.replace("}", "");
      return (payload);
    }
    http.end();   //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }
 }
 int getPessoaNivel(String cartao) {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://192.168.43.253:8080/json/arduino/findcard/" + cartao);      //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      Serial.println("GET cartao");
      String payload = http.getString();   //Get the request response payload                    //Print the response payload
      payload.replace("{\"acesso\":", "");
      payload.replace("}", "");
      return (payload.toInt());
    }
    http.end();   //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }

 }
void abreporta(){
  Serial.println("abriu");
  }

void registroEntrada(String pessoaid, String salaid){
  String insert = getUTC();
  
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;    //Declare object of class HTTPClient

    http.begin("http://192.168.43.253:8080/json/arduino/insereregistro");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    String ip = WiFi.localIP().toString();
    int httpCode = http.POST("{\"sala\":"+ salaid + ",\"pessoa\":"+ pessoaid +",\"datahora\":\""+insert+"\",\"entrada\":true,\"alert\":false}");   //Send the request
    String payload = http.getString();                  //Get the response payload
    Serial.println("POST");
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
    http.end();  //Close connection
    Serial.println("registrou entrada");
  } else {

    Serial.println("Error in WiFi connection");

  }
  
  }

void registroSaida(String pessoaid, String salaid){
   String insert = getUTC();

   if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;    //Declare object of class HTTPClient

    http.begin("http://192.168.43.253:8080/json/arduino/insereregistro");      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    String ip = WiFi.localIP().toString();
    int httpCode = http.POST("{\"sala\":"+ salaid + ",\"pessoa\":"+ pessoaid +",\"datahora\":\""+insert+"\",\"entrada\":false,\"alert\":false}");   //Send the request
    String payload = http.getString();                  //Get the response payload
    Serial.println("POST");
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
    http.end();  //Close connection
    Serial.println("registrou saida");
  } else {

    Serial.println("Error in WiFi connection");

  }

  }




  String getUTC() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://worldclockapi.com/api/json/utc/now");      //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      Serial.println("GET cartao");
      String payload = http.getString();   //Get the request response payload                    //Print the response payload
      payload = payload.substring(30, 47);
      Serial.println(payload);
      return (payload);
    }
    http.end();   //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }

 }


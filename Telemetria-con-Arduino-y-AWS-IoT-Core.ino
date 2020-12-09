#include <ArduinoBearSSL.h>//Puerto BearSSL a Arduino
#include <ArduinoECCX08.h>//Biblioteca para los chips criptográficos Atmel/Microchip
#include <ArduinoMqttClient.h>//Permite enviar y recibir mensajes MQTT
#include <WiFi101.h>//Permite la conexión de la placa a internet con autenticación criptográfica
#include "secrets.h"//Claves de conexión
#include <Wire.h>//Biblioteca para comunicación I2C
#include <LiquidCrystal_I2C.h>//Biblioteca para la LCD con protocolo I2C
#include <ArduinoJson.h>//Biblioteca para convertir mensajes en objetos json
#include "DHT.h" //Extrae la información del sensor de temperatura


//Claves de conexión
const char ssidT[] = SECRET_SSID;
const char passT[] = SECRET_PASSWORD;
const char brokerT[] = SECRET_BROKER;
const char* certificateT = SECRET_CERTIFICATE;

//Variables de los pines
const uint8_t pinDHT22T = 2, typeT = DHT22;

//Variables globales
const uint16_t intervalT = 10000, delayConectionT = 1000;
unsigned long previousMillisT;

//Enumeración para asignación de estados de solicitud
enum statusRequest {
  badRequestT = 0,
  goodRequestT = 1,
  emptyDataT = 2
} _statusR;


//Constructores
WiFiClient wifiClient;//Utilizado para la conexión de socket TCP
BearSSLClient sslClient(wifiClient);//Utilizado para la conexión SSL/TLS, se integra con ECC508
MqttClient mqttClient(sslClient);//Permite conectarse a un agente MQTT y publicar cadenas en un tema
LiquidCrystal_I2C lcdT(0x27, 16, 2);
DHT dhtT(pinDHT22T, typeT);


void setup() {
  Serial.begin(115200);
  lcdT.begin();
  dhtT.begin();

  //Verifica si existe una firma CSR
  if (!ECCX08.begin()) {
    printLCDStatus("ECCX08", "", badRequestT, emptyDataT);
    while (1);
  }

  //Establece una devolución de llamada para obtener la hora actual
  //utilizado para validar el certificado de los servidores
  ArduinoBearSSL.onGetTime(getTime);

  //Configuración de la ranura 0 ECCX08 para usarla con la clave privada
  //y el certificado público que la acompaña
  sslClient.setEccSlot(0, certificateT);

  //Establece una devolución de llamada del mensaje, esta función se
  //llama cuando el MQTT Client recibe un mensaje
  mqttClient.onMessage(onMessageReceived);

  connectWiFi();

  printLCDStatus("Iniciando...", "by evq", emptyDataT, emptyDataT);
  previousMillisT = millis();
}


void loop() {
  unsigned long currentMillisT = millis();

  if (WiFi.status() != WL_CONNECTED)
    connectWiFi();

  //Si el cliente MQTT se desconecta, se intentará establecer de nuevo la conexión
  if (!mqttClient.connected())
    connectMQTT();

  //Sondea los nuevos mensajes MQTT, verifica y mantiene un enlace de comunicación
  mqttClient.poll();


  //La siguiente función nos permite tener una tarea ejecutándose al mismo tiempo que las
  //anteriores, con la diferencia que es por un intervalo definido, esto nos evita detener
  //la parar el procesamiento de la placa. Es otros términos, es la función avanzada de delay()
  //Cada intervalo dura 10 segundos, para aumentar o disminuir el tiempo de envió, modifica
  //el valor de intervalT
  if ((unsigned long)(currentMillisT - previousMillisT) >= intervalT) {
    float temperatureT = dhtT.readTemperature(), humidityT = dhtT.readHumidity();

    //Verifica si los datos obtenidos del sensor son números
    if (isnan(temperatureT) || isnan(humidityT)) {
      Serial.println(F("No es posible obtener datos del sensor DHT"));
      return;
    }

    printLCDStatus(String(temperatureT), String(humidityT), emptyDataT, emptyDataT);
    sendDataToAWS(temperatureT, humidityT);

    previousMillisT = millis();
  }
}


//Obtiene la hora actual del módulo WiFi
unsigned long getTime() {
  return WiFi.getTime();
}


//Establece una conexión Wifi
void connectWiFi() {
  const char *messagesWIFIT[] = {"Conectando a", "Conectado", "Wifi"};

  //Estado del WIFI
  printLCDStatus(messagesWIFIT[0], ssidT, emptyDataT, emptyDataT);

  while (WiFi.begin(ssidT, passT) != WL_CONNECTED) {
    delay(delayConectionT);
  }

  printLCDStatus(messagesWIFIT[1], messagesWIFIT[2], emptyDataT, goodRequestT);
}


//Establece una conexión MQTT
void connectMQTT() {
  const char *messagesMQTTT[] = {"Estableciendo", "Conexion", "MQTT"};

  printLCDStatus(messagesMQTTT[0], messagesMQTTT[2], emptyDataT, emptyDataT);

  while (!mqttClient.connect(brokerT, 8883)) {
    delay(delayConectionT);
  }
  printLCDStatus(messagesMQTTT[1], messagesMQTTT[2], emptyDataT, goodRequestT);

  //Tema de suscripción MQTT para la recepción de mensajes
  mqttClient.subscribe("inTopic");
}


//Envía un objecto json al IoT Hub
void sendDataToAWS(float temperatureT, float humidityT) {
  //Tamaño del buffer que tiene como destino ser el objeto enviado por MQTT
  char jsonBufferT[120];

  //Capacidad del conjunto de memoria en bytes.
  StaticJsonDocument <120> docT;

  //Atributos del objecto
  docT["Temp"] = temperatureT;
  docT["Hum"] = humidityT;
  docT["Dis"] = "MKR1000";
  docT["id"] = getTime();
  serializeJson(docT, jsonBufferT);

  //Tema de suscripción MQTT para el envío de mensajes
  mqttClient.beginMessage("outTopic");
  mqttClient.print(jsonBufferT);

  printLCDStatus("Mensaje publicado", "", goodRequestT, emptyDataT);
  Serial.println(jsonBufferT);
  mqttClient.endMessage();
}


//La función onMessageReceived() imprime el tema y la carga útil
//de cualquier mensaje de un tema suscrito
void onMessageReceived(int messageSizeT) {
  //Capacidad del conjunto de memoria en bytes.
  StaticJsonDocument<60> docT;
  String payloadT;

  //Toda la carga se acumula en el string mientras el cliente esté disponible
  while (mqttClient.available()) {
    payloadT += ((char)mqttClient.read());
  }

  //Deserialización del objecto
  deserializeJson(docT, payloadT);
  const char* messageT = docT["message"];

  printLCDStatus("Mensaje recibido", String(messageT), emptyDataT, emptyDataT);
}


//Función que imprime los mensajes en la LCD y el puerto serie
void printLCDStatus(String fMessageT, String sMessageT, uint8_t fStateT, uint8_t sStateT) {
  const uint16_t delayMessageT = 500;
  const char *statesT[] = {": ERROR", ": OK"};

  //Evalua solo el primer estado mandado en la funcion
  switch (fStateT) {
    case 0:
      fMessageT += statesT[0];
      if (sMessageT != "" && (fStateT != 2 && sStateT != 2))
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT == "" && sStateT != 2)
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT != "" && sStateT == 2)
        sMessageT += "";
      break;
    case 1:
      fMessageT += statesT[1];
      if (sMessageT != "" && (fStateT != 2 && sStateT != 2))
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT == "" && sStateT != 2)
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT != "" && sStateT == 2)
        sMessageT += "";
      break;
    default:
      if (sMessageT != "" && (fStateT != 2 || sStateT != 2))
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT == "" && sStateT != 2)
        sMessageT += (sStateT) ? statesT[1] : statesT[0];
      else if (sMessageT != "" and (fStateT == 2 || sStateT == 2)) {
        fMessageT += "";
        sMessageT += "";
      }
      break;
  }

  Serial.println(fMessageT + " || " + sMessageT);

  lcdT.clear();
  lcdT.print(fMessageT);
  lcdT.setCursor(0, 1);
  lcdT.print(sMessageT);
  //delay(delayMessageT);
}

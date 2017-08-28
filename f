/*
 Sketch which publishes data from a dht22 sensor to a MQTT topic.

 This sketch goes in deep sleep mode once the temperature has been sent to the MQTT
 topic and wakes up periodically (configure SLEEP_DELAY_IN_SECONDS accordingly).

 Hookup guide:
 - connect D0 pin to RST pin in order to enable the ESP8266 to wake up periodically

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>
//#include <Adafruit_Sensor.h>
//#include <DHT.h>
#include <string.h>
#include <malloc.h>

//add at the beginning of sketch // to check battery voltage using internal adc
// ADC_MODE(ADC_VCC); will result in compile error. use this instead.
//int __get_adc_mode(void) { return (int) (ADC_VCC); }
ADC_MODE(ADC_VCC); //vcc read //TOUT pin has to be disconnected in this mode.

#include <dht.h> // src: https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable

dht DHT;

//Constants
//#define DHTPIN D4     // what pin we're connected to
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
#define DHT22_PIN D4
// Static IP details...
//IPAddress ip(192, 168, 1, 178);
//IPAddress gateway(192, 168, 1, 1);
//IPAddress subnet(255, 255, 255, 0);
//IPAddress DNS(192, 168, 1, 1);

struct
{
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
} stat = { 0,0,0,0,0,0,0,0};

//#define SLEEP_DELAY_IN_SECONDS  600
#define SLEEP_DELAY_IN_SECONDS  180  // value in secs
#define ONE_WIRE_BUS            D4      // DS18B20 pin

//const char* ssid = "SEIL_SCC";
//const char* ssid = "SEIL";
const char* ssid = "SEIL_sense";
const char* password = "deadlock123";

float prev_temp = 0.0 ;

const char* mqtt_server = "10.129.23.41";  //production machine
// const char* mqtt_server = "10.129.23.30";  //uddhav's machine
// const char* mqtt_server = "192.168.0.101";   //local network of router
const char* mqtt_username = "<MQTT_BROKER_USERNAME>";
const char* mqtt_password = "<MQTT_BROKER_PASSWORD>";

// const char* mqtt_topic = "data/kresit/dht/SEIL"; //test
const char* mqtt_topic = "nodemcu/kresit/dht/FCK"; //production
//const char* mqtt_topic = "nodemcu/kresit/dht/SIC205"; //classroom
//const char* mqtt_topic = "nodemcu/kresit/dht/SEIL"; //LAB
const char* client_id = "dht_FCK_31";
char node_id_String[] = "31";

WiFiClient espClient;
PubSubClient client(espClient);

//OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature DS18B20(&oneWire);

char temperatureString[6];
char humidityString[6];
char vddString[6];
char result[30];

float vdd = 0.0; //for measuring supply voltage 
float calibration_factor = 1.197; //for measuring supply voltage

void setup() {
  // setup serial port
  Serial.begin(9600);

  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup OneWire bus
  //DS18B20.begin();
  //dht.begin();
  Serial.println(mqtt_topic);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //Serial.println(password);
 // WiFi.config(ip, gateway, subnet, DNS);
//  delay(100);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*float getTemperature() {
  Serial.println("Requesting DS18B20 temperature...");
  float temp;
  Serial.print("Previous temperature is ...") ;
  Serial.println(prev_temp);
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  prev_temp = temp ;
  
  return temp;
}*/

//float getTemperature() {
//  Serial.println("Requesting DHT temperature...");
//  float temp;
//  temp= dht.readTemperature();
//  return temp;
//}
//
//float getHumidity() {
//  Serial.println("Requesting DHT temperature...");
//  float hum;
//  hum = dht.readHumidity();
//  return hum;
//}

void read_dht_data()
{
    // READ DATA
    Serial.print("DHT22, \t");

    uint32_t start = micros();
    int chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();

    stat.total++;
    switch (chk)
    {
    case DHTLIB_OK:
        stat.ok++;
        Serial.print("OK,\t");
        break;
    case DHTLIB_ERROR_CHECKSUM:
        stat.crc_error++;
        Serial.print("Checksum error,\t");
        break;
    case DHTLIB_ERROR_TIMEOUT:
        stat.time_out++;
        Serial.print("Time out error,\t");
        break;
    default:
        stat.unknown++;
        Serial.print("Unknown error,\t");
        break;
    }
    // DISPLAY DATA
//    Serial.print(DHT.humidity, 1);
    Serial.print(DHT.humidity);
    Serial.print(",\t");
//    Serial.print(DHT.temperature, 1);
    Serial.print(DHT.temperature);
    Serial.print(",\t");
    Serial.print(stop - start);
    Serial.println();

    if (stat.total % 20 == 0)
    {
        Serial.println("\nTOT\tOK\tCRC\tTO\tUNK");
        Serial.print(stat.total);
        Serial.print("\t");
        Serial.print(stat.ok);
        Serial.print("\t");
        Serial.print(stat.crc_error);
        Serial.print("\t");
        Serial.print(stat.time_out);
        Serial.print("\t");
        Serial.print(stat.connect);
        Serial.print("\t");
        Serial.print(stat.ack_l);
        Serial.print("\t");
        Serial.print(stat.ack_h);
        Serial.print("\t");
        Serial.print(stat.unknown);
        Serial.println("\n");
    }  
  
}

float measure_vdd()
{

  float vdd = calibration_factor*ESP.getVcc() / 1000.0;
  //Serial.println(vdd);
  return vdd;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float vdd = measure_vdd();
//  float temperature = getTemperature();
//  float humidity = getHumidity();
  //delay(500);
  read_dht_data();
  float temperature = DHT.temperature;
  float humidity = DHT.humidity;
  // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temperature, 2, 2, temperatureString);
  dtostrf(humidity, 2, 2, humidityString);
  dtostrf(vdd, 2, 2, vddString);
  
  // send temperature to the serial console
  Serial.print("Sending temperature: ");
  Serial.println(temperatureString);

  // send humidity to the serial console
  Serial.print("Sending humidity: ");
  Serial.println(humidityString);

  // send temperature to the serial console
  Serial.print("Sending VDD: ");
  Serial.println(vddString);
  
  // send data to the MQTT topic 
  strcpy(result, node_id_String);
  strcat(result,",");
  strcat(result, temperatureString);
  strcat(result,",");
  strcat(result, humidityString);
  strcat(result,",");
  strcat(result, vddString);
  Serial.println(result);
  client.publish(mqtt_topic, result);

  //Serial.println("Closing MQTT connection...");
  client.disconnect();

  //Serial.println("Closing WiFi connection...");
  WiFi.disconnect();

  delay(100);

  Serial.print("Entering deep sleep mode for ");
  Serial.println( SLEEP_DELAY_IN_SECONDS );
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  ESP.deepSleep(10 * 1000, WAKE_NO_RFCAL);
  delay(500);   // wait for deep sleep to happen
}

 Serial.print("Entering deep sleep mode for ");
  Serial.println( SLEEP_D4,dY_IN_SECONDS );
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  ESP.deepSleep(10 * 1000, WAKE_NO_RFCAL);
  delay(500);   // wait for deep sleep to happen
}

 Serial.print("Entering deep sleep mode for ");
  Serial.println( SLEEP_D
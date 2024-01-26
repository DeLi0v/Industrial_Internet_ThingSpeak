#include <Arduino.h>
#include <ESP8266WiFi.h>        // Библиотека для работы с Wi-Fi на ESP
#include <Wire.h>               // Библиотека для работы с I2C шиной
#include <SPI.h>                // Библиотека для работы с устройствами поддерживающими SPI протокол, хз зачем она тут, но без этой библиотеки будет вылезать ошибка 
#include <Adafruit_Sensor.h>    // Общая библиотека Adafruit, без нее не будут работать их остальные библиотеки
#include <DHT.h>                // Библиотека для DHT11
#include <Adafruit_BMP085.h>    // Библиотека для BMP180 
#include <OneWire.h>            // Библиотека для работы с 1-Wire шиной (для подключения DS18b20, т.к. он работает на этой шине)
#include <DallasTemperature.h>  // Библиотека для облегчения работы с подключением DS18b20
#include <BH1750.h>             // Библиотека для работы с BH1750

String api_key = "YOUR WRITE API KEY";        //  Введите свой ключ Write API записи из ThingSpeak
const char *ssid = "YOUR WIFI SSID";               // замените на ваш Wi-Fi ssid и ключ wpa2
const char *pass = "YOUR WIFI PASSWORD";         // замените на ваш пароль Wi-Fi
const char *server = "api.thingspeak.com";  //   "184.106.153.149" или api.thingspeak.com

#define ONE_WIRE_BUS 2 // Пин для подключения 1-Wire шины и DS18B20 (D4)
#define sensorPin A0 // Пин для подключения емкостного датчика влажности почвы
#define DHTPIN D8     // use pins 0 (D3), 2 (D4), 12 (D6), 13 (D7), 14 (D5) or 15 (D8)
#define DHTTYPE DHT11   // DHT 11

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds(&oneWire); // Pass our oneWire reference to Dallas Temperature.

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
BH1750 lightMeter;

void setup()
{
  Serial.begin(115200);
  delay(10);

  Wire.begin(D2, D1); // "Запускаем" I2C шину 
  // "Запускаем" датчики
  dht.begin();        // DHT11
  lightMeter.begin(); // BH1750
  bmp.begin();        // BMP180
  ds.begin();    // DS18B20

  // Подключаемся к Wi-Fi
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

float getHumidity() { // DHT11 - влажность
    float _h = dht.readHumidity();
    if (isnan(_h)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return 0;
    }

    return _h;
}

float getTemp_DHT() { // DHT11 - температура (для общего понимая, лучше брать с DS18B20)
    float _t = dht.readTemperature();
    if (isnan(_t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return 0;
    }

    return _t;
}

float getLight() { // BH1750 - уровень освещенности
    float _lux = lightMeter.readLightLevel();

    return _lux;
}

float getTemp_BMP() { // BMP180 - температура (для общего понимая, лучше брать с DS18B20)

    return bmp.readTemperature();
}

float getPressure() { // BMP180 - давление

    float _pressure = bmp.readPressure(); // В Паскалях
    // float _pressure = bmp.readPressure() / 133.3; // В мм. рт. столба 

    return _pressure;
}

float getAltitude() { // BMP180 - высота над уровнем моря

    float _altitude = bmp.readAltitude(); 
    Serial.print("Altitude = ");
    Serial.print(bmp.readAltitude());
    Serial.println(" meters");

    return _altitude;
}

float getTemp_DS() { // DS18B20 - температура (лучше брать эту)

  ds.requestTemperatures(); // считываем температуру с датчиков, на это требуется 750мс

  float _t = ds.getTempCByIndex(0); // получаем температуру

  return _t;
}

float getEarthHumidity() { // Емкостного датчик влажности почвы - влажность почвы
  float _h = analogRead(sensorPin);

  return _h;
}

void loop()
{
  // Записываем данные с датчиков в переменные
  float temp = getTemp_DS(); // Температура с DS18B20
  // float humidity = getHumidity(); // Влажность с датчика DHT11
  // float pressure = getPressure(); // Давление с BMP180
  float light = getLight(); // Уровень освещенности с BH1750
  float earthHumidity = getEarthHumidity(); // Емкостной датчик влажности почвы

  if (client.connect(server, 80))  // Отправляем HTTP запрос на сервер ThingSpeak - по простому - отправляем данные в ThingSpeak
  {
    String data_to_send = api_key;
    data_to_send += "&field1="; // Поле для вывода 1, включается в Channel Settings
    data_to_send += temp;
    // data_to_send += "&field2="; // Поле для вывода 2, включается в Channel Settings
    // data_to_send += humidity;
    // data_to_send += "&field3="; // Поле для вывода 3, включается в Channel Settings
    // data_to_send += pressure;
    data_to_send += "&field4="; // Поле для вывода 4, включается в Channel Settings
    data_to_send += light;
    data_to_send += "&field5="; // Поле для вывода 5, включается в Channel Settings
    data_to_send += earthHumidity;
    // data_to_send += "\r\n\r\n\r\n\r\n\r\n";
    data_to_send += "\r\n";


    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + api_key + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);
    delay(1000);
    
    // Вывод данных в монитор порта, если не хотите пользоваться - можно удалить
    Serial.print("temp: ");
    Serial.println(temp);
    // Serial.print("Humidity: ");
    // Serial.println(humidity);
    // Serial.print("pressure: ");
    // Serial.println(pressure);
    Serial.print("light: ");
    Serial.println(light);
    Serial.print("earthHumidity: ");
    Serial.println(earthHumidity);
    Serial.println("Send to Thingspeak.");
    
  }
  client.stop();

  Serial.println("Waiting...");

  delay(10000);
}
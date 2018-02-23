/*
Habitat Hub Arduino Code
Aurthor: Garth Frenzel (c) 2018
*/

#include <math.h>
#include <BridgeClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_RGBLCDShield.h>
#include <Process.h>


// These #defines make it easy to set the backlight color
#define RED 0x1
#define GREEN 0x2
#define BLUE 0x4

// Update these with values suitable for your hardware/network.
const byte mac[]    = {  0xB4, 0x21, 0x8A, 0xF0, 0x74, 0xFE };
const IPAddress ip(192, 168, 1, 212);
const IPAddress server(192, 168, 1, 212);
const DeviceAddress coolSpotThermometer = {0x28, 0xFF, 0xF3, 0xFA, 0xB1, 0x17, 0x04, 0x3E};
const DeviceAddress hotSpotThermometer  = {0x28, 0xFF, 0x63, 0xFA, 0xB1, 0x17, 0x04, 0x89};

// BUTTONS
const uint8_t BTN_CLEAN_CAGE_PIN = 8;
const uint8_t BTN_CLEAN_WATER_PIN = 9;

// DHT 22 Configuration
const uint8_t DHTTYPE = DHT22;
const uint8_t DHTPIN = 4;

// Ambtemp data wire is plugged into pin 2
const byte ONE_WIRE_BUS = 5;
const long SENSOR_REFRESH_RATE = 15000;
const long DISPLAY_CAROUSEL_RATE = 15000;

// Unit ID
const char DEVICEID[] =  "Habhub/hh_1234";
const char CONFIG_SAVE_FILE_NAME[] = "/root/hh/shhc.py";
const char CONFIG_LOAD_FILE_NAME[] = "/root/hh/lhhc.py";

// DISPLAY TEXT Using PROGMEM
const char string_0[] PROGMEM = "Hot Spot Temp";
const char string_1[] PROGMEM = "Cool Spot Temp";
const char string_2[] PROGMEM = "Ambient Temp";
const char string_3[] PROGMEM = "Humidity";
const char string_4[] PROGMEM = "Water Level";
const char string_5[] PROGMEM = "Clean Water Bowl";
const char string_6[] PROGMEM = "Clean Cage";

const char* const display_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5, string_6 };

const char menu_0[] PROGMEM = "Hot Spot Temp Hi";
const char menu_1[] PROGMEM = "Hot Spot Temp Lo";
const char menu_2[] PROGMEM = "Col Spot Temp Hi";
const char menu_3[] PROGMEM = "Col Spot Temp Lo";
const char menu_4[] PROGMEM = "Ambient Temp Hi";
const char menu_5[] PROGMEM = "Ambient Temp Lo";
const char menu_6[] PROGMEM = "Humidity Hi";
const char menu_7[] PROGMEM = "Humidity Lo";
const char menu_8[] PROGMEM = "Water Level Lo";
const char menu_9[] PROGMEM = "Clean Water Bowl";
const char menu_10[] PROGMEM = "Clean Cage";

const char* const menu_table[] PROGMEM = {menu_0, menu_1, menu_2, menu_3, menu_4, menu_5, menu_6, menu_7, menu_8, menu_9, menu_10 };

// Program wide buffer
char buffer[30]={'\0'};

// The shield uses the I2C SCL and SDA pins.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
DHT_Unified dht(DHTPIN, DHTTYPE);
/*******************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass the oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Define MQTT Objects and variables
BridgeClient bridge;

unsigned long lastReconnectAttempt;
unsigned long lastReadSensor;
unsigned long lastDisplayShow;
unsigned long lastGetDate;
int displayIndex = -1;
unsigned long todayDate;
bool btnMenu = false;
byte menuIndex = -1;

// byte hourNow;

// Configuration that we'll store on SD using FileSystem
struct Config {
  uint8_t hottemphi;
  uint8_t hottemplo;
  uint8_t cooltemphi;
  uint8_t cooltemplo;
  uint8_t ambtemphi;
  uint8_t ambtemplo;
  uint8_t humidityhi;
  uint8_t humiditylo;
  uint8_t waterlevel;
  uint8_t clnwaterdays;
  unsigned long lastwaterday;
  uint8_t clncagedays;
  unsigned long lastcageday;
};

// Used to store current and last values
struct Values {
  int hottemp;
  int cooltemp;
  int ambtemp;
  byte humidity;
  byte waterlevel;
  unsigned long waterday;
  unsigned long cageday;
};

Config config;
Values values;
Values lastValues;

unsigned long getDateTime(const char* fmt){
  Process date;
  date.begin(F("/bin/date"));
  date.addParameter(fmt);
  date.run();
  //String dateString;
  if(date.available()>0){
    strcpy(buffer,date.readString().c_str());
     //dateString = date.readString();
  }
  // return dateString.toInt();
  return atol(buffer);
}

unsigned long getDateTime(const __FlashStringHelper* pData){
  //char buffer[10];
  int cursor = 0;
  char *ptr = ( char *) pData;
  while( (buffer[cursor] = pgm_read_byte_near( ptr + cursor) ) != '\0') ++cursor;
  return getDateTime(buffer);
}

void putKeyValue(const char* key, const char* value){
  Bridge.put(key,value);
}

void putKeyValue(const char* key, const int value){
  sprintf(buffer,"%d", value);
  putKeyValue(key, buffer);
}

void putKeyValue(const char* key, const unsigned long value){
  sprintf(buffer,"%lu", value);
  putKeyValue(key, buffer);
}

uint8_t getKeyValue(const char* key, const uint8_t value){
  Bridge.get(key, buffer, sizeof(buffer));
  return strlen(buffer)==0 ? value:char(atoi(buffer));
}

unsigned long getKeyValue(const char* key, const unsigned long value){
  Bridge.get(key, buffer, sizeof(buffer));
  return strlen(buffer)==0 ? value:atol(buffer);
}
// Loads the configuration from a file
void loadConfiguration() {
  config.hottemphi = getKeyValue("htthi",(uint8_t)85);
  config.hottemplo = getKeyValue("httlo",(uint8_t)75);
  config.cooltemphi = getKeyValue("cothi",(uint8_t)80);
  config.cooltemplo = getKeyValue("cotlo",(uint8_t)70);
  config.ambtemphi = getKeyValue("abthi",(uint8_t)85);
  config.ambtemplo = getKeyValue("abtlo",(uint8_t)70);
  config.humidityhi = getKeyValue("humhi",(uint8_t)90);
  config.humiditylo = getKeyValue("humhi",(uint8_t)60);
  config.waterlevel = getKeyValue("wtrlv",(uint8_t)10);
  config.clnwaterdays = getKeyValue("clnwd",(uint8_t)7);
  config.lastwaterday = getKeyValue("lstwd",(unsigned long)0);
  config.clncagedays = getKeyValue("clncd",(uint8_t)30);
  config.lastcageday = getKeyValue("lstcd",(unsigned long)0);
  values.waterday = config.lastwaterday;
  values.cageday = config.lastcageday;
}

// Celsius To Fahrenheit converter
int celsiusToFahrenheit(int temp){
  return (temp*9+2)/5+32;
}

// Reads data from the sensors
boolean readSensors() {
  sensors.requestTemperatures();

  // Collecting data for Cool Spot Temperature
  lastValues.cooltemp = values.cooltemp;
  values.cooltemp = round(sensors.getTempF(coolSpotThermometer));

  // Collecting data for Hot Spot Temperature
  lastValues.hottemp = values.hottemp;
  values.hottemp = round(sensors.getTempF(hotSpotThermometer));

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    values.ambtemp = -99;
  }
  else {
    lastValues.ambtemp = values.ambtemp;
    values.ambtemp = celsiusToFahrenheit(event.temperature);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    values.humidity = -99;
  }
  else {
    lastValues.humidity = values.humidity;
    values.humidity = round(event.relative_humidity);
  }

  lastValues.waterlevel = values.waterlevel;
  values.waterlevel = analogRead(A5);
  values.waterlevel = values.waterlevel * 0.1;

  return true;
}

// Updates middleware using the Bridge Client
void sendSensorData(){
  if(lastValues.cooltemp != values.cooltemp){
    putKeyValue("cot", values.cooltemp ? values.cooltemp : -99);
  }
  if(lastValues.hottemp != values.hottemp){
    putKeyValue("hot", values.hottemp ? values.hottemp : -99);
  }
  if(lastValues.ambtemp != values.ambtemp){
    putKeyValue("amt", values.ambtemp ?  values.ambtemp : -99);
  }
  if(lastValues.humidity != values.humidity){
    putKeyValue("hum", values.humidity ? values.humidity : -99);
  }
  if(lastValues.waterlevel != values.waterlevel){
    putKeyValue("wal", values.waterlevel ? values.waterlevel : -99);
  }
  if(config.lastwaterday != values.waterday){
    putKeyValue("lstwd", values.waterday ? values.waterday : 0);
  }
  if(config.lastcageday != values.cageday){
    putKeyValue("lstcd", values.cageday ? values.cageday : 0);
  }
  if(config.lastwaterday != values.waterday || config.lastcageday != values.cageday){
    // Serial.println("Sav Cfg");
    putKeyValue("scfg", "1");
  }
}

// Menu LCD
void lcdMenuShow(const char* key, int value){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(key);
  lcd.setCursor(0,1);
  lcd.blink();
  lcd.print(value);
}

int lcdBackGroundColor(int index) {
  int color = GREEN;
  switch(displayIndex){
    case 0:
      color = values.hottemp < config.hottemplo ? BLUE: values.hottemp > config.hottemphi ? RED: GREEN;
      break;
    case 1:
      color = values.cooltemp < config.cooltemplo ? BLUE: values.cooltemp > config.cooltemphi ? RED: GREEN;
      break;
    case 2:
      color = values.ambtemp < config.ambtemplo ? BLUE: values.ambtemp > config.ambtemphi ? RED: GREEN;
      break;
    case 3:
      color = values.humidity < config.humiditylo ? BLUE: values.humidity > config.humidityhi ? RED: GREEN;
      break;
    case 4:
      color = values.waterlevel < config.waterlevel ? BLUE: values.waterlevel > config.waterlevel ? RED: GREEN;
      break;
    case 5:
      color = todayDate - config.lastwaterday > config.clnwaterdays ? RED:GREEN;
      break;
    case 6:
      color = todayDate - config.lastcageday > config.clncagedays ? RED:GREEN;
      break;
  }
  return color;
}

void lcdDisplayShow(int displayIndex, const char* key, int value){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(key);
  lcd.setCursor(0,1);
  // Set the background BLUE (too cold) GREEN (in range) RED (too hot)
  lcd.setBacklight(lcdBackGroundColor(displayIndex));

  switch(displayIndex){
    case 0:
    case 1:
    case 2:
    lcd.print(value);
    lcd.print((char)223);
    lcd.print(F("F"));
    break;
    case 3:
    lcd.print(value);
    lcd.print(F("%"));
    break;
    case 4:
    lcd.print(value);
    lcd.print(F("cm"));
    break;
    case 5:
    case 6:
    if (value < 0) {
      lcd.print(F("now!"));
    }
    else {
      lcd.print(F("In "));
      lcd.print(value);
      lcd.print(F(" days"));
    }
    break;
  }
}

// Check to see either Clean Water or Cage btn has been pressed
void checkButton(unsigned long &holder, uint8_t pin){
  int buttonState = digitalRead(pin);
  if(buttonState == HIGH){
    holder = todayDate;
    delay(500);
  }
}

void setup()
{
  // 1. Setup LCD
  // 2. Setup MQTT client settings
  // 3. loadConfiguration File
  // 4.
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setBacklight(RED);
  delay(10000);
  lcd.setBacklight(BLUE);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  Process p;
  p.runShellCommand(CONFIG_LOAD_FILE_NAME);

  // Should load default config if run for the first time
  loadConfiguration();

  // sensors
  sensors.begin();
  dht.begin();
  sensor_t sensor;

  // Setup DHT22 Sensor Delay
  dht.temperature().getSensor(&sensor);

  pinMode(BTN_CLEAN_CAGE_PIN, INPUT);
  pinMode(BTN_CLEAN_WATER_PIN, INPUT);

  delay(1500);
  lastReconnectAttempt = 0;

  todayDate = getDateTime(F("+%Y%m%d"));

  lcd.setBacklight(GREEN);

  values.hottemp = -99;
  values.cooltemp= -99;
  values.ambtemp= -99;
  values.humidity= -99;
  values.waterlevel= -99;
  values.waterday= 0;
  values.cageday= 0;
}

void loop()
{
  unsigned long nowTimer = millis();
  bool btnPressed = false;
  bool btnMenuPressed = false;

  if(nowTimer - lastGetDate > 1800000){
    lastGetDate = nowTimer;
    todayDate = getDateTime(F("+%Y%m%d"));
  }

  // Read all sensor data
  if (nowTimer - lastReadSensor > SENSOR_REFRESH_RATE){
    lastReadSensor = nowTimer;
    readSensors();
    sendSensorData();
  }

  // READ CAGE AND WATER BUTTONS
  lastValues.cageday = values.cageday;
  checkButton(values.cageday, BTN_CLEAN_CAGE_PIN);
  lastValues.waterday = values.waterday;
  checkButton(values.waterday, BTN_CLEAN_WATER_PIN);

  // Check user button input
  uint8_t buttons = lcd.readButtons();
  if(buttons) {
    delay(500);
    if (buttons & BUTTON_RIGHT) {
      if(!btnMenu){
        displayIndex = displayIndex + 1 >= 7 ? 0: displayIndex +1;
        btnPressed = true;
      }
      else menuIndex += 1;
    }

    if (buttons & BUTTON_LEFT){
      if(!btnMenu){
          btnPressed = true;
          if(displayIndex == 0) {
            displayIndex = 6;
          } else {
            displayIndex = displayIndex - 1;
          }
      }
      else{
        if(menuIndex == 0){
          menuIndex = 2;
        } else {
          menuIndex -= 1;
        }
      }
    }

    // Enter and Exits Configuration Menu
    if (buttons & BUTTON_SELECT){
      btnMenu = !btnMenu;
      if(btnMenu) menuIndex = 0;
      if(!btnMenu){
        lcd.noBlink();
        // Send message to save config because we got out.
        putKeyValue("htthi", config.hottemphi);
        putKeyValue("httlo", config.hottemplo);
        putKeyValue("cothi", config.cooltemphi);
        putKeyValue("cotlo", config.cooltemplo);
        putKeyValue("abthi", config.ambtemphi);
        putKeyValue("abtlo", config.ambtemplo);
        putKeyValue("humhi", config.humidityhi);
        putKeyValue("humlo", config.humiditylo);
        putKeyValue("wtrlv", config.waterlevel);
        putKeyValue("clnwd", config.clnwaterdays);
        putKeyValue("clncd", config.clncagedays);
        putKeyValue("scfg", "1");
      }
    }

    // Used for the configuration menu
    if (buttons & BUTTON_UP && btnMenu){
      switch(menuIndex){
        case 0:
          config.hottemphi -= 1;
          if(config.hottemphi < 50) config.hottemphi = 100;
          break;
        case 1:
          config.hottemplo -= 1;
          if(config.hottemplo < 50) config.hottemplo = 100;
          break;
        case 2:
          config.cooltemphi -= 1;
          if(config.cooltemphi < 50) config.cooltemphi = 100;
          break;
        case 3:
          config.cooltemplo -= 1;
          if(config.cooltemplo < 50) config.cooltemplo = 100;
          break;
        case 4:
          config.ambtemphi -= 1;
          if(config.ambtemphi < 50) config.ambtemphi = 100;
          break;
        case 5:
          config.ambtemplo -= 1;
          if(config.ambtemplo < 50) config.ambtemplo = 100;
          break;
        case 6:
          config.humidityhi -= 1;
          if(config.humidityhi < 0) config.humidityhi = 100;
          break;
        case 7:
          config.humiditylo -= 1;
          if(config.humiditylo < 0) config.humiditylo = 100;
          break;
        case 8:
          config.waterlevel -= 1;
          if(config.waterlevel < 0) config.waterlevel = 10;
          break;
        case 9:
          config.clnwaterdays -= 1;
          if(config.clnwaterdays < 0) config.clnwaterdays = 30;
          break;
        case 10:
          config.clncagedays -= 1;
          if(config.clncagedays < 0) config.clncagedays = 30;
          break;
      }
    }
    // Used for the configuration menu
    if (buttons & BUTTON_DOWN && btnMenu){
      switch(menuIndex){
        case 0:
          config.hottemphi += 1;
          if(config.hottemphi > 100) config.hottemphi = 50;
          break;
        case 1:
          config.hottemplo += 1;
          if(config.hottemplo > 100) config.hottemplo = 50;
          break;
        case 2:
          config.cooltemphi += 1;
          if(config.cooltemphi > 100) config.cooltemphi = 50;
          break;
        case 3:
          config.cooltemplo += 1;
          if(config.cooltemplo > 100) config.cooltemplo = 50;
          break;
        case 4:
          config.ambtemphi += 1;
          if(config.ambtemphi > 100) config.ambtemphi = 50;
          break;
        case 5:
          config.ambtemplo += 1;
          if(config.ambtemplo > 100) config.ambtemplo = 50;
          break;
        case 6:
          config.humidityhi += 1;
          if(config.humidityhi > 100) config.humidityhi = 0;
          break;
        case 7:
          config.humiditylo += 1;
          if(config.humiditylo > 100) config.humiditylo = 0;
          break;
        case 8:
          config.waterlevel += 1;
          if(config.waterlevel > 10) config.waterlevel = 0;
          break;
        case 9:
          config.clnwaterdays += 1;
          if(config.clnwaterdays > 30) config.clnwaterdays = 0;
          break;
        case 10:
          config.clncagedays += 1;
          if(config.clncagedays > 30) config.clncagedays = 0;
          break;
      }
    }

    if(btnMenu){
      btnMenuPressed = true;
    }
  }
  else
    btnPressed = false;

  // Rotate through data or if button press to change display
  // Only left and right button switches between sensor data
  if (((nowTimer - lastDisplayShow > DISPLAY_CAROUSEL_RATE) || btnPressed) && btnMenu == false ){ //|| btnPressed
    // readSensors();

    if(!btnPressed) {
      displayIndex = displayIndex + 1 >= 7 ? 0: displayIndex +1;
      // Include reload Configuration
      loadConfiguration();
    }
    //char buffer[17];
    if(displayIndex >= 0 && displayIndex <= 6){
      strcpy_P(buffer, (char*)pgm_read_word(&(display_table[displayIndex])));
    }
    lastDisplayShow = nowTimer;
    switch (displayIndex){
      case 0:
        lcdDisplayShow(displayIndex, buffer, values.hottemp);
        break;
      case 1: // Cool Spot Temperature
        lcdDisplayShow(displayIndex, buffer, values.cooltemp);
        break;
      case 2: // Ambient Temperature
        lcdDisplayShow(displayIndex, buffer, values.ambtemp);
        break;
      case 3: // humidity
        lcdDisplayShow(displayIndex, buffer, values.humidity);
        break;
      case 4: // Water Level
        lcdDisplayShow(displayIndex, buffer, values.waterlevel);
        break;
      case 5: // Days to Clean Water Bowl
        lcdDisplayShow(displayIndex, buffer, config.clnwaterdays - (todayDate - config.lastwaterday));
        break;
      case 6: // Days to Clean Cage
        lcdDisplayShow(displayIndex, buffer, config.clncagedays - (todayDate - config.lastcageday));
        break;
      default:
        displayIndex = -1;
        break;
    }
  }

  // This is for the Config Menu Screen
  if(btnMenu && btnMenuPressed){
      if(menuIndex >= 0 && menuIndex <= 6){
        strcpy_P(buffer, (char*)pgm_read_word(&(menu_table[menuIndex])));
      }
      switch(menuIndex){
        case 0:
            lcdMenuShow(buffer, config.hottemphi);
          break;
        case 1:
            lcdMenuShow(buffer, config.hottemplo);
          break;
        case 2:
            lcdMenuShow(buffer, config.cooltemphi);
          break;
        case 3:
            lcdMenuShow(buffer, config.cooltemplo);
          break;
        case 4:
            lcdMenuShow(buffer, config.ambtemphi);
          break;
        case 5:
            lcdMenuShow(buffer, config.ambtemplo);
          break;
        case 6:
            lcdMenuShow(buffer, config.humidityhi);
          break;
        case 7:
            lcdMenuShow(buffer, config.humiditylo);
          break;
        case 8:
            lcdMenuShow(buffer, config.waterlevel);
          break;
        case 9:
            lcdMenuShow(buffer, config.clnwaterdays);
          break;
        case 10:
            lcdMenuShow(buffer, config.clncagedays);
          break;
        default:
          menuIndex = 0;
          break;
      }
  }
}

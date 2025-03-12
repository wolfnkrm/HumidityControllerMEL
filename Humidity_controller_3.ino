/*Introduction
This program is for Humidity Controller inside the electrospinning chamber
Dedicated to Materials Electrochemistry Laboratory (MEL)
Institut Teknologi Bandung
Engineer: Faiq Haidar Hamid (23722016) & Anargya Adyatma Satria Pinandita (13720050)
Contacts: dahaidar22@gmail.com & anargya.adyatma@gmail.com
Note: Ganti nilai humidity ke yang diinginkan di line 35
--------------------------------------------------------------------------------------
// Install library before first use 
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>

// Hardware Configuration
#define DHTPIN 4
#define DIFFUSER1_PIN 26
#define DRYER_PIN 25
#define PELTIERUP_PIN 32
#define PELTIERDOWN_PIN 33
#define DHTTYPE DHT22

// Keypad w/ PCF8574 Setup
I2CKeyPad keyPad(0x20);
char keys[] = "D#0*C987B654A321N"; // Verified 4x4 keyPad mapping

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Sensor Instance
DHT dht(DHTPIN, DHTTYPE);

// System State
float humidity = 0, temperature = 0;
double targetHumidity = 60.0;   //<---------GANTI KE NILAI HUMIDITY YANG DIINGINKAN 
bool setHumidityMode = false;
int inputValue = 0, inputDigits = 0;
bool decimalEntered = false;
int decimalPosition = 0;

// Timing Control
unsigned long lastControlTime = 0;
unsigned long lastKeyTime = 0;
unsigned long lastSensorUpdate = 0;
const unsigned long CONTROL_INTERVAL = 2500;
const unsigned long SENSOR_INTERVAL = 1000;
const unsigned long DEBOUNCE_DELAY = 50;
const float TOLERANCE = 0.5;

// Function Prototypes
void showStartupMessage();
void updateSensorData();
void updateDisplay();
void handleHumidityControl();
void checkKeypadInput(unsigned long currentMillis);
void enterSetMode();
void handleHumiditySetting(unsigned long currentMillis);
void handleNumericInput(char key);
void handleDecimal();
void handleBackspace();
void updateInputDisplay();
void confirmHumidity();
void cancelHumiditySet();
void exitSetMode();
void setRelays(bool hum1, bool hum2, bool fan1, bool fan2);
void allRelaysOff();

void setup() {
  Serial.begin(115200);
  
  // Initialize Relays
  pinMode(DIFFUSER1_PIN, OUTPUT);
  pinMode(DRYER_PIN, OUTPUT);
  pinMode(PELTIERUP_PIN, OUTPUT);
  pinMode(PELTIERDOWN_PIN, OUTPUT);
  allRelaysOff();
  
  // Initialize Sensors
  dht.begin();
  /*keyPad.begin();
  if (keyPad.begin() == false)
  {
    Serial.println("\nERROR: cannot communicate to keyPad.\nPlease reboot.\n");
    while(1);
  }
  */
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  showStartupMessage();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (setHumidityMode) {
    //handleHumiditySetting(currentMillis);
  } else {
    if (currentMillis - lastSensorUpdate >= SENSOR_INTERVAL) {
      updateSensorData();
      lastSensorUpdate = currentMillis;
    }
    if (currentMillis - lastControlTime >= CONTROL_INTERVAL) {
      handleHumidityControl();
      lastControlTime = currentMillis;
    }
    //checkKeypadInput(currentMillis);
  }
}


void showStartupMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity Control");
  lcd.setCursor(0, 1);
  lcd.print("System Ready...");
  delay(2000);
  lcd.clear();
}

void updateSensorData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!   ");
    lcd.setCursor(0, 1);
    lcd.print("Check DHT      ");
    allRelaysOff();
    //humidity = plHumidity;
    //temperature = plTemp;
    //updateDisplay();
    return;
  }
  
  updateDisplay();
}

void updateDisplay() {
  // Line 1: Current readings
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(humidity, 1);
  lcd.print("% T:");
  lcd.print(temperature, 1);
  lcd.write(223); // Degree symbol
  lcd.print("C  ");
  
  // Line 2: Target humidity
  lcd.setCursor(0, 1);
  lcd.print("  Target:");
  lcd.print(targetHumidity, 1);
  lcd.print("% ");
  if (setHumidityMode) lcd.print("<SET>");
}

void handleHumidityControl() {
  if (humidity < targetHumidity - TOLERANCE) {
    // Below target: Both humidifiers + Fan1 ON
    digitalWrite(DIFFUSER1_PIN, LOW);
    digitalWrite(DRYER_PIN, HIGH);
    digitalWrite(PELTIERUP_PIN, HIGH);
    digitalWrite(PELTIERDOWN_PIN, HIGH);
  } else if (humidity > targetHumidity + TOLERANCE) {
    // Above target: Both fans ON
    digitalWrite(DIFFUSER1_PIN, HIGH);
    digitalWrite(DRYER_PIN, LOW);
    digitalWrite(PELTIERUP_PIN, LOW);
    digitalWrite(PELTIERDOWN_PIN, LOW);
  } else {
    // Within range: One humidifier + Fan1 ON
    digitalWrite(DIFFUSER1_PIN, LOW);
    digitalWrite(DRYER_PIN, HIGH);
    digitalWrite(PELTIERUP_PIN, LOW);
    digitalWrite(PELTIERDOWN_PIN, LOW);
  }
}
/*
void checkKeypadInput(unsigned long currentMillis) {
  if (currentMillis - lastKeyTime < DEBOUNCE_DELAY) return;
  
  char key = keyPad.getKey();
  if (key == 'A') {
    enterSetMode();
    lastKeyTime = currentMillis;
  }
}

void enterSetMode() {
  setHumidityMode = true;
  inputValue = 0;
  inputDigits = 0;
  decimalEntered = false;
  decimalPosition = 0;
  lcd.clear();
  lcd.print("Set Humidity:");
  lcd.setCursor(0, 1);
}

void handleHumiditySetting(unsigned long currentMillis) {
  if (currentMillis - lastKeyTime < DEBOUNCE_DELAY) return;
  
  char key = keyPad.getKey();
  if (!key) return;
  lastKeyTime = currentMillis;

  if (key == 'D') { // Confirm
    confirmHumidity();
  } else if (key == 'C') { // Cancel
    cancelHumiditySet();
  } else if (key == 'B') { // Backspace
    handleBackspace();
  } else if (key == '*') { // Decimal point
    handleDecimal();
  } else if (key >= '0' && key <= '9') { // Numeric input
    handleNumericInput(key);
  }
  
  updateInputDisplay();
}

void handleNumericInput(char key) {
  if (inputDigits < 5) { // Max 99.99
    inputValue = inputValue * 10 + (key - '0');
    inputDigits++;
    if (decimalEntered) decimalPosition++;
  }
}

void handleDecimal() {
  if (!decimalEntered && inputDigits > 0) {
    decimalEntered = true;
    decimalPosition = 0;
  }
}

void handleBackspace() {
  if (inputDigits > 0) {
    inputValue /= 10;
    inputDigits--;
    if (decimalEntered) {
      if (decimalPosition > 0) decimalPosition--;
      else decimalEntered = false;
    }
  }
}

void updateInputDisplay() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  
  if (decimalEntered) {
    int wholePart = inputValue / pow(10, decimalPosition);
    int decimalPart = inputValue % (int)pow(10, decimalPosition);
    lcd.print(wholePart);
    lcd.print('.');
    lcd.print(decimalPart);
  } else {
    lcd.print(inputValue);
  }
}

void confirmHumidity() {
  double newHumidity = inputValue;
  if (decimalEntered) {
    newHumidity /= pow(10, decimalPosition);
  }
  targetHumidity = constrain(newHumidity, 0, 99.99);
  exitSetMode();
}

void cancelHumiditySet() {
  exitSetMode();
}

void exitSetMode() {
  setHumidityMode = false;
  lcd.clear();
}
*/
void allRelaysOff() {
  digitalWrite(DIFFUSER1_PIN, HIGH);
  digitalWrite(DRYER_PIN, HIGH);
  digitalWrite(PELTIERUP_PIN, HIGH);
  digitalWrite(PELTIERDOWN_PIN, HIGH);
}

//--END OF FILE--

/*Introduction
This program is for Humidity Controller inside the electrospinning chamber
Dedicated to Materials Electrochemistry Laboratory (MEL)
Institut Teknologi Bandung
Engineer: Faiq Haidar Hamid (23722016) & Anargya Adyatma Satria Pinandita (13720050)
Contacts: dahaidar22@gmail.com & anargya.adyatma@gmail.com
--------------------------------------------------------------------------------------
*/
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>

// Hardware Configuration
#define DHTPIN 4
#define HUMIDIFIER1_PIN 15
#define HUMIDIFIER2_PIN 14
#define FAN1_PIN 16
#define FAN2_PIN 17
#define DHTTYPE DHT22

// Keypad w/ PCF8574 Setup
I2CKeyPad keypad(0x26);
char keys[] = "D#0*C987B654A321N"; // Verified 4x4 keypad mapping

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Sensor Instance
DHT dht(DHTPIN, DHTTYPE);

// System State
float humidity = 0, temperature = 0;
double targetHumidity = 50.0;
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
const float TOLERANCE = 2.5;

void setup() {
  Serial.begin(115200);
  
  // Initialize Relays
  pinMode(HUMIDIFIER1_PIN, OUTPUT);
  pinMode(HUMIDIFIER2_PIN, OUTPUT);
  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  allRelaysOff();
  
  // Initialize Sensors
  dht.begin();
  keypad.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  showStartupMessage();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (setHumidityMode) {
    handleHumiditySetting(currentMillis);
  } else {
    if (currentMillis - lastSensorUpdate >= SENSOR_INTERVAL) {
      updateSensorData();
      lastSensorUpdate = currentMillis;
    }
    if (currentMillis - lastControlTime >= CONTROL_INTERVAL) {
      handleHumidityControl();
      lastControlTime = currentMillis;
    }
    checkKeypadInput(currentMillis);
  }
}

void showStartupMessage() {
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
  lcd.print("Target:");
  lcd.print(targetHumidity, 1);
  lcd.print("% ");
  if (setHumidityMode) lcd.print("<SET>");
}

void handleHumidityControl() {
  if (humidity < targetHumidity - TOLERANCE) {
    // Below target: Both humidifiers + Fan1 ON
    digitalWrite(HUMIDIFIER1_PIN, HIGH);
    digitalWrite(HUMIDIFIER2_PIN, HIGH);
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, LOW);
  } else if (humidity > targetHumidity + TOLERANCE) {
    // Above target: Both fans ON
    digitalWrite(HUMIDIFIER1_PIN, LOW);
    digitalWrite(HUMIDIFIER2_PIN, LOW);
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, HIGH);
  } else {
    // Within range: One humidifier + Fan1 ON
    digitalWrite(HUMIDIFIER1_PIN, HIGH);
    digitalWrite(HUMIDIFIER2_PIN, LOW);
    digitalWrite(FAN1_PIN, HIGH);
    digitalWrite(FAN2_PIN, LOW);
  }
}

void checkKeypadInput(unsigned long currentMillis) {
  if (currentMillis - lastKeyTime < DEBOUNCE_DELAY) return;
  
  char key = keypad.getKey();
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
  
  char key = keypad.getKey();
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

void allRelaysOff() {
  digitalWrite(HUMIDIFIER1_PIN, LOW);
  digitalWrite(HUMIDIFIER2_PIN, LOW);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);
}
/*Introduction
This program is for the latest version of Humidity Controller inside the electrospinning chamber
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
#define DIFFUSER1_PIN 26
#define DRYER_PIN 25
#define PELTIERUP_PIN 32
#define PELTIERDOWN_PIN 33
#define DHTTYPE DHT22

// Keypad w/ PCF8574 Setup
I2CKeyPad keyPad(0x20);
uint8_t keys[] = "D#0*C987B654A321N"; // Verified 4x4 keyPad mapping

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
const unsigned long DEBOUNCE_DELAY = 200;
const float TOLERANCE = 0.5;

// Configuration Constants
const double MIN_HUMIDITY = 0.0;
const double MAX_HUMIDITY = 99.99;
const int MAX_INPUT_DIGITS = 5; // For 99.99 format

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
char getKeypadKey();

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
  
  // Initialize Keypad
  Wire.begin();
  if (keyPad.begin() == false) {
    Serial.println("\nERROR: cannot communicate to keyPad.\nPlease reboot.\n");
    while(1);
  }
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  showStartupMessage();
  
  Serial.println("System initialized successfully");
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
    lcd.print("Check DHT       ");
    allRelaysOff();
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
  lcd.print("%");
  if (setHumidityMode) {
    lcd.print(" <SET>");
  } else {
    lcd.print("     ");
  }
}

void handleHumidityControl() {
  if (humidity < targetHumidity - TOLERANCE) {
    // Below target: Diffuser ON, Dryer OFF, both Peltiers ON
    digitalWrite(DIFFUSER1_PIN, LOW);   // ON
    digitalWrite(DRYER_PIN, HIGH);      // OFF
    digitalWrite(PELTIERUP_PIN, LOW);   // ON
    digitalWrite(PELTIERDOWN_PIN, LOW); // ON
    Serial.println("Mode: Humidifying");
  } else if (humidity > targetHumidity + TOLERANCE) {
    // Above target: Diffuser OFF, Dryer ON, both Peltiers OFF
    digitalWrite(DIFFUSER1_PIN, HIGH);  // OFF
    digitalWrite(DRYER_PIN, LOW);       // ON
    digitalWrite(PELTIERUP_PIN, HIGH);  // OFF
    digitalWrite(PELTIERDOWN_PIN, HIGH);// OFF
    Serial.println("Mode: Dehumidifying");
  } else {
    // Within range: Diffuser ON, Dryer OFF, one Peltier OFF
    digitalWrite(DIFFUSER1_PIN, LOW);   // ON
    digitalWrite(DRYER_PIN, HIGH);      // OFF
    digitalWrite(PELTIERUP_PIN, HIGH);  // OFF
    digitalWrite(PELTIERDOWN_PIN, HIGH);// OFF
    Serial.println("Mode: Maintaining");
  }
}

void checkKeypadInput(unsigned long currentMillis) {
  if (currentMillis - lastKeyTime < DEBOUNCE_DELAY) return;
  
  char key = getKeypadKey();
  if (key != 0 && key == 'A') {
    enterSetMode();
    lastKeyTime = currentMillis;
    Serial.println("Entering set mode");
  }
}

void enterSetMode() {
  setHumidityMode = true;
  inputValue = 0;
  inputDigits = 0;
  decimalEntered = false;
  decimalPosition = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Humidity:");
  lcd.setCursor(0, 1);
  lcd.print("Input: ");
  updateInputDisplay();
}

void handleHumiditySetting(unsigned long currentMillis) {
  if (currentMillis - lastKeyTime < DEBOUNCE_DELAY) return;
  
  char key = getKeypadKey();
  if (key == 0) return; // No key pressed
  
  lastKeyTime = currentMillis;
  Serial.print("Key pressed in set mode: ");
  Serial.println(key);

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
  // Convert character to numeric value
  int digit = key - '0';  // This works because key is ASCII character '0'-'9'
  
  if (inputDigits < MAX_INPUT_DIGITS) {
    inputValue = inputValue * 10 + digit;
    inputDigits++;
    if (decimalEntered) {
      decimalPosition++;
    }
    Serial.print("Input value: ");
    Serial.print(inputValue);
    Serial.print(", digits: ");
    Serial.println(inputDigits);
  }
}

void handleDecimal() {
  if (!decimalEntered && inputDigits > 0 && inputDigits < MAX_INPUT_DIGITS - 1) {
    decimalEntered = true;
    decimalPosition = 0;
    Serial.println("Decimal entered");
  }
}

void handleBackspace() {
  if (inputDigits > 0) {
    inputValue /= 10;
    inputDigits--;
    if (decimalEntered) {
      if (decimalPosition > 0) {
        decimalPosition--;
      } else {
        decimalEntered = false;
      }
    }
    Serial.print("Backspace - Input value: ");
    Serial.print(inputValue);
    Serial.print(", digits: ");
    Serial.println(inputDigits);
  }
}

void updateInputDisplay() {
  lcd.setCursor(7, 1);
  lcd.print("         "); // Clear input area
  lcd.setCursor(7, 1);
  
  if (inputDigits == 0) {
    lcd.print("_");
    return;
  }
  
  if (decimalEntered && decimalPosition > 0) {
    // User explicitly entered decimal
    int wholePart = inputValue / (int)pow(10, decimalPosition);
    int decimalPart = inputValue % (int)pow(10, decimalPosition);
    lcd.print(wholePart);
    lcd.print('.');
    
    // Print decimal part with leading zeros if needed
    for (int i = decimalPosition - 1; i > 0; i--) {
      if (decimalPart < pow(10, i)) {
        lcd.print('0');
      }
    }
    lcd.print(decimalPart);
  } else {
    // Show preview of auto-decimal interpretation
    if (inputDigits == 3) {
      // Show as XY.Z (702 -> "70.2")
      int wholePart = inputValue / 10;
      int decimalPart = inputValue % 10;
      lcd.print(wholePart);
      lcd.print('.');
      lcd.print(decimalPart);
    } else if (inputDigits == 4) {
      // Show as XY.ZW (7025 -> "70.25")
      int wholePart = inputValue / 100;
      int decimalPart = inputValue % 100;
      lcd.print(wholePart);
      lcd.print('.');
      if (decimalPart < 10) lcd.print('0'); // Leading zero
      lcd.print(decimalPart);
    } else {
      // 1-2 digits: show as-is
      lcd.print(inputValue);
      if (decimalEntered) {
        lcd.print('.');
      }
    }
  }
}

void confirmHumidity() {
  if (inputDigits == 0) {
    // No input, cancel
    cancelHumiditySet();
    return;
  }
  
  double newHumidity = inputValue;
  
  if (decimalEntered && decimalPosition > 0) {
    // User explicitly entered decimal point
    newHumidity /= pow(10, decimalPosition);
  } else {
    // Auto-decimal logic: interpret based on number of digits
    if (inputDigits == 3) {
      // 3 digits: XY.Z format (702 -> 70.2)
      newHumidity /= 10.0;
    } else if (inputDigits == 4) {
      // 4 digits: XY.ZW format (7025 -> 70.25)
      newHumidity /= 100.0;
    }
    // 1-2 digits remain as whole numbers (7 -> 7.0, 70 -> 70.0)
  }
  
  // Check if input is valid before constraining
  bool isValidRange = (newHumidity >= MIN_HUMIDITY && newHumidity <= MAX_HUMIDITY);
  
  if (!isValidRange) {
    // Show sarcastic message for invalid input
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Are U serious?");
    lcd.setCursor(0, 1);
    lcd.print("Try again...");
    delay(2000);
    
    // Return to input mode instead of setting invalid value
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Humidity:");
    lcd.setCursor(0, 1);
    lcd.print("Input: ");
    
    // Reset input
    inputValue = 0;
    inputDigits = 0;
    decimalEntered = false;
    decimalPosition = 0;
    updateInputDisplay();
    return;
  }
  
  targetHumidity = newHumidity;
  
  Serial.print("New target humidity set to: ");
  Serial.println(targetHumidity);
  
  // Show confirmation message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Target Set:");
  lcd.setCursor(0, 1);
  lcd.print(targetHumidity, 1);
  lcd.print("%");
  delay(1500);
  
  exitSetMode();
}

void cancelHumiditySet() {
  Serial.println("Humidity setting cancelled");
  exitSetMode();
}

void exitSetMode() {
  setHumidityMode = false;
  lcd.clear();
  // Force immediate display update
  updateDisplay();
}

void allRelaysOff() {
  digitalWrite(DIFFUSER1_PIN, HIGH);  // HIGH = OFF
  digitalWrite(DRYER_PIN, HIGH);      // HIGH = OFF
  digitalWrite(PELTIERUP_PIN, HIGH);  // HIGH = OFF
  digitalWrite(PELTIERDOWN_PIN, HIGH);// HIGH = OFF
  Serial.println("All relays OFF");
}

char getKeypadKey() {
  uint8_t index = keyPad.getKey();
  if (index == I2CKEYPAD_NOKEY) {
    return 0; // No key pressed
  }
  
  // Convert index to character using the mapping
  if (index < 16) {
    return keys[index];
  }
  
  return 0; // Invalid index
}
//--END OF FILE--
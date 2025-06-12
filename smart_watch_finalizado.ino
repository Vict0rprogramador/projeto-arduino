#include <Wire.h>
#include <DS3231.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <math.h>

// ====== Pinos do Bluetooth HC-05 ======
SoftwareSerial mySerial(9, 8); // RX, TX

// ====== Pinos do LED e Botão ======
#define ledPin 2
#define buttonPin 12

// ====== Pinos do Display TFT ST7735 ======
#define TFT_CS   10
#define TFT_RST  6
#define TFT_DC   7
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ====== RTC ======
DS3231 rtc(SDA, SCL);
Time now;

// ====== Variáveis de Controle ======
char command = 0;
bool lastButtonState = HIGH;
unsigned long lastButtonPress = 0;
bool waitingForDoubleClick = false;
bool longPressDetected = false;

enum Mode { MODE_OI, MODE_MODERN, MODE_ANALOG, MODE_STOPWATCH };
Mode currentMode = MODE_MODERN;

// ====== Cronômetro ======
bool stopwatchRunning = false;
unsigned long stopwatchStart = 0;
unsigned long stopwatchElapsed = 0;

// ====== Controle de troca de modo por botão ======
unsigned long lastModeChange = 0;
const unsigned long debounceDelay = 300;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  rtc.begin();

  // -------- AJUSTAR AQUI --------
  // rtc.setDOW(WEDNESDAY);
  // rtc.setTime(18, 29, 0);
  // rtc.setDate(11, 6, 2025);
  // ------------------------------

  Serial.println("Comandos:");
  Serial.println("  '1' - Ligar LED");
  Serial.println("  '0' - Desligar LED");
  Serial.println("  '2' - Modo previsão (animação)");
  Serial.println("  '4' - Modo moderno");
  Serial.println("  '5' - Modo analógico");
  Serial.println("  '3' - Modo cronômetro");
  Serial.println("  'A' - Iniciar/pausar cronômetro");
  Serial.println("  'B' - Zerar cronômetro");
}

void loop() {
  if (mySerial.available()) {
    command = mySerial.read();
    switch (command) {
      case '1': digitalWrite(ledPin, HIGH); break;
      case '0': digitalWrite(ledPin, LOW); break;
      case '2': tft.fillScreen(ST77XX_BLACK); currentMode = MODE_OI; break;
      case '4': tft.fillScreen(ST77XX_BLACK); currentMode = MODE_MODERN; break;
      case '5': tft.fillScreen(ST77XX_BLACK); currentMode = MODE_ANALOG; break;
      case '3': tft.fillScreen(ST77XX_BLACK); currentMode = MODE_STOPWATCH; break;
      case 'A': toggleStopwatch(); break;
      case 'B': resetStopwatch(); break;
    }
  }

  bool buttonState = digitalRead(buttonPin);
  unsigned long nowMillis = millis();

  if (buttonState == LOW && lastButtonState == HIGH) {
    digitalWrite(ledPin, HIGH);
    if (waitingForDoubleClick && nowMillis - lastButtonPress < 400) {
      toggleStopwatch();
      waitingForDoubleClick = false;
    } else {
      waitingForDoubleClick = true;
      lastButtonPress = nowMillis;
    }
    if (nowMillis - lastModeChange > debounceDelay) {
      nextMode();
      lastModeChange = nowMillis;
    }
  } else if (buttonState == LOW && !longPressDetected && nowMillis - lastButtonPress > 2000) {
    resetStopwatch();
    longPressDetected = true;
  } else if (buttonState == HIGH && lastButtonState == LOW) {
    digitalWrite(ledPin, LOW);
    longPressDetected = false;
  }

  lastButtonState = buttonState;

  if (Serial.available()) {
    mySerial.write(Serial.read());
  }

  switch (currentMode) {
    case MODE_OI: showOi(); break;
    case MODE_MODERN: drawModernStyle(); break;
    case MODE_ANALOG: drawAnalogClock(); break;
    case MODE_STOPWATCH: drawStopwatch(); break;
  }
}

void nextMode() {
  currentMode = (Mode)(((int)currentMode + 1) % 4);
  tft.fillScreen(ST77XX_BLACK);
}

void showOi() {
  static unsigned long lastUpdate = 0;
  static int cloudX = 0;
  static bool moonBright = true;
  static int frame = 0;

  if (millis() - lastUpdate < 1500) return;
  lastUpdate = millis();

  tft.fillScreen(ST77XX_BLACK);

  // Nome da cidade centralizado
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  int cityX = (128 - (8 * 2 * 6)) / 2;
  tft.setCursor(cityX, 5);
  tft.print("Mesquita");

  // Lua piscando
  moonBright = (frame % 20 < 10);
  uint16_t moonColor = moonBright ? ST77XX_YELLOW : ST77XX_WHITE;
  tft.fillCircle(64, 30, 10, moonColor);

  // Nuvem animada
  int cx = 39 + cloudX;
  tft.fillCircle(cx, 50, 6, ST77XX_WHITE);
  tft.fillCircle(cx + 8, 50, 6, ST77XX_WHITE);
  tft.fillRect(cx - 2, 50, 14, 6, ST77XX_WHITE);
  cloudX += 1;
  if (cloudX > 50) cloudX = -20;

  // Temperatura
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  int tempX = (128 - 36) / 2; // 2 dígitos grandes + espaço pro símbolo
  tft.setCursor(tempX, 75);
  tft.print("19");
  tft.drawCircle(tempX + 40, 80, 2, ST77XX_WHITE); // °

  // Mínima / Máxima
  tft.setTextSize(2);
  int minmaxX = (128 - 72) / 2;
  tft.setCursor(minmaxX, 105);
  tft.print("23 / 15");

  // Hora
  int timeX = (128 - 60) / 2;
  tft.setCursor(timeX, 130);
  tft.print("19:00");

  frame++;
}

void drawModernStyle() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 3000) return;
  lastUpdate = millis();

  now = rtc.getTime();
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(5);
  tft.setCursor(10, 10);
  if (now.hour < 10) tft.print("0");
  tft.print(now.hour);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(10, 60);
  if (now.min < 10) tft.print("0");
  tft.print(now.min);

  tft.fillRoundRect(85, 60, 35, 25, 5, ST77XX_YELLOW);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(92, 65);
  tft.print(now.date);

  tft.fillCircle(15, 110, 6, ST77XX_WHITE);
  tft.fillCircle(20, 105, 4, ST77XX_WHITE);
  tft.fillCircle(26, 103, 3, ST77XX_YELLOW);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(40, 105);
  tft.print((int)rtc.getTemp());
  tft.fillCircle(64, 108, 2, ST77XX_YELLOW);
}

void drawAnalogClock() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();

  now = rtc.getTime();
  tft.fillScreen(ST77XX_BLACK);

  const int centerX = 64;
  const int centerY = 64;
  const int radius = 60;

  tft.drawCircle(centerX, centerY, radius, ST77XX_WHITE);
  tft.drawCircle(centerX, centerY, radius - 1, ST77XX_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  for (int i = 1; i <= 12; i++) {
    float angle = (i - 3) * 30 * DEG_TO_RAD;
    int tx = centerX + cos(angle) * (radius - 10);
    int ty = centerY + sin(angle) * (radius - 10);
    tft.setCursor(tx - (i < 10 ? 3 : 6), ty - 3);
    tft.print(i);
  }

  float angleHour = ((now.hour % 12) + now.min / 60.0) * 30 * DEG_TO_RAD;
  int hx = centerX + cos(angleHour) * (radius - 30);
  int hy = centerY + sin(angleHour) * (radius - 30);
  tft.drawLine(centerX, centerY, hx, hy, ST77XX_WHITE);

  float angleMin = now.min * 6 * DEG_TO_RAD;
  int mx = centerX + cos(angleMin) * (radius - 15);
  int my = centerY + sin(angleMin) * (radius - 15);
  tft.drawLine(centerX, centerY, mx, my, ST77XX_YELLOW);

  float angleSec = now.sec * 6 * DEG_TO_RAD;
  int sx = centerX + cos(angleSec) * (radius - 5);
  int sy = centerY + sin(angleSec) * (radius - 5);
  tft.drawLine(centerX, centerY, sx, sy, ST77XX_RED);

  tft.fillCircle(centerX, centerY, 3, ST77XX_WHITE);
}

void drawStopwatch() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 900) return;
  lastUpdate = millis();

  tft.fillScreen(ST77XX_BLACK);

  unsigned long elapsed = stopwatchElapsed;
  if (stopwatchRunning) {
    elapsed += millis() - stopwatchStart;
  }

  unsigned long seconds = (elapsed / 1000) % 60;
  unsigned long minutes = (elapsed / 60000) % 60;
  unsigned long millisec = (elapsed % 1000) / 10;

  #define DARKGREY 0x5294

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(1, 10);
  tft.print("Cronometro");

  tft.fillRoundRect(8, 35, 112, 88, 10, DARKGREY);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(20, 60);
  if (minutes < 10) tft.print("0");
  tft.print(minutes);
  tft.print(":");
  if (seconds < 10) tft.print("0");
  tft.print(seconds);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(30, 80);
  if (millisec < 10) tft.print("0");
  tft.print(millisec);
  tft.print(" ms");

  tft.fillCircle(100, 75, 5, stopwatchRunning ? ST77XX_GREEN : ST77XX_RED);
}

void toggleStopwatch() {
  if (stopwatchRunning) {
    stopwatchElapsed += millis() - stopwatchStart;
    stopwatchRunning = false;
  } else {
    stopwatchStart = millis();
    stopwatchRunning = true;
  }
}

void resetStopwatch() {
  stopwatchRunning = false;
  stopwatchElapsed = 0;
}

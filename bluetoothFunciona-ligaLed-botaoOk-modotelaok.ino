// Programa : Módulo Arduino Bluetooth HC-05 - Controle de LED com botão e display TFT + relógio moderno

#include <Wire.h>
#include <DS3231.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SoftwareSerial.h>

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
bool ledState = false;
bool lastButtonState = HIGH;

enum Mode { MODE_IDLE, MODE_MODERN };
Mode currentMode = MODE_IDLE;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  Serial.println("Envie '1' para LIGAR, '0' para DESLIGAR o LED, '2' para mostrar 'oi', ou '4' para modo relógio moderno.");

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);

  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(0);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  rtc.begin();
}

void loop() {
  // --- Comando via Bluetooth ---
  if (mySerial.available()) {
    command = mySerial.read();

    Serial.print("Comando recebido via Bluetooth: ");
    Serial.println(command);

    if (command == '1') {
      ledState = true;
      digitalWrite(ledPin, ledState);
      mySerial.println("LED ligado");
    } else if (command == '0') {
      ledState = false;
      digitalWrite(ledPin, ledState);
      mySerial.println("LED desligado");
    } else if (command == '2') {
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(30, 60);
      tft.print("oi");
      mySerial.println("Mensagem 'oi' exibida na tela");
      currentMode = MODE_IDLE;
    } else if (command == '4') {
      tft.fillScreen(ST77XX_BLACK);
      currentMode = MODE_MODERN;
      mySerial.println("Modo relógio moderno ativado");
    }
  }

  // --- Controle via botão físico ---
  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50); // debounce
    if (digitalRead(buttonPin) == LOW) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      Serial.println("Botão pressionado - LED " + String(ledState ? "LIGADO" : "DESLIGADO"));
      mySerial.println(ledState ? "LED ligado (botão)" : "LED desligado (botão)");
    }
  }
  lastButtonState = buttonState;

  // --- Comandos AT pelo monitor serial ---
  if (Serial.available()) {
    delay(10);
    mySerial.write(Serial.read());
  }

  // --- Modo relógio moderno ---
  if (currentMode == MODE_MODERN) {
    drawModernStyle();
  }
}

void drawModernStyle() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();

  now = rtc.getTime();
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(5);
  tft.setCursor(10, 10);
  if (now.hour < 10) tft.print("0");
  tft.print(now.hour);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(5);
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

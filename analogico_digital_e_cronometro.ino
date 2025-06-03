// ==== Bibliote
cas ====
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include <DS3231.h>
#include <SoftwareSerial.h> //Biblioteca bluetootth

// ==== Bluetooth=== 
const int pinoRx = //NUMERO DA ENTRADA RX do blue
const int pinoTx = // //    //    //  TX do blue
SoftwareSerial conexao(pinoRx, pinoTx)

// ==== Display ====
#define TFT_CS     10
#define TFT_RST    8
#define TFT_DC     9

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ==== RTC ====
DS3231 rtc(SDA, SCL);
Time agora;

// ==== Relógio Analógico ====
const int centerX = 64;
const int centerY = 64;
const int clockRadius = 60;

// ==== Botão ====
const int buttonPin = 12;
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
bool buttonHeld = false;

// ==== Modos ====
enum Mode { ANALOG, DIGITAL, TIMER };
Mode currentMode = ANALOG;

// ==== Controle ponteiros ====
int prev_sec = -1;
int prev_min = -1;
int prev_hour = -1;

// ==== Cronômetro ====
bool timerRunning = false;
unsigned long timerStart = 0;
unsigned long timerElapsed = 0;
bool timerStopped = false;

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  rtc.begin();
  
  pinMode(buttonPin, INPUT_PULLUP);

  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
// =============== Conexão Bluetooth ===============
conexao.begin(9600);
Serial.println("Bluetooth iniciado");

  drawClockFace();
}

// ================= LOOP =================
void loop() {
  handleButton();
  //Para verificar se há conexão blue
  if(conexao.available()) {
    char opcaoTela= conexao.read(); //le o caracter que o python enviar
    Serial.print("Tela escolhida:"); 
    Serial.println(opcaoTela); 

    switch (opcaoTela) { //Seleciona os modos do relogio
      case '1':
        if(currentMode != ANALOG) {
          currentMode= ANALOG;
          tft.fillScreen(ST77XX_BLACK);
          prev_sec = -1;
          conexao.println("Modo: Analógico");
        }
        break;
      case '2':
      if(currentMode != DIGITAL) {
        currentMode = DIGITAL;
        tft.fillScreen(ST77XX_BLACK);
        conexao.println("Modo: Digital");
      }
      break;
      case '3':
      if(currentMode != TIMER){
        currentMode= TIMER;
        tft.fillScreen(ST77XX_BLACK);
         timerRunning = false;
        timerElapsed = 0;
        timerStopped = false;
        conexao.println("Modo: Cronômetro");
      }
      break;
      default:
        conexao.println("Opção invalida, escolha uma tela existente");
        break;
    }
  }
  
  agora = rtc.getTime();
  int hour = agora.hour % 12;
  int minute = agora.min;
  int second = agora.sec;

  if (currentMode == ANALOG) {
    if (second != prev_sec) {
      drawHands(prev_hour, prev_min, prev_sec, true);
      drawHands(hour, minute, second, false);
      prev_sec = second;
      prev_min = minute;
      prev_hour = hour;
    }
  } else if (currentMode == DIGITAL) {
    drawDigitalClock();
  } else if (currentMode == TIMER) {
    drawTimer();
  }

  delay(100);
}

// ================= Funções =================

// ==== Controle do botão ====
void handleButton() {
  bool currentButtonState = digitalRead(buttonPin);

  // Pressionado
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPressTime = millis();
    buttonHeld = false;
  }

  // Segurando
  if (lastButtonState == LOW && currentButtonState == LOW) {
    if (!buttonHeld && millis() - buttonPressTime > 1500) {
      buttonHeld = true;

      if (currentMode == TIMER) {
        if (!timerRunning && !timerStopped) {
          timerRunning = true;
          timerStart = millis() - timerElapsed;
        } else if (timerRunning) {
          timerRunning = false;
          timerElapsed = millis() - timerStart;
          timerStopped = true;
        } else if (!timerRunning && timerStopped) {
          timerElapsed = 0;
          timerStopped = false;
        }
      }
    }
  }

  // Soltou
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    if (!buttonHeld) {
      currentMode = static_cast<Mode>((currentMode + 1) % 3);
      tft.fillScreen(ST77XX_BLACK);

      if (currentMode == ANALOG) {
        drawClockFace();
      } else if (currentMode == TIMER) {
        timerRunning = false;
        timerElapsed = 0;
        timerStopped = false;
      }
    }
  }

  lastButtonState = currentButtonState;
}

// ==== Relógio Analógico ====
void drawClockFace() {
  tft.fillScreen(ST77XX_BLACK);

  tft.drawCircle(centerX, centerY, clockRadius, ST77XX_WHITE);
  tft.drawCircle(centerX, centerY, clockRadius - 1, ST77XX_WHITE);

  for (int i = 0; i < 12; i++) {
    float angle = i * 30 * PI / 180;
    int xStart = centerX + (clockRadius - 8) * sin(angle);
    int yStart = centerY - (clockRadius - 8) * cos(angle);
    int xEnd = centerX + clockRadius * sin(angle);
    int yEnd = centerY - clockRadius * cos(angle);
    tft.drawLine(xStart, yStart, xEnd, yEnd, ST77XX_WHITE);
  }

  tft.fillCircle(centerX, centerY, 3, ST77XX_WHITE);
}

void drawHands(int hour, int minute, int second, bool erase) {
  uint16_t colorHour = erase ? ST77XX_BLACK : ST77XX_WHITE;
  uint16_t colorMinute = erase ? ST77XX_BLACK : ST77XX_BLUE;
  uint16_t colorSecond = erase ? ST77XX_BLACK : ST77XX_RED;

  // Hora
  float angleH = ((hour % 12) + minute / 60.0) * 30 * PI / 180;
  int xH = centerX + (clockRadius - 25) * sin(angleH);
  int yH = centerY - (clockRadius - 25) * cos(angleH);
  tft.drawLine(centerX, centerY, xH, yH, colorHour);

  // Minuto
  float angleM = minute * 6 * PI / 180;
  int xM = centerX + (clockRadius - 15) * sin(angleM);
  int yM = centerY - (clockRadius - 15) * cos(angleM);
  tft.drawLine(centerX, centerY, xM, yM, colorMinute);

  // Segundo
  float angleS = second * 6 * PI / 180;
  int xS = centerX + (clockRadius - 10) * sin(angleS);
  int yS = centerY - (clockRadius - 10) * cos(angleS);
  tft.drawLine(centerX, centerY, xS, yS, colorSecond);
}

// ==== Relógio Digital ====
void drawDigitalClock() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();

    tft.fillScreen(ST77XX_BLACK);

    // Data
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(1);
    tft.setCursor(10, 10);
    tft.print(rtc.getDOWStr());
    tft.print(", ");
    tft.print(rtc.getDateStr());

    // Temperatura
    tft.setCursor(10, 25);
    tft.print("Temp: ");
    tft.print(rtc.getTemp());
    tft.print(" C");

    // Hora
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(3);
    tft.setCursor(10, 60);
    tft.print(rtc.getTimeStr());
  }
}

// ==== Cronômetro ====
void drawTimer() {
  unsigned long current = millis();
  if (timerRunning) {
    timerElapsed = current - timerStart;
  }

  unsigned long totalSeconds = timerElapsed / 1000;
  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;
  int seconds = totalSeconds % 60;

  static unsigned long lastUpdate = 0;
  if (current - lastUpdate > 500) {
    lastUpdate = current;

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("CRONOMETRO");

    tft.setTextSize(3);
    tft.setCursor(10, 50);
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    tft.print(buffer);

    // Estado
    tft.setTextSize(1);
    tft.setCursor(10, 90);
    if (timerRunning) tft.print("Rodando...");
    else if (timerStopped) tft.print("Pausado");
    else tft.print("Zerado");
  }
}

// Programa : Módulo Arduino Bluetooth HC-05 - Controle de LED com botão e display TFT
// Autor : Adaptado de MakerHero

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

// ====== Variáveis de Controle ======
char command = 0;
bool ledState = false;
bool lastButtonState = HIGH;

void setup() {
  // Inicialização de seriais
  Serial.begin(115200);
  mySerial.begin(9600);

  Serial.println("Envie '1' para LIGAR, '0' para DESLIGAR o LED, ou '2' para mostrar 'oi' na tela.");

  // Configurações de pinos
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);

  // Inicializa o display TFT
  tft.initR(INITR_BLACKTAB); // Para displays ST7735 padrão
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1); // Ajuste se necessário
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
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
      // Mostra "oi" na tela TFT
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(30, 60);
      tft.print("oi");
      mySerial.println("Mensagem 'oi' exibida na tela");
    }
  }

  // --- Controle via botão físico ---
  bool buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50); // debounce simples
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
}

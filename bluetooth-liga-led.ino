// Programa : Módulo Arduino Bluetooth HC-05 - Controle de LED
// Autor : Adaptado de MakerHero

#include <SoftwareSerial.h>

// Define os pinos para o HC-05
SoftwareSerial mySerial(9, 8); // RX, TX

// Define o pino do LED
#define ledPin 2
char command = 0;

void setup() {
  // Inicia a serial do monitor
  Serial.begin(115200);
  Serial.println("Digite os comandos AT ou envie '1' para LIGAR e '0' para DESLIGAR o LED:");

  // Inicia a comunicação com o módulo Bluetooth
  mySerial.begin(9600);

  // Configura o pino do LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  // Verifica se há dados vindos do Bluetooth
  if (mySerial.available()) {
    command = mySerial.read();  // Lê o caractere recebido

    Serial.print("Comando recebido via Bluetooth: ");
    Serial.println(command);

    if (command == '1') {
      digitalWrite(ledPin, HIGH);  // Liga o LED
      mySerial.println("LED ligado");
    }
    else if (command == '0') {
      digitalWrite(ledPin, LOW);   // Desliga o LED
      mySerial.println("LED desligado");
    }
  }

  // Permite o envio de comandos AT pelo monitor serial
  if (Serial.available()) {
    delay(10);
    mySerial.write(Serial.read());
  }
}
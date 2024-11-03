#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <images.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define SCREEN_ADDRESS 0x3C
#define I2C_FREQUENCY 500000
#define BUTTON_PIN 0 // GPIO do botão PROG

int digitalPins[7] = {1, 2, 3, 4, 5, 6, 7};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

bool outputStatus[7] = {false, false, false, false, false, false, false};

int currentScreen = 0;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 200;
unsigned long lastToggleTime = 0;
unsigned long toggleInterval = 500;
int relayIndex = 0;

void drawRelayStatusScreen()
{
  display.clearDisplay();

  display.setCursor(26, 3);
  display.print("Relay Status:");
  display.drawLine(2, 13, 125, 13, 1);

  display.drawBitmap(1, 27, Table, 127, 36, 1);
  display.drawBitmap(4, 33, OffArrayLabel, 120, 5, 1);

  for (int i = 0; i < 7; i++)
  {
    if (outputStatus[i])
    {
      display.drawBitmap(i * 18 + 3, 29, TableCell, 15, 14, 1);
      display.drawBitmap(i * 18 + 6, 34, OnLabel, 9, 4, 0);
    }
  }
  display.display();
}

void drawWiFiStatusScreen()
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(26, 3);
  display.print("WiFi Status:");
  display.drawLine(2, 13, 125, 13, 1);

  if (WiFi.status() == WL_CONNECTED)
  {
    display.setCursor(0, 25);
    display.print("SSID: ");
    display.print(WiFi.SSID());

    display.setCursor(0, 35);
    display.print("IP: ");
    display.print(WiFi.localIP());

    display.setCursor(0, 45);
    display.print("Gateway: ");
    display.print(WiFi.gatewayIP());

    display.setCursor(0, 55);
    display.print("MAC:");
    display.print(WiFi.macAddress());
  }
  else
  {
    display.setCursor(10, 30);
    display.print("Nao conectado");
  }

  display.display();
}

void drawESPInfoScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.setCursor(26, 3);
  display.print("ESP32 Info:");
  display.drawLine(2, 13, 125, 13, 1);

  // ID do chip
  display.setCursor(0, 18);
  display.print("Chip ID: ");
  display.print((uint32_t)ESP.getEfuseMac(), HEX);

  // Número de núcleos da CPU
  display.setCursor(0, 28);
  display.print("Cores: ");
  display.print(ESP.getChipCores());

  // Frequência da CPU
  display.setCursor(0, 38);
  display.print("CPU Freq: ");
  display.print(ESP.getCpuFreqMHz());
  display.print(" MHz");

  // Flash Size
  display.setCursor(0, 48);
  display.print("Flash: ");
  display.print(ESP.getFlashChipSize() / (1024 * 1024));
  display.print(" MB");

  display.display();
}


void handleButtonPress()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    if (millis() - lastDebounceTime > debounceDelay)
    {
      lastDebounceTime = millis();
      currentScreen = (currentScreen + 1) % 3;
      if (currentScreen == 0)
      {
        drawRelayStatusScreen();
      }
      else if (currentScreen == 1)
      {
        drawWiFiStatusScreen();
      }
      else{
        drawESPInfoScreen();
      }
    }
  }
}

// Função para alternar os relés com controle de tempo
void toggleRelay()
{
  if (millis() - lastToggleTime >= toggleInterval)
  {
    lastToggleTime = millis();

    // Alternar saída atual
    outputStatus[relayIndex] = !outputStatus[relayIndex];
    digitalWrite(digitalPins[relayIndex], outputStatus[relayIndex]);

    if (relayIndex > 0)
    {
      outputStatus[relayIndex - 1] = !outputStatus[relayIndex - 1];
      digitalWrite(digitalPins[relayIndex - 1], outputStatus[relayIndex - 1]);
    }
    else if (outputStatus[6] == true){
      outputStatus[6] = !outputStatus[6];
      digitalWrite(digitalPins[6], outputStatus[6]);
    }

    // Atualizar a tela de status dos relés
    if (currentScreen == 0)
    {
      drawRelayStatusScreen();
    }

    // Próxima saída
    relayIndex = (relayIndex + 1) % 7;
  }
}

void setup()
{
  Serial.begin(9600);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(I2C_FREQUENCY);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("Falha na inicializacao do display SSD1306"));
    for (;;)
      ;
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  display.display();
  delay(2000);
  display.clearDisplay();

  for (int i = 0; i < 7; i++)
  {
    pinMode(digitalPins[i], OUTPUT);
    digitalWrite(digitalPins[i], LOW);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP32-Setup"); // Nome do Access Point para configuração

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Conectado!");
    drawRelayStatusScreen();
  }
}

void loop()
{
  handleButtonPress();
  toggleRelay();
}

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <images.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define SCREEN_ADDRESS 0x3C
#define I2C_FREQUENCY 500000
#define BUTTON_PIN 0 // GPIO do botão PROG
#define LOG_LINES 8

const int RELAY_COUNT = 7;
const int digitalPins[RELAY_COUNT] = {1, 2, 3, 4, 5, 6, 7};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
WebServer server(80);
WiFiManager wifiManager;

bool outputStatus[RELAY_COUNT] = {false};
int currentScreen = 0;
unsigned long debounceDelay = 50;
unsigned long buttonPressStart = 0;
bool buttonLongPressActive = false;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
String logBuffer[LOG_LINES];
int logIndex = 0;

void addLog(const String &message)
{
  logBuffer[logIndex] = message;
  logIndex = (logIndex + 1) % LOG_LINES;
}

void logMessage(const String &message)
{
  Serial.println(message);
  addLog(message);
}

void resetWiFiConfig()
{
  logMessage("Configurações Wi-Fi resetadas!");
  wifiManager.resetSettings();
  ESP.restart();
}

void batchRelayToggle()
{
  int activeRelays = 0;
  bool toggleSignal = false;

  for (int index = 0; index < RELAY_COUNT; index++)
  {
    if (outputStatus[index] == true)
    {
      activeRelays++;
    }
  }

  if (activeRelays < 3)
  {
    toggleSignal = true;
  }

  for (int index = 0; index < RELAY_COUNT; index++)
  {
    outputStatus[index] = toggleSignal;
    digitalWrite(digitalPins[index], outputStatus[index]);
  }

  logMessage("Relays Toggled to: " + String(toggleSignal));
}

void handleShortPressAction()
{
  portENTER_CRITICAL(&mux);
  currentScreen = (currentScreen + 1) % 4;
  portEXIT_CRITICAL(&mux);
}

void handleLongPressAction()
{
  switch (currentScreen)
  {
  case 0:
    batchRelayToggle();
    break;
  case 1:
    resetWiFiConfig();
    break;
  }
  
  buttonLongPressActive = true;
}

void IRAM_ATTR handleButtonInterrupt()
{
  unsigned long currentMillis = millis();

  if (digitalRead(BUTTON_PIN) == LOW)
  { // Botão pressionado
    buttonPressStart = currentMillis;
    buttonLongPressActive = false;
  }
  else
  { // Botão liberado
    unsigned long pressDuration = currentMillis - buttonPressStart;

    if (pressDuration >= 2000)
      buttonLongPressActive = true; // Marcar que pressionamento longo ocorreu

    else if (pressDuration >= debounceDelay && !buttonLongPressActive)
      handleShortPressAction();
  }
}

void checkLongPressAction()
{
  if (!buttonLongPressActive && digitalRead(BUTTON_PIN) == LOW)
  {
    unsigned long pressDuration = millis() - buttonPressStart;
    if (pressDuration >= 2000)
      handleLongPressAction();
  }
}

void drawRelayStatusScreen()
{
  display.clearDisplay();
  display.setCursor(26, 3);
  display.print("Relay Status:");
  display.drawLine(2, 13, 125, 13, 1);
  display.drawBitmap(1, 27, Table, 127, 36, 1);
  display.drawBitmap(4, 33, OffArrayLabel, 120, 5, 1);

  for (int i = 0; i < RELAY_COUNT; i++)
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
  display.setCursor(26, 3);
  display.print("ESP32 Info:");
  display.drawLine(2, 13, 125, 13, 1);

  display.setCursor(0, 25);
  display.print("Chip ID: ");
  display.print((uint32_t)ESP.getEfuseMac(), HEX);

  display.setCursor(0, 35);
  display.print("Cores: ");
  display.print(ESP.getChipCores());

  display.setCursor(0, 45);
  display.print("CPU Freq: ");
  display.print(ESP.getCpuFreqMHz());
  display.print(" MHz");

  display.setCursor(0, 55);
  display.print("Flash: ");
  display.print(ESP.getFlashChipSize() / (1024 * 1024));
  display.print(" MB");

  display.display();
}

void drawLogScreen()
{
  display.clearDisplay();
  display.setCursor(50, 3);
  display.print("Logs:");
  display.drawLine(2, 13, 125, 13, 1);

  int y = 15;
  int startIndex = (logIndex + LOG_LINES - 6) % LOG_LINES; // Começar pelos últimos 7 logs

  for (int i = 0; i < 6; i++) // Mostrar 7 linhas, que cabem na tela sem sobreposição
  {
    int index = (startIndex + i) % LOG_LINES;
    display.setCursor(0, y);
    display.print(logBuffer[index]);
    y += 8;
  }

  display.display();
}

void handleGetRelayStatus()
{
  JsonDocument jsonDoc;
  for (int i = 0; i < RELAY_COUNT; i++)
  {
    jsonDoc["relay" + String(i)] = outputStatus[i];
  }
  String json;
  serializeJson(jsonDoc, json);
  server.send(200, "application/json", json);
}

void serveFile(const char *path, const char *contentType)
{
  File file = LittleFS.open(path, "r");
  if (!file)
  {
    server.send(404, "text/plain", "Arquivo não encontrado");
    return;
  }
  server.streamFile(file, contentType);
  file.close();
}

void handleRelayControl()
{
  if (server.hasArg("relay") && server.hasArg("state"))
  {
    int relayNum = server.arg("relay").toInt();
    String state = server.arg("state");

    if (relayNum >= 0 && relayNum < RELAY_COUNT && (state == "on" || state == "off"))
    {
      outputStatus[relayNum] = (state == "on");
      digitalWrite(digitalPins[relayNum], outputStatus[relayNum]);
      logMessage("Relay " + String(relayNum + 1) + ": " + state);

      server.send(200, "text/plain", "OK");
    }
    else
    {
      server.send(400, "text/plain", "Número de relé inválido ou estado inválido.");
    }
  }
  else
  {
    server.send(400, "text/plain", "Parâmetros 'relay' e 'state' são obrigatórios.");
  }
}

void setupServer()
{
  server.on("/", []()
            { serveFile("/index.html", "text/html"); });
  server.on("/style.css", []()
            { serveFile("/style.css", "text/css"); });
  server.on("/script.js", []()
            { serveFile("/script.js", "application/javascript"); });
  server.on("/relay", handleRelayControl);
  server.on("/status", handleGetRelayStatus);
  server.begin();
}

void taskDisplayAndButton(void *pvParameters)
{
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  for (;;)
  {
    switch (currentScreen)
    {
    case 0:
      drawRelayStatusScreen();
      break;
    case 1:
      drawWiFiStatusScreen();
      break;
    case 2:
      drawESPInfoScreen();
      break;
    case 3:
      drawLogScreen();
      break;
    }
    checkLongPressAction(); // Checa se o pressionamento longo foi atingido
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void taskServer(void *pvParameters)
{
  for (;;)
  {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(9600);

  if (!LittleFS.begin())
  {
    logMessage("Falha ao montar o LittleFS");
    ESP.restart();
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(I2C_FREQUENCY);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    logMessage("Falha na inicialização do display SSD1306");
    ESP.restart();
  }

  display.display();
  delay(2000);

  for (int i = 0; i < RELAY_COUNT; i++)
  {
    pinMode(digitalPins[i], OUTPUT);
    digitalWrite(digitalPins[i], LOW);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, CHANGE);

  wifiManager.autoConnect("ESP32-Setup");

  if (WiFi.status() == WL_CONNECTED)
  {
    logMessage("Conectado!");
  }

  setupServer();

  xTaskCreatePinnedToCore(taskDisplayAndButton, "DisplayAndButton", 4096, NULL, 1, NULL, 1); // Núcleo 1
  xTaskCreatePinnedToCore(taskServer, "Server", 4096, NULL, 1, NULL, 0);                     // Núcleo 0
}

void loop()
{
  // Loop vazio, pois as tarefas estão distribuídas nos núcleos
}

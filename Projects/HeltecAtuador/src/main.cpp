#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_SDA 17      // SDA pin
#define OLED_SCL 18      // SCL pin
#define OLED_RST 21      // Reset pin (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define I2C_FREQUENCY 500000 // I2C frequency

// Pinos GPIO para as saídas digitais (modificar conforme necessário)
int digitalPins[7] = {1, 2, 3, 4, 5, 6, 7}; 

// Inicializar o display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Variável para armazenar o status das saídas
bool outputStatus[7] = {false, false, false, false, false, false, false}; 

// Função para desenhar a tabela com o status das saídas
void drawTable() {
  display.clearDisplay();
  int cellWidth = SCREEN_WIDTH / 7; // Largura de cada célula
  int cellHeight = SCREEN_HEIGHT / 3; // Altura de cada célula (metade da altura do display)

  display.setCursor(0,0);
  display.print("Relay Status:");

  // Desenhar linhas da tabela
  for (int i = 0; i <= 7; i++) {
    display.drawLine(i * cellWidth, SCREEN_HEIGHT / 3, i * cellWidth, SCREEN_HEIGHT, SSD1306_WHITE);
  }

  // Desenhar linha horizontal para a divisão das linhas
  display.drawLine(0, cellHeight * 2, SCREEN_WIDTH - 2, cellHeight * 2, SSD1306_WHITE);

  // Desenhar borda superior
  display.drawLine(0, SCREEN_HEIGHT / 3, SCREEN_WIDTH - 2, SCREEN_HEIGHT / 3, SSD1306_WHITE);
  // Desenhar borda inferior
  display.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 1, SSD1306_WHITE);

  // Atualizar o status das saídas no display
  for (int i = 0; i < 7; i++) {
    if (outputStatus[i]) {
      display.fillRect(i * cellWidth + 3, cellHeight + 3, cellWidth - 5, (cellHeight) - 5, SSD1306_WHITE);
    }
  }

  // Adicionar números nas células da linha inferior
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 7; i++) {
    int number = i + 1;
    String text = String(number);
    int textWidth = text.length() * 6;
    int textHeight = 8;
    int x = (i * cellWidth) + (cellWidth - textWidth) / 2;
    int y = (cellHeight * 2  + (cellHeight - textHeight) / 2);
    display.setCursor(x + 1, y);
    display.print(text);
  }

  // Exibir o buffer
  display.display();
}

// Método privado para alterar o nível lógico de uma saída e atualizar o display
void setOutput(int outputNumber, bool level) {
  // Definir o nível lógico da saída
  digitalWrite(digitalPins[outputNumber], level);

  // Atualizar o status da saída correspondente
  outputStatus[outputNumber] = level;

  // Atualizar a tabela no display
  drawTable();
}

void setup() {
  Serial.begin(9600);

  // Configurar comunicação I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(I2C_FREQUENCY);

  // Inicializar o display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Falha na inicialização do display SSD1306"));
    for (;;); // Trava o loop em caso de falha
  }

  // Exibir o logo inicial (splash screen)
  display.display();
  delay(2000); // Pausa de 2 segundos

  // Limpar o buffer do display
  display.clearDisplay();

  // Configurar pinos GPIO como saídas digitais
  for (int i = 0; i < 7; i++) {
    pinMode(digitalPins[i], OUTPUT);
    digitalWrite(digitalPins[i], LOW); // Inicialmente desligar as saídas
  }

  pinMode(35,OUTPUT);
  digitalWrite(35,HIGH);
  delay(100);
  digitalWrite(35,LOW);

  // Desenhar a tabela inicial
  drawTable();
}

void loop() {
  // Alternar as saídas digitalmente
  for (int i = 5; i < 7; i++) {
    setOutput(i, true);  // Ligar a saída atual
    delay(250);         // Pausar por 1 segundo
    setOutput(i, false); // Desligar a saída atual
    delay(250);          // Pausar por 0,5 segundos
  }
}

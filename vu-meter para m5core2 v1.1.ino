#include <M5Core2.h>
#include <driver/i2s.h>

// --- DEFINIÇÃO DOS TEMAS ---
enum Tema { PRINCIPAL, NEON, CLARO };
Tema temaAtual = PRINCIPAL; // Agora a sua interface da foto é a inicial

// Cores Gerais
uint16_t corFundo, corEscala, corAgulha, corTexto, corArco;

// --- PARÂMETROS TÉCNICOS ---
float suavizacao = 0.01;  
float ganho = 25.0;       
float valorSuavizado = 0;
float nivelDC = 0;        
int cx = 160; int cy = 210; int r = 140;
int old_x, old_y;

void aplicarTema() {
  switch (temaAtual) {
    case PRINCIPAL:
      corFundo = 0x18E3;  // Azul escuro/acinzentado da sua foto
      corEscala = WHITE;
      corAgulha = WHITE;
      corTexto = WHITE;
      corArco = 0x07FF;   // Ciano para o arco
      break;
    case NEON:
      corFundo = BLACK;
      corEscala = 0x07FF; // Cyan
      corAgulha = 0xF81F; // Magenta
      corTexto = 0xFFE0; // Amarelo
      corArco = 0x07FF;
      break;
    case CLARO:
      corFundo = WHITE;
      corEscala = BLUE;
      corAgulha = BLACK;
      corTexto = BLACK;
      corArco = BLUE;
      break;
  }
  M5.Lcd.fillScreen(corFundo);
}

void desenhaEscala() {
  // Desenha o Arco
  for (int a = 150; a >= 30; a--) {
    float rad = a * 3.14159 / 180.0;
    float rad2 = (a - 1) * 3.14159 / 180.0;
    int x = cx + r * cos(rad + 3.14159);
    int y = cy + r * sin(rad + 3.14159);
    int x_next = cx + r * cos(rad2 + 3.14159);
    int y_next = cy + r * sin(rad2 + 3.14159);
    M5.Lcd.drawLine(x, y, x_next, y_next, corArco);
  }

  // Ticks e Números
  for (int i = 0; i <= 10; i++) {
    float ang = map(i, 0, 10, 150, 30);
    float rad = ang * 3.14159 / 180.0;
    int x1 = cx + r * cos(rad + 3.14159);
    int y1 = cy + r * sin(rad + 3.14159);
    int x2 = cx + (r - 12) * cos(rad + 3.14159);
    int y2 = cy + (r - 12) * sin(rad + 3.14159);
    
    // Mantém o alerta vermelho nos extremos
    uint16_t corTick = (i > 8 || i < 2) ? RED : corEscala;
    M5.Lcd.drawLine(x1, y1, x2, y2, corTick);

    if (i % 2 == 0) {
      int x_num = cx + (r - 35) * cos(rad + 3.14159);
      int y_num = cy + (r - 35) * sin(rad + 3.14159);
      M5.Lcd.setTextColor(corTexto, corFundo);
      M5.Lcd.setTextDatum(MC_DATUM);
      M5.Lcd.drawString(String(i * 10), x_num, y_num);
    }
  }
}

void setup() {
  M5.begin();
  aplicarTema();
  desenhaEscala();
  
  // Configuração I2S PDM (Microfone interno)
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
  };
  i2s_pin_config_t pin_config = {.bck_io_num = I2S_PIN_NO_CHANGE, .ws_io_num = 0, .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = 34};
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
  M5.update();

  // Troca de Temas nos Botões Físicos
  if (M5.BtnA.wasPressed()) { temaAtual = PRINCIPAL; aplicarTema(); desenhaEscala(); }
  if (M5.BtnB.wasPressed()) { temaAtual = NEON;      aplicarTema(); desenhaEscala(); }
  if (M5.BtnC.wasPressed()) { temaAtual = CLARO;     aplicarTema(); desenhaEscala(); }

  // Captura e Processamento de Áudio
  size_t bytesread;
  int16_t read_buff[64];
  i2s_read(I2S_NUM_0, (char *)read_buff, 64 * 2, &bytesread, portMAX_DELAY);
  
  float soma = 0;
  for (int i = 0; i < 64; i++) soma += read_buff[i];
  float mediaImediata = soma / 64.0;
  nivelDC = (mediaImediata * 0.02) + (nivelDC * 0.98); 
  float sinalLimpo = (mediaImediata - nivelDC) * ganho;
  valorSuavizado = (sinalLimpo * suavizacao) + (valorSuavizado * (1.0 - suavizacao));

  float angulo = map(valorSuavizado, -800, 800, 150, 30); 
  angulo = constrain(angulo, 30, 150);

  // Monitoramento de Bateria
  float batV = M5.Axp.GetBatVoltage();
  bool carregando = M5.Axp.GetBatCurrent() > 0;
  int batPct = (batV < 3.2) ? 0 : (batV - 3.2) * 100 / (4.2 - 3.2);
  if (batPct > 100) batPct = 100;
  
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(corTexto, corFundo);
  M5.Lcd.setCursor(210, 12);
  M5.Lcd.printf("%d%%%s", batPct, carregando ? " [CHG]" : "");
  
  M5.Lcd.drawRect(270, 10, 35, 15, corTexto);
  int barWidth = map(batPct, 0, 100, 0, 31);
  M5.Lcd.fillRect(272, 12, barWidth, 11, (carregando ? YELLOW : (batPct > 20 ? GREEN : RED)));

  // Movimento da Agulha
  float rad = angulo * 3.14159 / 180.0;
  int x_ponta = cx + r * cos(rad + 3.14159);
  int y_ponta = cy + r * sin(rad + 3.14159);

  if (x_ponta != old_x || y_ponta != old_y) {
    M5.Lcd.drawLine(cx, cy, old_x, old_y, corFundo);
    desenhaEscala(); 
    M5.Lcd.drawLine(cx, cy, x_ponta, y_ponta, corAgulha);
    old_x = x_ponta; old_y = y_ponta;
  }
}

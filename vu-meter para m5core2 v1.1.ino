#include <M5Core2.h>
#include <driver/i2s.h>

// --- PARÂMETROS DE CALIBRAÇÃO TÉCNICA ---
float suavizacao = 0.05;
float ganho = 1.85; //ajuste aqui para mecher no ganho da leitura      
float offsetMeio = 1048.9;// AJUSTE AQUI para a agulha ficar no meio no silêncio
float valorSuavizado = 0;

int cx = 160; int cy = 210; int r = 140;
int old_x, old_y;

void desenhaEscala() {
  M5.Lcd.drawCircleHelper(cx, cy, r, 1, BLUE);
  
  for (int i = 0; i <= 10; i++) {
    float ang = map(i, 0, 10, 150, 30);
    float rad = ang * 3.14159 / 180.0;
    
    int x1 = cx + r * cos(rad + 3.14159);
    int y1 = cy + r * sin(rad + 3.14159);
    int x2 = cx + (r-12) * cos(rad + 3.14159);
    int y2 = cy + (r-12) * sin(rad + 3.14159);
    
    uint16_t cor = (i > 7) ? RED : BLUE;
    M5.Lcd.drawLine(x1, y1, x2, y2, cor);

    // --- ADIÇÃO DE NÚMEROS NA ESCALA ---
    if (i % 2 == 0) { // Desenha números apenas nos traços pares para não embolar
      int x_num = cx + (r-25) * cos(rad + 3.14159);
      int y_num = cy + (r-25) * sin(rad + 3.14159);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(x_num - 5, y_num - 5);
      M5.Lcd.print(i * 10); 
    }
  }
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  desenhaEscala();
  
  // Configuração I2S (Microfone)
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
  
  size_t bytesread;
  int16_t read_buff[64];
  i2s_read(I2S_NUM_0, (char *)read_buff, 64 * 2, &bytesread, portMAX_DELAY);
  
  int32_t sum = 0;
  for (int i = 0; i < 64; i++) sum += abs(read_buff[i]);
  
  // AQUI É A CALIBRAÇÃO DO MEIO: somamos o offset fixo à leitura do mic
  float leituraAtual = (sum / 64.0) * ganho + offsetMeio;

  valorSuavizado = (leituraAtual * suavizacao) + (valorSuavizado * (1.0 - suavizacao));

  // Trava para não estourar a escala
  if (valorSuavizado > 2500) valorSuavizado = 2500; 

  // Ícone de Bateria
  float batV = M5.Axp.GetBatVoltage();
  int batPct = (batV < 3.2) ? 0 : (batV - 3.2) * 100 / (4.2 - 3.2);
  if (batPct > 100) batPct = 100;
  M5.Lcd.drawRect(260, 10, 40, 15, WHITE);
  M5.Lcd.fillRect(262, 12, map(batPct, 0, 100, 0, 36), 11, (batPct > 20) ? GREEN : RED);

  // Agulha
  float angulo = map(valorSuavizado, 0, 2500, 30, 150);
  angulo = constrain(angulo, 30, 150);

  float rad = angulo * 3.14159 / 180.0;
  int x_ponta = cx + r * cos(rad + 3.14159);
  int y_ponta = cy + r * sin(rad + 3.14159);

  M5.Lcd.drawLine(cx, cy, old_x, old_y, BLACK);
  desenhaEscala(); 
  M5.Lcd.drawLine(cx, cy, x_ponta, y_ponta, WHITE);

  old_x = x_ponta; old_y = y_ponta;
  delay(15);
}

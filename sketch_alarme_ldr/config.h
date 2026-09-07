#ifndef CONFIG_H
#define CONFIG_H

// ===== Pinagem =====
const uint8_t PINO_LDR          = A0; // Divisor de tensao: 5V -> R1 (10k) -> A0 -> LDR -> GND
const uint8_t PINO_BOTAO_ARM    = 12;  // Armar/Desarmar - segurar define o delay de saida
const uint8_t PINO_BOTAO_SIL    = 13;  // Silenciar / Reset
const uint8_t PINO_LED_VERDE    = 8;  // Indicador: sistema armado
const uint8_t PINO_LED_VERMELHO = 7;  // Indicador: alarme disparado
const uint8_t PINO_BUZZER       = 2;  // Buzzer passivo via transistor (tone())

// ===== OLED (I2C - SDA em A4, SCL em A5 no Uno) =====
const uint8_t OLED_LARGURA   = 128;
const uint8_t OLED_ALTURA    = 64;
const int8_t  OLED_RESET     = -1;
const uint8_t OLED_ENDERECO  = 0x3C;

// ===== EEPROM =====
const int ENDERECO_EEPROM_CONTADOR = 0; // 2 bytes (unsigned int)

// ===== Parametros do alarme =====
const unsigned long DELAY_MINIMO_MS = 3000UL;  // delay minimo de saida (toque rapido)
const unsigned long DELAY_MAXIMO_MS = 60000UL; // delay maximo de saida (segurando 60s+)
const float LIMIAR_VARIACAO         = 0.25;    // 25% de variacao em relacao a baseline dispara
const int   LIMIAR_MINIMO_ABSOLUTO  = 50;      // piso de sensibilidade (evita falso disparo com baseline baixa)
const unsigned long DEBOUNCE_MS     = 50UL;

#endif

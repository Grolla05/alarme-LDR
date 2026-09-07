// Projeto Alarme com LDR - maquina de estados com delay de saida configuravel
// (quanto mais tempo o botao de armar for segurado, maior o delay antes de armar)
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include "config.h"

Adafruit_SSD1306 display(OLED_LARGURA, OLED_ALTURA, &Wire, OLED_RESET);

enum EstadoAlarme {
  DESARMADO,
  CONFIGURANDO_DELAY,
  SAINDO,
  ARMADO,
  DISPARADO,
  SILENCIADO
};

EstadoAlarme estadoAtual = DESARMADO;

unsigned long inicioPressao   = 0;
unsigned long delayEscolhido  = DELAY_MINIMO_MS;
unsigned long inicioSaida     = 0;

int baselineLDR = 0;
unsigned int contadorEventos = 0;

// Debounce dos botoes (padrao classico: le bruto, so aceita apos DEBOUNCE_MS estavel)
int ultimaLeituraArmBruta = HIGH, estadoArmEstavel = HIGH;
unsigned long ultimoDebounceArm = 0;

int ultimaLeituraSilBruta = HIGH, estadoSilEstavel = HIGH;
unsigned long ultimoDebounceSil = 0;

unsigned long ultimoToggleBuzzer = 0;
bool tomAgudo = false;

int lerBotao(uint8_t pino, int &ultimaLeituraBruta, int &estadoEstavel, unsigned long &ultimoDebounce) {
  int leituraBruta = digitalRead(pino);
  if (leituraBruta != ultimaLeituraBruta) {
    ultimoDebounce = millis();
  }
  if ((millis() - ultimoDebounce) > DEBOUNCE_MS) {
    estadoEstavel = leituraBruta;
  }
  ultimaLeituraBruta = leituraBruta;
  return estadoEstavel;
}

void calibrarBaseline() {
  long soma = 0;
  const int amostras = 20;
  for (int i = 0; i < amostras; i++) {
    soma += analogRead(PINO_LDR);
    delay(10);
  }
  baselineLDR = soma / amostras;
}

bool detectarVariacaoLDR() {
  int leituraAtual = analogRead(PINO_LDR);
  int diferenca = abs(leituraAtual - baselineLDR);
  int limiar = max((int)(baselineLDR * LIMIAR_VARIACAO), LIMIAR_MINIMO_ABSOLUTO);
  return diferenca > limiar;
}

void registrarEvento() {
  contadorEventos++;
  EEPROM.put(ENDERECO_EEPROM_CONTADOR, contadorEventos);
}

void tocarSirene() {
  if (millis() - ultimoToggleBuzzer > 300) {
    ultimoToggleBuzzer = millis();
    tomAgudo = !tomAgudo;
    tone(PINO_BUZZER, tomAgudo ? 1800 : 1000);
  }
}

void atualizarDisplay() {
  static unsigned long ultimaAtualizacao = 0;
  if (millis() - ultimaAtualizacao < 200) return;
  ultimaAtualizacao = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  switch (estadoAtual) {
    case DESARMADO:
      display.println(F("SISTEMA DESARMADO"));
      display.println(F("Segure ARMAR para"));
      display.println(F("definir o delay"));
      break;

    case CONFIGURANDO_DELAY: {
      unsigned long segurando = (millis() - inicioPressao) / 1000;
      display.println(F("Configurando delay"));
      display.print(segurando);
      display.println(F("s (solte p/ armar)"));
      break;
    }

    case SAINDO: {
      unsigned long decorrido = millis() - inicioSaida;
      unsigned long restanteMs = (decorrido < delayEscolhido) ? (delayEscolhido - decorrido) : 0;
      display.println(F("SAINDO..."));
      display.print(F("Armando em "));
      display.print(restanteMs / 1000 + 1);
      display.println(F("s"));
      break;
    }

    case ARMADO:
      display.println(F("SISTEMA ARMADO"));
      display.print(F("LDR base: "));
      display.println(baselineLDR);
      break;

    case DISPARADO:
      display.println(F("!! ALARME !!"));
      display.print(F("Eventos: "));
      display.println(contadorEventos);
      break;

    case SILENCIADO:
      display.println(F("Alarme silenciado"));
      display.println(F("Aperte ARMAR p/"));
      display.println(F("reiniciar"));
      break;
  }
  display.display();
}

void setup() {
  Serial.begin(9600);

  pinMode(PINO_BOTAO_ARM, INPUT_PULLUP);
  pinMode(PINO_BOTAO_SIL, INPUT_PULLUP);
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  digitalWrite(PINO_LED_VERDE, LOW);
  digitalWrite(PINO_LED_VERMELHO, LOW);

  EEPROM.get(ENDERECO_EEPROM_CONTADOR, contadorEventos);
  if (contadorEventos == 0xFFFF) contadorEventos = 0; // EEPROM virgem (nunca gravada)

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ENDERECO)) {
    Serial.println(F("Falha ao iniciar OLED"));
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  int botaoArm = lerBotao(PINO_BOTAO_ARM, ultimaLeituraArmBruta, estadoArmEstavel, ultimoDebounceArm);
  int botaoSil = lerBotao(PINO_BOTAO_SIL, ultimaLeituraSilBruta, estadoSilEstavel, ultimoDebounceSil);

  switch (estadoAtual) {
    case DESARMADO:
      if (botaoArm == LOW) { // pressionado (INPUT_PULLUP)
        inicioPressao = millis();
        estadoAtual = CONFIGURANDO_DELAY;
      }
      break;

    case CONFIGURANDO_DELAY:
      if (botaoArm == HIGH) { // solto -> calcula o delay pelo tempo segurado
        unsigned long tempoSegurado = millis() - inicioPressao;
        delayEscolhido = constrain(tempoSegurado, DELAY_MINIMO_MS, DELAY_MAXIMO_MS);
        inicioSaida = millis();
        estadoAtual = SAINDO;
      }
      break;

    case SAINDO: {
      unsigned long decorrido = millis() - inicioSaida;
      if (decorrido >= delayEscolhido) {
        calibrarBaseline();
        digitalWrite(PINO_LED_VERDE, HIGH);
        estadoAtual = ARMADO;
      } else {
        digitalWrite(PINO_LED_VERDE, (millis() / 250) % 2); // pisca durante a contagem
      }
      break;
    }

    case ARMADO:
      if (botaoSil == LOW) { // desarme manual
        digitalWrite(PINO_LED_VERDE, LOW);
        estadoAtual = DESARMADO;
        break;
      }
      if (detectarVariacaoLDR()) {
        registrarEvento();
        estadoAtual = DISPARADO;
      }
      break;

    case DISPARADO:
      digitalWrite(PINO_LED_VERMELHO, (millis() / 150) % 2);
      tocarSirene();
      if (botaoSil == LOW) {
        noTone(PINO_BUZZER);
        digitalWrite(PINO_LED_VERMELHO, HIGH);
        estadoAtual = SILENCIADO;
      }
      break;

    case SILENCIADO:
      if (botaoArm == LOW) {
        digitalWrite(PINO_LED_VERMELHO, LOW);
        digitalWrite(PINO_LED_VERDE, LOW);
        estadoAtual = DESARMADO;
      }
      break;
  }

  atualizarDisplay();
}

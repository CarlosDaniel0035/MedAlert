#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// ---------- CONFIG WI-FI ----------
const char* WIFI_SSID     = "wifi";
const char* WIFI_PASSWORD = "senha";

// ---------- CONFIG MQTT ----------
const char* MQTT_BROKER   = "broker.hivemq.com";
const uint16_t MQTT_PORT  = 1883;

const char* MQTT_TOPIC_CMD    = "medalert/cmd";
const char* MQTT_TOPIC_STATUS = "medalert/status";

// ---------- NTP / HORA ----------
const long gmtOffset_sec      = -3 * 3600;  // Brasilia (UTC-3)
const int  daylightOffset_sec = 0;
const char* ntpServer         = "pool.ntp.org";

// ---------- PINOS ----------
#define PIN_LED     25   // LED no D25
#define PIN_BUZZER  27   // BUZZER no D27 (ativo em nivel BAIXO)
#define PIN_BUTTON  26   // BOTAO no D26 (para GND)

// ---------- OLEDs ----------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR_1   0x3C
#define OLED_ADDR_2   0x3C   // se o 2º for 0x3D, a gente ajusta depois

#define OLED1_SDA 21
#define OLED1_SCL 22
#define OLED2_SDA 32
#define OLED2_SCL 33

// Dois barramentos I2C independentes
TwoWire I2C_OLED1(0);  // I2C0 -> OLED1
TwoWire I2C_OLED2(1);  // I2C1 -> OLED2

Adafruit_SSD1306 displayMain  (SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED1, -1); // OLED1
Adafruit_SSD1306 displayStatus(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED2, -1); // OLED2

// ---------- ESTADO ----------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool   hasAlarm        = false;   // existe alarme programado?
String alarmTimeStr    = "";      // "HH:MM"
int    dailyAlarmCount = 0;       // qtde de alarmes disparados no dia

// controle de mensagens temporarias no display 1
bool           transientMessageActive = false;
unsigned long  transientMessageUntil  = 0;
unsigned long  lastInfoUpdate         = 0;
const unsigned long INFO_INTERVAL     = 1000; // 1s

bool lastButtonState = HIGH;

// ---------- NOVO: CONTROLE DO ALARME TOCANDO ----------
bool          alarmRinging       = false;   // alarme está tocando agora?
unsigned long lastAlarmToggle    = 0;       // ultimo toggle LED/BUZZER
bool          alarmOutputState   = false;   // estado atual (ON/OFF)

// ---------- DISPLAY FUNÇÕES ----------

// Mostra info permanente (data/hora/alarme) no display 1
void showInfoOnMainDisplay() {
  displayMain.clearDisplay();
  displayMain.setTextSize(1);
  displayMain.setTextColor(SSD1306_WHITE);
  displayMain.setCursor(0, 0);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[20];

    // Data
    strftime(buf, sizeof(buf), "%d/%m/%Y", &timeinfo);
    displayMain.print("Data: ");
    displayMain.println(buf);

    // Hora
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    displayMain.print("Hora: ");
    displayMain.println(buf);
  } else {
    displayMain.println("Sem hora NTP");
  }

  displayMain.print("Alarme: ");
  if (hasAlarm) {
    displayMain.println(alarmTimeStr);
  } else {
    displayMain.println("Nenhum");
  }

  displayMain.display();
}

// Mensagens temporarias no display 1 (3s) – sobrescrevem a info
void showOnMainDisplay(const String& l1,
                       const String& l2 = "",
                       const String& l3 = "") {
  displayMain.clearDisplay();
  displayMain.setTextSize(1);
  displayMain.setTextColor(SSD1306_WHITE);
  displayMain.setCursor(0, 0);
  displayMain.println(l1);
  if (l2.length() > 0) displayMain.println(l2);
  if (l3.length() > 0) displayMain.println(l3);
  displayMain.display();

  transientMessageActive = true;
  transientMessageUntil  = millis() + 3000; // 3 segundos
}

// Status no display 2
void updateStatusDisplay() {
  displayStatus.clearDisplay();
  displayStatus.setTextSize(1);
  displayStatus.setTextColor(SSD1306_WHITE);

  // Linha 1: WiFi
  displayStatus.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED) {
    displayStatus.println("WiFi: Conectado");
  } else {
    displayStatus.println("WiFi: Desconect.");
  }

  // Linha 2: IP
  displayStatus.setCursor(0, 12);
  displayStatus.print("IP: ");
  if (WiFi.status() == WL_CONNECTED) {
    displayStatus.println(WiFi.localIP());
  } else {
    displayStatus.println("0.0.0.0");
  }

  // Linha 3: Alarmes do dia
  displayStatus.setCursor(0, 24);
  displayStatus.print("Alarmes: ");
  displayStatus.println(dailyAlarmCount);

  // Linha 4: MQTT
  displayStatus.setCursor(0, 36);
  displayStatus.print("MQTT: ");
  if (mqttClient.connected()) {
    displayStatus.println("Conectado");
  } else {
    displayStatus.println("Desconect.");
  }

  // Linha final: MedAlert
  displayStatus.setCursor(0, SCREEN_HEIGHT - 8);
  displayStatus.println("MedAlert");

  displayStatus.display();
}

// ---------- AUXILIARES ----------

void beepShort(int ms) {
  digitalWrite(PIN_BUZZER, LOW);   // ativo em nivel BAIXO -> LIGA
  delay(ms);
  digitalWrite(PIN_BUZZER, HIGH);  // DESLIGA
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("[ESP32] Conectando no WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("[ESP32] WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());

  // Configura NTP sempre que conectar no WiFi
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("[ESP32] NTP configurado.");

  updateStatusDisplay();
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[ESP32] Conectando ao MQTT em ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    String clientId = "MedAlertDualOLED-";
    clientId += String(random(0xFFFF), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("[ESP32] MQTT conectado!");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
      Serial.print("[ESP32] Inscrito em: ");
      Serial.println(MQTT_TOPIC_CMD);
      updateStatusDisplay();
    } else {
      Serial.print("[ESP32] Falha MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando de novo em 1s...");
      delay(1000);
    }
  }
}

// parser simples de SET_ALARM HH:MM (opcional)
bool parseSetAlarm(const String& msg, String& timeStr) {
  if (!msg.startsWith("SET_ALARM")) return false;
  int spaceIndex = msg.indexOf(' ');
  if (spaceIndex < 0) return false;
  timeStr = msg.substring(spaceIndex + 1);
  timeStr.trim();
  if (timeStr.length() != 5) return false;
  if (timeStr.charAt(2) != ':') return false;
  return true;
}

// ---------- NOVO: VERIFICA SE É HORA DO ALARME (SEM BLOQUEAR) ----------
void checkAlarm() {
  // só dispara se tem alarme programado e ainda não está tocando
  if (!hasAlarm || alarmRinging) return;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  char buf[6];
  strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
  String currentTime = String(buf);

  if (currentTime == alarmTimeStr) {
    Serial.print("[ESP32] ALARME DISPARADO em ");
    Serial.println(currentTime);

    showOnMainDisplay("ALARME!", alarmTimeStr);

    String statusMsg = "ALARME_DISPARADO ";
    statusMsg += alarmTimeStr;
    mqttClient.publish(MQTT_TOPIC_STATUS, statusMsg.c_str(), false);

    // marca que o alarme está tocando (piscando/bipando)
    hasAlarm          = false;       // não re-disparar
    alarmRinging      = true;
    lastAlarmToggle   = millis();
    alarmOutputState  = false;
    dailyAlarmCount++;
    updateStatusDisplay();
  }
}

// ---------- NOVO: FAZ O LED E O BUZZER TOCAREM ENQUANTO O ALARME ESTÁ ATIVO ----------
void handleAlarmRinging() {
  if (!alarmRinging) return;

  unsigned long now = millis();
  const unsigned long interval = 250; // ms -> velocidade do pisca/bip

  if (now - lastAlarmToggle >= interval) {
    lastAlarmToggle  = now;
    alarmOutputState = !alarmOutputState;

    if (alarmOutputState) {
      digitalWrite(PIN_LED, HIGH);
      digitalWrite(PIN_BUZZER, LOW);   // buzzer ON
    } else {
      digitalWrite(PIN_LED, LOW);
      digitalWrite(PIN_BUZZER, HIGH);  // buzzer OFF
    }
  }
}

// ---------- CALLBACK MQTT ----------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();

  Serial.print("[ESP32] Mensagem em ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (String(topic) == MQTT_TOPIC_CMD) {
    if (msg == "1") {
      // LED ON + bip (LED FICA ACESO)
      Serial.println("[ESP32] CMD 1: LED ON + BIP");
      digitalWrite(PIN_LED, HIGH);   // deixa ligado
      beepShort(200);                // só bip
      showOnMainDisplay("CMD 1 recebido", "LED ON + BIP");
      updateStatusDisplay();
      mqttClient.publish(MQTT_TOPIC_STATUS, "CMD_1_LED_ON_BEEP", false);
    }
    else if (msg == "2") {
      // LED OFF + bip
      Serial.println("[ESP32] CMD 2: LED OFF + BIP");
      beepShort(200);
      digitalWrite(PIN_LED, LOW);    // garante LED desligado
      digitalWrite(PIN_BUZZER, HIGH);
      showOnMainDisplay("CMD 2 recebido", "LED OFF + BIP");
      mqttClient.publish(MQTT_TOPIC_STATUS, "CMD_2_LED_OFF_BEEP", false);
    }
    else if (msg.startsWith("SET_ALARM")) {
      String t;
      if (parseSetAlarm(msg, t)) {
        hasAlarm     = true;
        alarmTimeStr = t;
        showOnMainDisplay("Alarme definido", alarmTimeStr);

        // Bipa 3x ao definir alarme
        for (int i = 0; i < 3; i++) {
          beepShort(150);
          delay(150);
        }

        String statusMsg = "ALARME_DEFINIDO ";
        statusMsg += alarmTimeStr;
        mqttClient.publish(MQTT_TOPIC_STATUS, statusMsg.c_str(), false);
      }
    }
  }
}

// ---------- SETUP ----------
void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP); // botao para GND

  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, HIGH);

  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[ESP32] Iniciando MedAlert COMPLETO...");

  // Inicializa I2C e displays
  I2C_OLED1.begin(OLED1_SDA, OLED1_SCL);
  if (!displayMain.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_1)) {
    Serial.println("[ESP32] ERRO: nao inicializou OLED1 em 0x3C");
  } else {
    Serial.println("[ESP32] OLED1 OK (0x3C)");
  }

  I2C_OLED2.begin(OLED2_SDA, OLED2_SCL);
  if (!displayStatus.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_2)) {
    Serial.println("[ESP32] ERRO: nao inicializou OLED2 em 0x3C");
  } else {
    Serial.println("[ESP32] OLED2 OK (0x3C)");
  }

  displayMain.clearDisplay();
  displayMain.display();
  displayStatus.clearDisplay();
  displayStatus.display();

  // WiFi + NTP
  connectWiFi();

  // MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  connectMQTT();

  // Tela inicial
  showInfoOnMainDisplay();
  updateStatusDisplay();
}

// ---------- LOOP ----------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  // Verifica se chegou na hora do alarme (só marca para tocar)
  checkAlarm();

  // Faz o LED + buzzer piscarem/biparem enquanto o alarme estiver ativo
  handleAlarmRinging();

  // ---- Leitura do botao (bordas) ----
  bool currentButton = digitalRead(PIN_BUTTON);

  // Detecção de borda de subida -> botão apertado
  if (lastButtonState == HIGH && currentButton == LOW) {

    if (alarmRinging) {
      // SE O ALARME ESTÁ TOCANDO: SILENCIA IMEDIATAMENTE
      Serial.println("[ESP32] Botao: alarme silenciado");
      alarmRinging = false;
      digitalWrite(PIN_LED, LOW);
      digitalWrite(PIN_BUZZER, HIGH);
      showOnMainDisplay("Alarme silenciado");
      mqttClient.publish(MQTT_TOPIC_STATUS, "ALARME_SILENCIADO", false);
    }
    else {
      // Comportamento antigo (sem alarme tocando)
      if (!hasAlarm) {
        // Sem alarmes no momento -> LED + bip + mensagem
        Serial.println("[ESP32] Botao: sem alarmes no momento");
        digitalWrite(PIN_LED, HIGH);
        beepShort(200);
        digitalWrite(PIN_LED, LOW);
        showOnMainDisplay("Sem alarmes", "no momento");
        mqttClient.publish(MQTT_TOPIC_STATUS, "BOTAO_SEM_ALARMES", false);
      } else {
        // Ha alarme(s) programado(s)
        Serial.println("[ESP32] Botao: existem alarmes programados");
        showOnMainDisplay("Alarmes progr.:", alarmTimeStr);
      }
    }
  }

  lastButtonState = currentButton;

  // ---- Controle das telas ----
  unsigned long now = millis();

  // Quando mensagem temporaria expira, volta para tela de info
  if (transientMessageActive && now > transientMessageUntil) {
    transientMessageActive = false;
    showInfoOnMainDisplay();
  }

  // Atualiza data/hora a cada 1s se NAO tiver msg temporaria
  if (!transientMessageActive && (now - lastInfoUpdate > INFO_INTERVAL)) {
    lastInfoUpdate = now;
    showInfoOnMainDisplay();
  }
}

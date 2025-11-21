MedAlert – Lembrete de Medicamentos com ESP32 e MQTT

Este projeto é um protótipo de lembrete de medicação usando ESP32, 2 displays OLED, LED, buzzer, botão físico e MQTT com um cliente em Python.

Ele foi desenvolvido como aplicação prática de IoT na área de Saúde (ODS 3 – Saúde e Bem-Estar).

🔌 Visão geral do sistema

O ESP32 conecta no Wi-Fi, sincroniza a hora via NTP e se conecta a um broker MQTT.

Um script Python roda no computador, também conectado ao mesmo broker.

A comunicação acontece pelos tópicos:

Comandos (PC → ESP32)
medalert/cmd

Comandos usados:

1 → teste: LED pisca + bip no buzzer

2 → desliga o LED

SET_ALARM HH:MM → define um horário de alarme (ex.: SET_ALARM 08:30)

Status (ESP32 → PC)
medalert/status

Exemplos de mensagens:

CMD_1_LED_ON_BEEP

CMD_2_LED_OFF

ALARME_DEFINIDO HH:MM

ALARME_DISPARADO

ALARME_CONFIRMADO

BOTAO_SEM_ALARMES

Assim eu consigo mostrar para o professor a comunicação bidirecional entre o PC e o dispositivo IoT.

🧱 Hardware

ESP32

2 x OLED I2C (SSD1306 128x64)

OLED 1 (principal): SDA 21, SCL 22

OLED 2 (status): SDA 32, SCL 33

LED no pino D25

Buzzer ativo no pino D27 (aciona em nível baixo)

Botão no pino D26 (para GND, com INPUT_PULLUP)

🖥️ O que cada display mostra
Display 1 (principal)

Data (DD/MM/AAAA)

Hora (HH:MM:SS) – via NTP (pool.ntp.org, fuso UTC-3)

Alarme: horário configurado ou “Nenhum”

Mensagens temporárias:

comando recebido (CMD 1, CMD 2, SET_ALARM)

hora do remédio

confirmação da dose

mensagem quando o botão é apertado

Display 2 (status)

WiFi: Conectado ou WiFi: Desconect.

IP: x.x.x.x

Alarmes: X (quantidade de alarmes confirmados no dia)

MQTT: Conectado ou MQTT: Desconect.

MedAlert na última linha

⏰ Lógica do alarme

No Python, o usuário escolhe a opção de definir alarme (por menu).

O script pergunta:

hora (0–23)

minutos (0–59)

O Python monta SET_ALARM HH:MM e envia para medalert/cmd.

O ESP32:

guarda esse horário,

mostra “Alarme definido” no display 1,

faz 3 bipes intermitentes com o buzzer e LED piscando,

envia ALARME_DEFINIDO HH:MM em medalert/status.

Quando chega a hora do alarme

O ESP32 compara periodicamente a hora atual (HH:MM) com o horário do alarme.

Quando bate:

o LED começa a piscar,

o buzzer apita intermitente,

o display 1 mostra Hora do remedio! e o horário,

o ESP32 envia ALARME_DISPARADO para o PC.

Confirmação pelo botão

Se o alarme estiver tocando e o botão (D26) for pressionado:

o LED e o buzzer são desligados,

o contador de Alarmes no display 2 é incrementado,

o alarme atual é apagado,

o display 1 mostra Dose confirmada / Obrigado!,

o ESP32 envia ALARME_CONFIRMADO via MQTT.

Se não houver alarme configurado:

o botão faz o LED piscar + um bip curto,

mostra Sem alarmes no momento,

envia BOTAO_SEM_ALARMES.

Se houver alarme configurado, mas ainda não deu o horário:

o botão mostra Alarmes progr.: HH:MM no display 1.

📂 Organização dos arquivos

Exemplo de organização (ajuste para o seu repo):

firmware_esp32/MedAlert_ESP32.ino → código do ESP32 (Arduino)

medalert_menu.py → cliente MQTT em Python (rodando no PC)

🐍 Cliente Python (resumo)

Usa a biblioteca paho-mqtt.

Conecta no broker público: test.mosquitto.org:1883.

Exibe um menu no terminal com opções:

1 → LED + bip (teste)

2 → LED OFF

3 → comando livre (texto)

5 → definir alarme (pergunta hora e minutos e monta SET_ALARM HH:MM)

Mostra na tela tudo que o ESP32 envia em medalert/status, com logs do tipo:

"[ESP32 -> PC] Topico: medalert/status Mensagem: ALARME_DEFINIDO 08:30"

Isso ajuda na hora de apresentar o projeto, pois mostra claramente a troca de mensagens.

🎯 Objetivo acadêmico

Demonstrar um protótipo funcional de IoT aplicado à saúde (lembrete de medicação).

Integrar:

hardware (ESP32 + periféricos),

comunicação em rede (Wi-Fi, MQTT),

sincronização de tempo (NTP),

aplicação de apoio no PC (Python).

Mostrar, na prática, conceitos de:

comunicação assíncrona via MQTT,

lógica de alarme com horário real,

interação homem-máquina (botão, display, feedback sonoro e visual).

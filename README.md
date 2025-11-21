MedAlert – Relatório do Projeto
1. Introdução

O MedAlert é um protótipo de dispositivo IoT voltado para lembretes de medicação, desenvolvido utilizando um ESP32, dois displays OLED, LED, buzzer, botão físico e comunicação via MQTT com um cliente em Python.

A proposta do projeto é apoiar o paciente no controle de horários de medicamentos, permitindo:

Definir alarmes de forma remota (via computador);

Disparar avisos visuais e sonoros no horário configurado;

Confirmar a tomada da medicação por meio de um botão físico;

Exibir informações em tempo real sobre o estado do sistema.

O MedAlert se encaixa no contexto da ODS 3 – Saúde e Bem-Estar, demonstrando como tecnologias de IoT podem ser aplicadas para melhorar a adesão a tratamentos e o acompanhamento do uso de medicamentos.

2. Objetivos do Projeto

Objetivo geral

Desenvolver um protótipo funcional de lembrete de medicamentos baseado em ESP32 e MQTT, com interface física (displays, LED, buzzer, botão) e interface remota (cliente Python).

Objetivos específicos

Implementar comunicação Wi-Fi e MQTT no ESP32;

Integrar dois displays OLED para exibição de informações de status e do alarme;

Utilizar NTP para obter data e hora reais;

Receber comandos de alarme e teste via MQTT;

Disparar um alarme sonoro/visual no horário programado;

Permitir a confirmação da dose por meio de botão físico, registrando essa ação;

Criar um cliente Python capaz de:

Enviar comandos ao dispositivo;

Exibir, em tempo real, os status enviados pelo ESP32.

3. Arquitetura do Sistema

O sistema é composto por dois blocos principais:

Dispositivo embarcado (MedAlert – ESP32)
Responsável por:

Conectar à rede Wi-Fi;

Sincronizar a hora via NTP;

Conectar ao broker MQTT;

Controlar displays, LED, buzzer e botão;

Processar comandos recebidos via MQTT;

Publicar estados e eventos.

Cliente PC (Python + MQTT)
Responsável por:

Conectar ao mesmo broker MQTT;

Oferecer um menu interativo no terminal;

Enviar comandos para o ESP32 (testes e definição de alarmes);

Exibir as mensagens de status que o ESP32 publica.

Tópicos MQTT utilizados

Comandos (PC → ESP32):
medalert/cmd

Status (ESP32 → PC):
medalert/status

Broker utilizado

Broker público: test.mosquitto.org

Porta: 1883

4. Funcionamento do Dispositivo (ESP32)

O ESP32 é responsável por toda a lógica embarcada do MedAlert.

4.1 Conectividade e tempo

Conecta ao Wi-Fi com SSID e senha configurados no firmware;

Após conectar, sincroniza a hora com o servidor NTP (pool.ntp.org) usando fuso horário UTC-3 (Brasil);

A data e hora atualizadas são exibidas no Display 1 (principal).

4.2 Displays OLED

Display 1 (principal – SDA 21 / SCL 22)
Exibe:

Data (DD/MM/AAAA)

Hora (HH:MM:SS)

Status do alarme: horário configurado ou “Nenhum”

Mensagens temporárias, como:

CMD 1 recebido, CMD 2 recebido

Alarme definido

Hora do remedio!

Dose confirmada

Sem alarmes no momento

Display 2 (status – SDA 32 / SCL 33)
Exibe:

WiFi: Conectado ou WiFi: Desconect.

IP: x.x.x.x

Alarmes: X (quantidade de alarmes confirmados no dia)

MQTT: Conectado ou MQTT: Desconect.

MedAlert na linha inferior

4.3 Lógica de alarme

O alarme é configurado via comando MQTT:
SET_ALARM HH:MM

Quando esse comando é recebido:

O horário é armazenado em memória (alarmTimeStr);

O display 1 mostra “Alarme definido” e o horário;

O LED pisca e o buzzer executa 3 bipes de confirmação;

O ESP32 envia ALARME_DEFINIDO HH:MM para o PC.

Periodicamente, o ESP32 compara a hora atual com o horário do alarme:

Quando a hora atual HH:MM coincide com o horário configurado:

O LED começa a piscar continuamente;

O buzzer apita intermitente;

O display 1 mostra Hora do remedio! e o horário;

O ESP32 envia ALARME_DISPARADO para o PC.

4.4 Botão de confirmação (D26)

O botão tem três comportamentos distintos:

Se o alarme estiver tocando

Ao pressionar o botão:

LED e buzzer são desligados;

dailyAlarmCount é incrementado (contador de alarmes do dia);

o alarme é limpo (volta a “Nenhum”);

o display 1 mostra Dose confirmada / Obrigado!;

o ESP32 envia ALARME_CONFIRMADO no tópico de status.

Se não existir alarme configurado

Ao pressionar o botão:

LED pisca + buzzer emite um bip curto;

display 1 mostra Sem alarmes no momento;

ESP32 envia BOTAO_SEM_ALARMES.

Se houver alarme programado, mas ainda não tiver dado o horário

Ao pressionar o botão:

display 1 mostra Alarmes progr.: HH:MM.

5. Funcionamento do Cliente Python

O script medalert_menu.py funciona como interface de testes e controle do MedAlert.

5.1 Tecnologias

Linguagem: Python

Biblioteca MQTT: paho-mqtt

5.2 Funções principais

Conecta ao broker test.mosquitto.org:1883;

Publica mensagens em medalert/cmd;

Se inscreve em medalert/status e imprime tudo o que o ESP32 envia.

5.3 Menu interativo

O script exibe um menu no terminal, por exemplo:

1 → Envia "1" (LED + bip de teste)

2 → Envia "2" (LED OFF)

3 → Permite digitar um comando livre

5 → Fluxo guiado para definir alarme:

pede HORA (0–23)

pede MINUTOS (0–59)

monta automaticamente SET_ALARM HH:MM e envia

A cada status publicado pelo ESP32 em medalert/status, o Python imprime no formato:

[ESP32 -> PC] Topico: medalert/status
[ESP32 -> PC] Mensagem: ...

Isso ajuda a demonstrar para o professor a troca de mensagens entre o PC e o dispositivo em tempo real.

6. Testes e Validação

Foram realizados testes para validar:

Conexão estável do ESP32 à rede Wi-Fi e ao broker MQTT;

Recepção e interpretação correta dos comandos:

"1" → teste de LED + buzzer;

"2" → desligamento do LED;

SET_ALARM HH:MM → configuração correta do horário do alarme;

Exibição adequada das informações nos dois displays OLED;

Disparo automático do alarme no horário especificado:

LED piscando;

buzzer apitando intermitente;

mensagem de “Hora do remedio!” no display principal;

Confirmação da dose através do botão:

parada do alarme;

incremento no contador de “Alarmes” no display de status;

envio de ALARME_CONFIRMADO para o PC.

Os resultados mostraram que o sistema consegue receber comandos remotos, disparar alarmes na hora correta e registrar a confirmação do paciente, com feedback visual, sonoro e digital (via MQTT).

7. Conclusão

O projeto MedAlert demonstra, na prática, uma solução de Internet das Coisas aplicada à saúde, integrando:

Dispositivo embarcado com ESP32;

Comunicação Wi-Fi e MQTT;

Sincronização de horário via NTP;

Interfaces físicas intuitivas (displays, LED, buzzer, botão);

Cliente em Python para envio de comandos e monitoramento.

Além de cumprir os requisitos acadêmicos, o protótipo mostra como é possível construir, com componentes acessíveis, um sistema de lembrete de medicação com interação local e remota, servindo como base para evoluções futuras, como:

múltiplos alarmes;

integração com aplicativos móveis;

registro histórico das doses confirmadas;

envio de alertas para familiares ou cuidadores.

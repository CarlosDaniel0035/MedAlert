import time
import threading
from datetime import datetime, timedelta

import paho.mqtt.client as mqtt

# para os popups:
import tkinter as tk
from tkinter import simpledialog, messagebox

BROKER = "broker.hivemq.com"
PORT = 1883

TOPIC_CMD = "projeto/remedio/cmd"
TOPIC_STATUS = "projeto/remedio/status"

# ---- TK só para os pop-ups (sem janela principal) ----
root = tk.Tk()
root.withdraw()  # esconde a janela principal


# ==== CALLBACKS MQTT ====
def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker, codigo rc =", rc)
    client.subscribe(TOPIC_STATUS)
    print(f"Assinado em '{TOPIC_STATUS}' para receber mensagens do ESP32.\n")


def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    print(f"[{msg.topic}] {payload}")


# ==== ENVIO DE COMANDOS SIMPLES ====
def send_cmd(cmd: str):
    mqtt_client.publish(TOPIC_CMD, cmd)
    print(f">>> Enviado: {cmd}")


def cmd_on():
    send_cmd("ON")


def cmd_off():
    send_cmd("OFF")


def cmd_alarm_manual():
    # dispara alarme imediatamente
    send_cmd("ALARME")


def cmd_stop():
    send_cmd("STOP")


def cmd_beep():
    send_cmd("BEEP")


# ==== AGENDAMENTO DO ALARME (opção 5) ====
def cmd_set_alarm_agendado():
    # 1) pop-up para HORA
    h = simpledialog.askinteger(
        "Horario do remedio",
        "Digite a HORA (0-23):",
        minvalue=0,
        maxvalue=23,
        parent=root
    )
    if h is None:
        print("Agendamento cancelado.")
        return

    # 2) pop-up para MINUTO
    m = simpledialog.askinteger(
        "Horario do remedio",
        "Digite o MINUTO (0-59):",
        minvalue=0,
        maxvalue=59,
        parent=root
    )
    if m is None:
        print("Agendamento cancelado.")
        return

    hhmm = f"{h:02d}:{m:02d}"

    # Envia para o ESP32 mostrar nos displays
    send_cmd(f"SET_ALARM {hhmm}")

    # Calcula o próximo horário no PC
    agora = datetime.now()
    horario_hoje = agora.replace(hour=h, minute=m, second=0, microsecond=0)
    if horario_hoje <= agora:
        # se o horário já passou hoje, agenda para amanhã
        horario_alarme = horario_hoje + timedelta(days=1)
    else:
        horario_alarme = horario_hoje

    print(f"Alarme programado para {horario_alarme.strftime('%d/%m %H:%M')}")

    # Thread que espera até a hora e manda ALARME
    def worker():
        while True:
            agora2 = datetime.now()
            if agora2 >= horario_alarme:
                send_cmd("ALARME")
                print(">>> Alarme disparado automaticamente (comando ALARME enviado).")
                break
            time.sleep(1)

    threading.Thread(target=worker, daemon=True).start()
    messagebox.showinfo("Alarme agendado", f"Alarme definido para {hhmm}")


# ---- MQTT global ----
mqtt_client = mqtt.Client(client_id="PC-CONSOLE-Controller")
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

print("Conectando ao broker MQTT...")
mqtt_client.connect(BROKER, PORT, 60)
mqtt_client.loop_start()  # loop de rede em segundo plano


# ---- LOOP DO MENU DE TEXTO ----
def mostrar_menu():
    print("\n================ MENU MQTT =================")
    print("1 - LED ON")
    print("2 - LED OFF")
    print("3 - Disparar alarme AGORA (manual)")
    print("4 - Parar alarme (STOP)")
    print("5 - Programar horario do remedio (abre pop-ups)")
    print("0 - Sair")
    print("===========================================\n")


try:
    while True:
        mostrar_menu()
        opcao = input("Escolha uma opcao: ").strip()

        if opcao == "1":
            cmd_on()
        elif opcao == "2":
            cmd_off()
        elif opcao == "3":
            cmd_alarm_manual()
        elif opcao == "4":
            cmd_stop()
        elif opcao == "5":
            cmd_set_alarm_agendado()
        elif opcao == "0":
            print("Saindo...")
            break
        else:
            print("Opcao invalida.")

        # pequena pausa só pra ficar mais legível
        time.sleep(0.3)

except KeyboardInterrupt:
    print("\nEncerrando pelo teclado...")

finally:
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
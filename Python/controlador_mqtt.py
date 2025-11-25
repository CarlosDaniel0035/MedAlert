
import time
import paho.mqtt.client as mqtt

# =================== CONFIGURACAO ===================

BROKER = "broker.hivemq.com"   # MESMO broker do ESP32
PORT = 1883

TOPIC_CMD = "medalert/cmd"
TOPIC_STATUS = "medalert/status"


# =================== CHAMADAS ===================

def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[PC] Conectado ao broker {BROKER}:{PORT} (code={reason_code})")
    client.subscribe(TOPIC_STATUS)
    print(f"[PC] Inscrito em {TOPIC_STATUS}")
    print("-" * 40)


def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8", errors="ignore")
    except Exception:
        payload = str(msg.payload)

    print(f"[ESP32 -> PC] Topico : {msg.topic}")
    print(f"[ESP32 -> PC] Mensagem: {payload}")
    print("-" * 40)


# =================== FUNCOES DE ENVIO ===================

def enviar_cmd_simples(client, valor):
    """Envia um comando simples (ex.: '1' ou '2') para o ESP32."""
    client.publish(TOPIC_CMD, valor, qos=1)
    print(f"[PC] Enviado para {TOPIC_CMD}: {valor}")


def enviar_set_alarm(client):
    """Pergunta hora e minutos ao usuario e envia SET_ALARM HH:MM."""
    try:
        hora = int(input("Digite a hora (0-23): ").strip())
        if hora < 0 or hora > 23:
            print("Hora invalida.")
            return
    except ValueError:
        print("Valor de hora invalido.")
        return

    try:
        minuto = int(input("Digite os minutos (0-59): ").strip())
        if minuto < 0 or minuto > 59:
            print("Minutos invalidos.")
            return
    except ValueError:
        print("Valor de minutos invalido.")
        return

    hhmm = f"{hora:02d}:{minuto:02d}"
    comando = f"SET_ALARM {hhmm}"
    client.publish(TOPIC_CMD, comando, qos=1)
    print(f"[PC] Enviado para {TOPIC_CMD}: {comando}")
    print("O ESP32 deve confirmar o alarme no display e no topico medalert/status.")
    print("-" * 40)


# =================== MENU ===================

def mostrar_menu():
    print()
    print("===== MENU MEDALERT =====")
    print("1 - Ligar LED e dar BIP (envia '1')")
    print("2 - Desligar LED e dar BIP (envia '2')")
    print("3 - Comando livre (texto informado pelo usuario)")
    print("5 - Definir alarme (SET_ALARM HH:MM)")
    print("0 - Sair")
    print("=========================")


# =================== MAIN ===================

def main():
    # Cliente MQTT usando API v2
    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="MedAlert_PC"
    )
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"[PC] Conectando ao broker {BROKER}:{PORT} ...")
    client.connect(BROKER, PORT, 60)

    # loop MQTT em thread separada
    client.loop_start()

    try:
        while True:
            mostrar_menu()
            opcao = input("Escolha uma opcao: ").strip()

            if opcao == "1":
                enviar_cmd_simples(client, "1")
            elif opcao == "2":
                enviar_cmd_simples(client, "2")
            elif opcao == "3":
                texto = input("Digite o comando que deseja enviar: ").strip()
                if texto:
                    client.publish(TOPIC_CMD, texto, qos=1)
                    print(f"[PC] Enviado para {TOPIC_CMD}: {texto}")
            elif opcao == "5":
                enviar_set_alarm(client)
            elif opcao == "0":
                print("Saindo...")
                break
            else:
                print("Opcao invalida.")

            time.sleep(0.2)

    except KeyboardInterrupt:
        print("\n[PC] Encerrando (CTRL+C)...")

    finally:
        client.loop_stop()
        client.disconnect()
        print("[PC] Desconectado do broker.")


if __name__ == "__main__":
    main()


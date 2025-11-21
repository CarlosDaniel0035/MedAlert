import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion

# =============== CONFIGURAÇÕES =====================

BROKER = "test.mosquitto.org"
PORT = 1883

TOPIC_CMD = "medalert/cmd"
TOPIC_STATUS = "medalert/status"

# =============== CALLBACKS =========================

def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker, codigo de retorno:", rc)
    if rc == 0:
        print("Conexao OK!")
        client.subscribe(TOPIC_STATUS)
        print("Inscrito em", TOPIC_STATUS)
    else:
        print("Falha na conexao (veja o codigo acima).")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
    except UnicodeDecodeError:
        payload = str(msg.payload)

    # Log bonito para mostrar pro professor
    print("\n[ESP32 -> PC] Topico:", msg.topic)
    print("[ESP32 -> PC] Mensagem:", payload)

# =============== CLIENTE MQTT ======================

client = mqtt.Client(
    client_id="MedAlert_PC",
    callback_api_version=CallbackAPIVersion.VERSION1
)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_start()

# =============== MENU ==============================

def mostrar_menu():
    print("\n===== MENU MEDALERT =====")
    print("1 - LED ON + BIP (manda '1')")
    print("2 - LED OFF (manda '2')")
    print("3 - Comando livre (texto)")
    print("4 - Sair")
    print("5 - Definir alarme (SET_ALARM HH:MM)")
    print("=========================")

def definir_alarme():
    try:
        hora_str = input("Digite a HORA (0-23): ").strip()
        hora = int(hora_str)
        if not 0 <= hora <= 23:
            print("Hora invalida. Use 0 a 23.")
            return

        minuto_str = input("Digite os MINUTOS (0-59): ").strip()
        minuto = int(minuto_str)
        if not 0 <= minuto <= 59:
            print("Minuto invalido. Use 0 a 59.")
            return

        horario = f"{hora:02d}:{minuto:02d}"
        comando = f"SET_ALARM {horario}"
        client.publish(TOPIC_CMD, comando)
        print("Enviado:", comando)

    except ValueError:
        print("Valor invalido. Use apenas numeros.")

# =============== LOOP PRINCIPAL ====================

try:
    while True:
        mostrar_menu()
        opc = input("Escolha uma opcao (1-5): ").strip()

        if opc == "1":
            client.publish(TOPIC_CMD, "1")
            print("Enviado: 1 (LED ON + BIP)")

        elif opc == "2":
            client.publish(TOPIC_CMD, "2")
            print("Enviado: 2 (LED OFF)")

        elif opc == "3":
            comando = input("Digite o comando completo: ").strip()
            if not comando:
                print("Comando vazio, voltando ao menu.")
                continue
            client.publish(TOPIC_CMD, comando)
            print("Enviado comando livre:", comando)

        elif opc == "4":
            print("Saindo...")
            break

        elif opc == "5":
            definir_alarme()

        else:
            print("Opcao invalida, tente novamente.")

finally:
    client.loop_stop()
    client.disconnect()
    print("Desconectado do broker.")

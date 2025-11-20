## Aplicação em Python (monitoramento e comandos)

Na pasta `python` encontra-se o script responsável por:

- enviar comandos ao dispositivo (por exemplo: `SET_ALARM` e `ALARME`);
- receber e exibir mensagens MQTT publicadas pelo MedAlert;
- auxiliar nos testes de agendamento e confirmação de doses.

Para executar:

1. Instale as dependências (se necessário).
2. Configure o endereço do broker MQTT no código.
3. Execute o script em um terminal Python.

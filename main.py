import paho.mqtt.client as mqtt
# importe o conector do Python com o MySQL: instalar novamente neste env
import mysql.connector
# agora é necessário criar um objeto de conexão: encontra o MySQL e informa as credenciais para se conectar ao banco
# instalar novamente o concetor: pip install mysql-connector-python
con = mysql.connector.connect(host='localhost', database='banco_iot',user='root',password='root')
# verifique se a conexão ao BD foi realizada com sucesso
if con.is_connected():
    db_info = con.get_server_info()
    print("Conentado com sucesso ao Servidor ", db_info)
    # a partir de agora pode-se executar comandos SQL: para tanto é necessário criar um objeto tipo cursor
    # o cursor permite acesso aos elementos do BD
    cursor = con.cursor()
    # agora você pode executar o seu comando SQL. Por exemplo o comando "select database();" mostra o BD atual selecionado
    cursor.execute("select database();")
    # crio uma variável qualquer para receber o retorno do comando de execução
    linha = cursor.fetchone()
    print("Conectado ao DB", linha)
    # createTable é usada no comando SQL para  criar a tabela dadosIoT, que só tem um registro chamado "mensagem"
    createTable = """
                CREATE TABLE Medicao(
                  ID INT NOT NULL AUTO_INCREMENT,
                  Data_e_Horario DATETIME(0) NOT NULL,
                  Controle_do_Sistema VARCHAR(20),
                  Posicao_Janela VARCHAR(20),
                  Umidade DECIMAL(4,2),
                  Temperatura DECIMAL(4,2),
                  Estado_Chuva VARCHAR(25),
                  Estado_Lampada VARCHAR(20),
                  Sensor_Chuva_ID INT,
                  Sensor_Temperatura_ID INT,
                  Sensor_Umidade_ID INT,
                  Lâmpada_ID INT,
                  Servo_ID INT,
                  PRIMARY KEY (ID, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID),
                  INDEX `fk_Medição_Sensor Chuva_idx` (Sensor_Chuva_ID ASC) VISIBLE,
                  INDEX `fk_Medição_Sensor Temperatura1_idx` (Sensor_Temperatura_ID ASC) VISIBLE,
                  INDEX `fk_Medição_Sensor Umidade1_idx` (Sensor_Umidade_ID ASC) VISIBLE,
                  INDEX `fk_Medição_Lâmpada1_idx` (Lâmpada_ID ASC) VISIBLE,
                  INDEX `fk_Medição_Servo1_idx` (Servo_ID ASC) VISIBLE,
                  CONSTRAINT `fk_Medição_Sensor Chuva`
                    FOREIGN KEY (Sensor_Chuva_ID)
                    REFERENCES Sensor_Chuva (ID_Sensor_Chuva)
                    ON DELETE NO ACTION
                    ON UPDATE NO ACTION,
                  CONSTRAINT `fk_Medição_Sensor Temperatura1`
                    FOREIGN KEY (Sensor_Temperatura_ID)
                    REFERENCES Sensor_Temperatura (ID_Sensor_Temperatura)
                    ON DELETE NO ACTION
                    ON UPDATE NO ACTION,
                  CONSTRAINT `fk_Medição_Sensor Umidade1`
                    FOREIGN KEY (Sensor_Umidade_ID)
                    REFERENCES Sensor_Umidade (ID_Sensor_Umidade)
                    ON DELETE NO ACTION
                    ON UPDATE NO ACTION,
                  CONSTRAINT `fk_Medição_Lâmpada1`
                    FOREIGN KEY (Lâmpada_ID)
                    REFERENCES Lampada (ID_Atuador_Lampada)
                    ON DELETE NO ACTION
                    ON UPDATE NO ACTION,
                  CONSTRAINT `fk_Medição_Servo1`
                    FOREIGN KEY (Servo_ID)
                    REFERENCES Servo (ID_Atuador_Servo)
                    ON DELETE NO ACTION
                    ON UPDATE NO ACTION);
            """
    # este par try/except verifica se a tabela  já está criada. Se a tabela não existe, cai no try e é criada
    # se existe, cai no except e só mostra a mensagem que  a tabela existe
    try:
        cursor.execute(createTable)
    except:
        print("Tabela Medicao já existe.")
        pass

# esta função é a função callback informando que o cliente se conectou ao servidor
def on_connect(client, userdata, flags, rc):
    print("Connectado com codigo "+str(rc))
    # assim que conecta, assina um tópico. Se a conexão for perdida, assim que reconectar, as assinaturas serão renovadas
    client.subscribe("Temperatura_Sensor_DHT22")
    client.subscribe("Umidade_Sensor_DHT22")
    client.subscribe("Chuva_Sensor_MH")
    client.subscribe("Estado_Janela_Servo")
    client.subscribe("Topico_Controle_Manual")
    client.subscribe("Topico_Comando_Rele")

# esta função é a função callback que é chamada quando uma publicação é recebida do servidor
def on_message(client, userdata, msg):

    print("Mensagem recebida no tópico: " + msg.topic)
    print("Mensagem: "+ str(msg.payload.decode()))

    cursor = con.cursor()
    # ao receber um dado, insere como um registro da tabela medicao
    if msg.topic == "Temperatura_Sensor_DHT22":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, NULL, NULL, NULL, '{}', NULL, NULL, '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    if msg.topic == "Umidade_Sensor_DHT22":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, NULL, NULL, '{}', NULL, NULL, NULL, '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    if msg.topic == "Chuva_Sensor_MH":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, NULL, NULL, NULL, NULL, '{}', NULL, '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    if msg.topic == "Estado_Janela_Servo":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, NULL, '{}', NULL, NULL, NULL, NULL, '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    if msg.topic == "Topico_Controle_Manual":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, '{}', NULL, NULL, NULL, NULL, NULL, '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    if msg.topic == "Topico_Comando_Rele":
        cursor.execute("INSERT INTO medicao (Data_e_Horario, Controle_do_Sistema, Posicao_Janela, Umidade, Temperatura, Estado_Chuva, Estado_Lampada, Sensor_Chuva_ID, Sensor_Temperatura_ID, Sensor_Umidade_ID, Lâmpada_ID, Servo_ID) VALUES (CURRENT_TIMESTAMP, NULL, NULL, NULL, NULL, NULL, '{}', '1', '1', '1', '1','1')".format(str(msg.payload.decode())))

    con.commit()
    #cursor.execute("SELECT * FROM medicao")
    #myresult = cursor.fetchall()
    #print(myresult)

    if str(msg.payload.decode().strip()) == "termina":
        print("Recebeu comando termina.")
        if con.is_connected():
            cursor.close()
            con.close()
            print("Fim da conexão com o Banco dadosIoT")
    if str(msg.payload.decode().strip())  == "delete":
        cursor.execute("TRUNCATE TABLE medicao")

if __name__ == '__main__':
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("test.mosquitto.org", 1883, 60)
    #client.connect("broker.hivemq.com", 1883, 60) # broker alternativo
    # a função abaixo manipula trafego de rede, trata callbacks e manipula reconexões.
    client.loop_forever()
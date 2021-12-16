//Cristian Fernandes Sena - IoT 2021/2

//Declaração da Task para o processamento no segundo núcleo
TaskHandle_t Task1;

//Bibliotecas
#include <arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include "DHT.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"

#define DHTTYPE DHT22

//Definicoes para o MQTT:
#define Controle_Manual      "Topico_Controle_Manual"
#define Comando_Servo        "Topico_Comando_Servo"
#define Comando_Rele         "Topico_Comando_Rele"
#define Temperatura_Sensor   "Temperatura_Sensor_DHT22"
#define Umidade_Sensor       "Umidade_Sensor_DHT22"
#define Chuva_Sensor         "Chuva_Sensor_MH"
#define Estado_Janela        "Estado_Janela_Servo"

#define ID_MQTT  "Casa Inteligente ESP32 - TI"     //id mqtt (para identificação de sessão)

//Rede:
const char* SSID     = "RICARDO 3"; //SSID da rede WI-FI que deseja se conectar
const char* PASSWORD = "34125143";  //Senha da rede WI-FI que deseja se conectar
//const char* SSID     = "A"; //SSID da rede WI-FI que deseja se conectar
//const char* PASSWORD = "12345678910";  //Senha da rede WI-FI que deseja se conectar

//Configuração Sinric:
#define APP_KEY           "ab55bc6b-08e3-4d9d-9b9c-f19c09eac8e5"      
#define APP_SECRET        "2eb6ce12-73f3-425a-8b40-a5f254528773-e13f927c-3b70-44b3-995c-6557aa33a49d"  
#define SWITCH_ID         "619efa193983ae74fbdc09d4"    
#define BAUD_RATE         9600


//Pinos:
const int portaChuva = GPIO_NUM_35; //Entrada do sensor de chuva
const int servoPin = GPIO_NUM_13;   //Pino de sinal do servo motor
const int rele = GPIO_NUM_4;        //Pino de sinal do relé
const int DHTPIN = GPIO_NUM_32;     //Pino de sinal do DHT22
const int LRed = GPIO_NUM_27;       //Led Vermelho
const int LGreen = GPIO_NUM_26;     //Led Verde
const int LBlue = GPIO_NUM_25;      //Led Azul

//Configuração do Broker:
const char* BROKER_MQTT = "test.mosquitto.org";
int BROKER_PORT = 1883;   // Porta do Broker MQTT

//Declaração dos objetos:
LiquidCrystal lcd(16, 17, 5, 18, 19, 21); //(RS, E, D4, D5, D6, D7)
DHT dht(DHTPIN, DHTTYPE);                 // Cria o objeto do sensor DHT com seu pino e tipo
Servo servo;                              // Cria o objeto do servo motor
WiFiClient espClient;                     // Cria o objeto espClient
PubSubClient MQTT(espClient);             // Instancia o Cliente MQTT passando o objeto espClient

//Variáveis:
int leituraChuva;
float tempAnterior = 0.0, umidAnterior = 0.0, umid = 0.0, temp = 0.0, hic = 0.0;
bool aberto = 0;              // Estado da janela
bool controleManual = 0;      // Informa se o usuário quer controlar a janela manualmente ou não
bool estadoChuvaAnterior = 0; // Informa a leitura anterior do sensor de chuva (1 = estava chovendo, 0 = não estava chovendo)
char temp_string[10] = {0};   // Cria string para a temperatura
char umid_string[10] = {0};   // Cria string para a umidade

//Prototypes:
void controleLED(int cor);
void controleDHT();
void controleLCD();
void ControleServo(bool comando);
void controleDeChuva();
void publicacoes();
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void Task1code(void * pvParameters);

/*
   Implementações
*/

/*
   Descrição: Inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initWiFi(void){
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
}


/*
   Descrição: Inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initMQTT(void){
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //Informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}


/*
   Descrição: Função de callback (esta função é chamada toda vez que uma informação de um dos tópicos subescritos chega)
   Parâmetros: Um topico, o payload e seu tamanho
   Retorno: nenhum
*/
void mqtt_callback(char* topic, byte* payload, unsigned int length){
  String msg;

  //Obtem a string do payload recebido
  for (int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }

  Serial.print("Chegou a seguinte string via MQTT: ");
  Serial.println(msg);

  //Toma ação dependendo da string recebida:
  
  if (msg.equals("M")){
    controleManual = 1; //Liga o controle manual do servo
    Serial.println("Controle manual ativado");
  }
  else if (msg.equals("A")){
    controleManual = 0; //Desliga o controle manual do servo
    Serial.println("Controle desativado");
  }
  else if (msg.equals("O") && controleManual){
    ControleServo(1); //Abre a janela
    Serial.println("Janela aberta manualmente");
  }
  else if (msg.equals("C") && controleManual){
    ControleServo(0); //Fecha a janela
    Serial.println("Janela fechada manualmente");
  }
  else if (msg.equals("1")){
    digitalWrite(rele, HIGH); //Liga o relé
    Serial.println("Rele ligado");
  }
  else if (msg.equals("0")){
    digitalWrite(rele, LOW); //Desliga o relé
    Serial.println("Rele Desligado");
  }
}


/*
   Descrição: Reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
              em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectMQTT(void){
  while (!MQTT.connected()){
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)){
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(Comando_Servo);
      MQTT.subscribe(Comando_Rele);
      MQTT.subscribe(Controle_Manual);
      
    }else{
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      controleLED(3);  //LED Vermelho
      //delay(2000);
    }
  }  
}


/*
   Descrição: Reconecta-se ao WiFi
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectWiFi(void){
  //Se já está conectado a rede WI-FI, nada é feito.
  if (WiFi.status() == WL_CONNECTED)
    return;

  //Caso contrário, são efetuadas tentativas de conexão
  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED){
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("\nIP obtido: ");
  Serial.println(WiFi.localIP());
}

//-----------

/*
   Descrição: Controla a cor do LED RGB através do número informado
   Parâmetros: Um valor inteiro
   Retorno: nenhum
*/
void controleLED(int cor){
  if(cor == 1){                    // LED Verde
    digitalWrite(LGreen, HIGH);
    digitalWrite(LBlue, LOW);
    digitalWrite(LRed, LOW);
  }else if(cor == 2){              // LED azul
    digitalWrite(LGreen, LOW);
    digitalWrite(LBlue, HIGH);
    digitalWrite(LRed, LOW);
  }else{                           // LED Vermelho
    digitalWrite(LGreen, LOW);
    digitalWrite(LBlue, LOW);
    digitalWrite(LRed, HIGH);
  }
}


/*
   Descrição: Faz as leituras dos dados do sensor DHT22, verifica seu funcionamento e imprime no monitor serial 
   Parâmetros: nenhum
   Retorno: nenhum
*/
void controleDHT(){
  umid = dht.readHumidity();                     // Leitura de Umidade
  temp = dht.readTemperature();                  // Leitura de Temperatura
  //hic = dht.computeHeatIndex(temp, umid, false); // Calcula o Heat Index

  //Verifica se o sensor DHT22 está respondendo
  if (isnan(umid) || isnan(temp)) {
    Serial.println(F("Falha no sensor DHT"));
    controleLED(3);                              // LED Vermelho
    return;
  }

  //Imprime os valores no monitor serial
  //Umidade:
  Serial.print(umid);
  Serial.print("% ");
  //Temperature:
  Serial.print(temp);
  Serial.println("°C ");
  //Heat index:
  //Serial.print(hic);
  //Serial.println(" ");
}


/*
   Descrição: Atualiza os dados de temperatura e umidade no LCD
   Parâmetros: nenhum
   Retorno: nenhum
*/
void controleLCD(){
  //Verifica se houve alguma alteração nos valores antes de imprimir no LCD
  if(tempAnterior != temp || umidAnterior != umid){
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Temp:");
    lcd.print(temp);
    //lcd.print("°C");
    lcd.setCursor(3, 1);
    lcd.print("Umid:");
    lcd.print(umid);
  }
}


/*
   Descrição: Controla a posição do servo motor com base no comando passado
   Parâmetros: Um bool comando
   Retorno: nenhum
*/
void ControleServo(bool comando){
  if(comando){                                                      // Abre a janela  
    if(!aberto){                                                    // Verifica se está fechada
        for(int posDegrees = 0; posDegrees <= 120; posDegrees++) {  // Aciona o servo
          servo.write(posDegrees);
          delay(20);
        }
        //Publica o estado da janela
        MQTT.publish(Estado_Janela, "Janela aberta");
        Serial.println("Enviado MQTT: Janela aberta");
      }
      aberto = 1;                                                   //Atualiza o estado
      
  }else{                                                            // Fecha a janela
    if(aberto){                                                     // Verifica se está aberta
      for(int posDegrees = 120; posDegrees >= 0; posDegrees--) {    // Aciona o servo
          servo.write(posDegrees);
          delay(20);
      }
      //Publica o estado da janela
      MQTT.publish(Estado_Janela, "Janela fechada");
      Serial.println("Enviado MQTT: Janela fechada");
    }
    aberto = 0;                                                     // Atualiza o estado
  }
}


/*
   Descrição: Efetua a leitura do sensor de chuva e faz as atuações necessárias
   Parâmetros: nenhum
   Retorno: nenhum
*/
void controleDeChuva(){ 
  leituraChuva = analogRead(portaChuva);           // Ler o sensor de chuva
  Serial.println(leituraChuva);                    // Imprime o valor no monitor serial

  if(leituraChuva >= 1000 || temp < 20.0){         // Verifica se está chovendo ou se está frio (menos que 20°C)
    controleLED(2);                                // LED azul
    
    //Verifica se o controle manual da janela está desativado
    if(!controleManual){
      ControleServo(0);                            // Fecha a janela
    }
  }else if(temp >= 20.0 && leituraChuva < 1000){   // Verifica se não está frio e sem chuva
    controleLED(1);                                // LED verde
    
    //Verifica se o controle manual da janela está desativado
    if(!controleManual){
      ControleServo(1);                            // Abre a janela  
    }
  }
}


/*
   Descrição: Efetua o envio das informações para o Broker e garante que não serão enviados dados desnecessarios/repetidos
   Parâmetros: nenhum
   Retorno: nenhum
*/
void publicacoes(){
  //Verifica se houve alteração na temperatura
  if(tempAnterior != temp){
    //Formata a temperatura  como string
    sprintf(temp_string, "%.2f", temp);
  
    //Publica a temperatura
    MQTT.publish(Temperatura_Sensor, temp_string);
    Serial.println(temp_string);
  }

  //Verifica se houve alteração na umidade
  if(umidAnterior != umid){
    //Formata a umidade  como string
    sprintf(umid_string, "%.2f", umid);
  
    //Publica a umidade
    MQTT.publish(Umidade_Sensor, umid_string);
    Serial.println(umid_string);
  }

  //Passa os valores lidos paras as variáveis de verificação
  tempAnterior = temp;
  umidAnterior = umid;

  //Verifica se está chovendo
  if(leituraChuva >= 1000){
    //Verifica se o estado anterior não indicava que está chovendo
    if(!estadoChuvaAnterior){
      //Publica o estado da chuva
      MQTT.publish(Chuva_Sensor, "Está chovendo");
      Serial.println("Enviado MQTT: Esta chovendo");
    }
    estadoChuvaAnterior = 1; //Atualiza o estado anterior
  }
  //Não está chovendo
  else{
    //Verifica se o estado anterior indicava que está chovendo
    if(estadoChuvaAnterior){
      //Publica o estado da chuva
      MQTT.publish(Chuva_Sensor, "Não está chovendo");
      Serial.println("Enviado MQTT: Nao esta chovendo");
    }
    estadoChuvaAnterior = 0; //Atualiza o estado anterior
  }
  
}

//------------------------------------------------------

/*
   Descrição: Recebe dados via Sinric para controle do rele
   Parâmetros: Id do rele no Sinric e seu estado
   Retorno: Um bool de controle
*/
bool onPowerState(const String &deviceId, bool &state) {
  digitalWrite(rele, state);         //Comanda o rele
  Serial.println("Rele via alexa");
  
  //Atualiza no MQTT Box e Dash
  if(state)
    MQTT.publish(Comando_Rele, "1");
  else if(!state)
    MQTT.publish(Comando_Rele, "0");
    
  return true;                       //Request handled properly
}


/*
   Descrição: Procedimento paralelo que verifica o estado das conexões WiFI e ao broker MQTT.
              Em caso de desconexão (qualquer uma das duas), a conexão é refeita.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){ 
    //Garante funcionamento das conexões WiFi e ao broker MQTT
    if (!MQTT.connected())
      reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

    reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
    delay(150);
  } 
}


//------------------------------------------------------

/*
   Descrição: Inicia o ESP32
   Parâmetros: nenhum
   Retorno: nenhum
*/
void setup() {
  Serial.begin(9600);

  //Configuração do LCD
  lcd.begin(16, 2);
  lcd.clear();

  //Iniciando o sensor DHT22 
  dht.begin();

  pinMode(portaChuva, INPUT);
  pinMode(rele, OUTPUT);
  pinMode(LRed, OUTPUT);
  pinMode(LGreen, OUTPUT);
  pinMode(LBlue, OUTPUT);

  //Inicia o servo motor
  servo.attach(servoPin);

  //Inicializa a conexao wi-fi
  initWiFi();

  //Inicializa a conexao ao broker MQTT
  initMQTT();

  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];   // Cria um novo switch para a Alexa
  mySwitch.onPowerState(onPowerState);                // Aplica onPowerState callback
  SinricPro.begin(APP_KEY, APP_SECRET);               // Inicia SinricPro

  //Configuração da tarefa no core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* função que implementa a tarefa */
                    "Task1",     /* nome da tarefa */
                    10000,       /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,        /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    1,           /* prioridade da tarefa (0 a N) */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* Núcleo que executará a tarefa */ 
                                   
  delay(500);
}


/*
   Descrição: loop
   Parâmetros: nenhum
   Retorno: nenhum
*/
void loop() {

  //Controles principais:
  controleDHT();
  controleLCD();
  controleDeChuva();

  //Garante funcionamento das conexões WiFi e ao broker MQTT
  //VerificaConexoesWiFIEMQTT();

  //Efetua o envio das informações para o Broker
  publicacoes();
  
  //keep-alive da comunicação com broker MQTT
  MQTT.loop();

  //Handle SinricPro commands
  SinricPro.handle();
  
  delay(550);
}

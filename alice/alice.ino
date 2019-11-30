//Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
//Autor: Pedro Bertoleti

#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <BigNumber.h>

//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_DISCONNECT   "dcc075/users/disconnect"    //tópico MQTT de envio de informações para Broker
#define TOPICO_CONNECT      "dcc075/users/connect"                                
#define TOPICO_ALPHA        "dcc075/params/alpha"
#define TOPICO_BETA         "dcc075/params/beta"
#define TOPICO_DELTA        "dcc075/params/delta"
#define TOPICO_TOTIENT      "dcc075/params/totient_delta"
#define TOPICO_COMMAND      "dcc075/users/command"                                
      

// ATENCAO: coloque um id unico. Sugestao -> iotGrupo1, iotGrupo2 e assim por diante.                                                    
#define ID_MQTT  "Alice"     //id mqtt (para identificação de sessão)
                               //IMPORTANTE: este deve ser único no broker (ou seja, 
                               //            se um client MQTT tentar entrar com o mesmo 
                               //            id de outro já conectado ao broker, o broker 
                               //            irá fechar a conexão de um deles).
                               

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1


// WIFI
const char* SSID = "TP-LINK_3319A2"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "3&SnaW@E$NEK"; // Senha da rede WI-FI que deseja se conectar
 
// MQTT
const char* BROKER_MQTT = "broker.hivemq.com";
//const char* BROKER_MQTT = "iot.eclipse.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT


//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
String msg_ant = "";
bool sent = false;
bool on_net = true; 
bool params_received = false;
String alpha;
String beta;
String delta;
String totient_delta;
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);

/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    BigNumber::begin();
    randomSeed(analogRead(A0));
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
}
 
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial(){
    Serial.begin(115200);
}

//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    
    reconectWiFi();
}
 
//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

int randomCharArray(char *s, char *r)
{
  bool critical = true;
  byte i = 0;
  byte len = strlen(s);
  while (critical && i < len)
  {
    byte lm = s[i] - '0';
    r[i] = random(lm + 1) + '0';
    critical = (r[i] == s[i]);
    i++;
  }

  while (i < len)
  {
    uint32_t rv = rand();       // 4 bytes = 8 nybbles 0..15
    byte sum = 0;
    for (byte b = 0; b < 8; b++)
    {
      // get nybble
      byte v = rv & 0x0F;
      // prepare for next nybble
      rv >>= 4;
      // not in 0..9 ==> skip nybble but use its value.
      if (v > 9)
      {
        sum += v;
        continue;
      }
      r[i++] = v + '0';
      if (i == len) break;
    }
    // if all nybbles are skipped, sum is at least 80
    // so we can use sum to generate at least one digit per iteration.
    if (i < len && sum >= 80)   
    {
      r[i++] = sum % 10 + '0'; 
    }
  }
  r[len] = '\0';
}

char StrContains(const char *str, const char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
    return 0;
}

void compute_session_key(){
  BigNumber _alpha         = (char*)alpha.c_str();
  BigNumber _beta          = (char*)beta.c_str();
  BigNumber _delta         = (char*)delta.c_str();
  BigNumber _totient_delta = (char*)totient_delta.c_str();
  Serial.print("Alpha: ");
  Serial.println(_alpha);
  Serial.print("Beta: ");
  Serial.println(_beta);
  Serial.print("Delta: ");
  Serial.println(_delta);
  Serial.print("Totient delta: ");
  Serial.println(_totient_delta);

  char r[100] = "";
  randomCharArray((char*)delta.c_str(), r);
  BigNumber xa = r;
  char s[100] = "";
  randomCharArray((char*)totient_delta.c_str(), s);
  BigNumber xb = s;
 
  while(xb*_beta % _totient_delta == 0){
    char t[100] = "";
    randomCharArray((char*)totient_delta.c_str(), t);
    xb = t;
  }
  BigNumber gamma = _alpha * xa.pow(2) + _beta * xb;
  Serial.println();
  Serial.println(String(String("- Gamma generated by ") + ID_MQTT + String(".")));
  Serial.println();
  Serial.print("Gamma: ");
  Serial.println(gamma);
}

//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg, _topic;

    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    //obtem a string do topico recebido
    for(int i = 0; topic[i] != '\0'; i++)
    {
       char c = (char)topic[i];
       _topic += c;
    }
    sent = false;
    //toma ação dependendo da string recebida:
    if(_topic.equals(TOPICO_COMMAND) && StrContains(msg.c_str(), ID_MQTT)){
      if(StrContains(msg.c_str(), "disconnect")){
        MQTT.publish(TOPICO_DISCONNECT, ID_MQTT);
        Serial.println("- Disconectado da rede.");
        on_net = false;
      }
    }
    if(_topic.equals(TOPICO_ALPHA)){
      alpha = String(msg);
    }
    if(_topic.equals(TOPICO_BETA)){
      beta = String(msg);
    }
    if(_topic.equals(TOPICO_DELTA)){
      delta = String(msg);
    }
    if(_topic.equals(TOPICO_TOTIENT)){
      totient_delta = String(msg);
    }

    if(alpha.length() > 0 && beta.length() > 0 && delta.length() > 0 && totient_delta.length() > 0){
      Serial.println("- Session parameters received.");
      params_received = true;
      compute_session_key();
     // Serial.println(session_key);
    }
    delay(500);
}
 
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_COMMAND);
            MQTT.subscribe(TOPICO_ALPHA);
            MQTT.subscribe(TOPICO_BETA);
            MQTT.subscribe(TOPICO_DELTA);
            MQTT.subscribe(TOPICO_TOTIENT);
            MQTT.publish(TOPICO_CONNECT, ID_MQTT);
            on_net = true;
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
 
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
    
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

//Função: envia ao Broker o estado atual do output 
//Parâmetros: nenhum
//Retorno: nenhum
void EnviaEstadoOutputMQTT(void)
{
    if(sent) return;
    sent = true;
    //Serial.println("- Estado da saida D0 enviado ao broker!");
    delay(1000);
}

//Função: inicializa o output em nível lógico baixo
//Parâmetros: nenhum
//Retorno: nenhum
void InitOutput(void)
{
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(D0, OUTPUT);
    digitalWrite(D0, HIGH);          
}


//programa principal
void loop() 
{   
    
    //envia o status de todos os outputs para o Broker no protocolo esperado
    //EnviaEstadoOutputMQTT();
    
    //keep-alive da comunicação com broker MQTT
    if(on_net){
      //garante funcionamento das conexões WiFi e ao broker MQTT
      VerificaConexoesWiFIEMQTT();
      MQTT.loop();
    }else if(MQTT.connected()){
      while(MQTT.connected()){
        MQTT.disconnect();
        on_net = false;
      }
    }
    delay(500);
}

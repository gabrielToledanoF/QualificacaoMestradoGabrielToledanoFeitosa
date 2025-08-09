/* Código feito em cima do código da biblioteca Heltec para comunicação LoRa*/

#include "LoRaWan_APP.h" //Biblioteca para LoRa
#include "Arduino.h"  //Biblioteca Arduino, para uso do Esp32
#include <WiFi.h> //Biblioteca pra WiFi
#include <HTTPClient.h> //Biblioteca para HTTP
#include <Time.h>  //Biblioteca pra Tempo

#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             20       // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 251 // Define o tamanho do Payload

// NTP Config

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -4 * 3600; //GMT -4 horas
const int   daylightOffset_sec = 0;
struct tm timeinfo;
// END NTP Config

// LoRa Config
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi,rxSize;

bool lora_idle = true;

//End LoRa Config

//Variáveis para HTTP

const char WIFI_SSID[] = "Troque_Para_O_Seu_WiFI"; // Rede de Internet a se conectar
const char WIFI_PASSWORD[] = "Troque_Para_Sua_Senha";// Senha da rede

String HOST_NAME   = "https://iotsapi.rcchome.com.br/"; //Host HTTP, não mudar

String outgoing;
//Fim Variáveis HTTP



void setup() {
    Serial.begin(115200);
    //Setup LoRa
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
    
    txNumber=0;
    rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    //Fim Setup LoRa
    
    //Wi-Fi Setup
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando no Wi-fi: ");
    Serial.println(WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connectado Com Sucesso.");
    //End Wi-fi Setup
    //NTP Setup
    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    if(!getLocalTime(&timeinfo)) Serial.println("Failed to obtain time");
    //End NTP Setup
  }


void loop(){
  ///*
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
  //*/
}
///*
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ){ //Função chamada ao receber um pacote
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );
    Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
    while(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain Time, trying again!");
    }
    char time[40];
    strftime(time,40, "%d %B %Y %H:%M:%S", &timeinfo);
    outgoing+= String(time)+ ", ";
    outgoing += rxpacket;
    lora_idle = true;
    post(); //chama o post dentro dessa função, para tentar fazer post somente quando receber uma mensagem
}

//*/

void post(){
  //HTTP SETUP
  HTTPClient http;
  int httpCode;
  int cont = 0;
  bool sucesso = false;

  http.begin(HOST_NAME);
  http.addHeader("Content-Type", "text/plain");
  //
  while(cont < 5 && !sucesso){ //Tenta faxer o post 5 vezes
    httpCode = http.POST(outgoing);
  // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_ACCEPTED) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("sucesso");
        sucesso = true;
        break;
      } else {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        Serial.println("sucesso?");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    cont += 1;
  }
  http.end();

  if(sucesso){outgoing ="";} //Se o post for um sucesso, reseta a variável. Caso o post tenha falhado mantem a variável para armazenar as mensagens anteriores

}

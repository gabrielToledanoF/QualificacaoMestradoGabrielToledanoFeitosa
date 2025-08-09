#include "thermistor.h"         //Biblioteca para uso de Thermistors (Sensor de Temperatura)
#include <GravityTDS.h>         //Biblioteca para o uso do TDS
#include <SPI.h>
#include <LoRa.h>

//LoRa Pins
#define csPin 10
#define resetPin 7
#define irqPin 6

//SPI Interface Pins
#define SCK 13
#define MISO 12
#define MOSI 11

#define period 5 // Minutos de delay entre coleta(60 segundos * X minutos)

//Thermistor Variables
#define NTC_PIN A0 // Analog pin used to read the NTC

// Thermistor object
THERMISTOR thermistor(NTC_PIN,        // Analog pin
                      10000,          // Nominal resistance at 25 ºC
                      3950,           // thermistor's beta coefficient
                      10000);         // Value of the series resistor
float temp; //Variável para armazenar o valor da temperatura

//End Thermistor Variables

//TDS Variables
#define TdsSensorPin A2 //Pino onde está o TDS
GravityTDS gravityTds;  //Variável utilizada pela biblioteca para realizar as medições
//End TDS Variables

//PH Variables
#define PH_PIN A5
float valor_calibracao = 21.5; // Fator de calibração para o ajuste fino
//End PH Variables

//Função utilizada para resetar o arduino, para resolver o problema do chip LoRa
void (* resetFunc) (void) = 0;

void setup() {
  //Serial Setup
  Serial.begin(9600);
  delay(1000);
  //while (!Serial);
  //End Serial Setup
  
  //TDS Setup
  gravityTds.setPin(TdsSensorPin); //define o pino para medição
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
  gravityTds.setKvalue(1.00);
  //End TDS Setup

  //SPI.begin(SCK,MISO,MOSI,csPin);
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  //Set LoRa Params
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(5);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setPreambleLength(8);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);


}

void loop() {
  String outgoing = coleta();              // outgoing message
  sendPacket(outgoing);
  Serial.println(outgoing);
  for(int i = 0; i < period; i++){
    delay(60000);
  }
  resetFunc();
}

void sendPacket(String out){ //Função com defeito, trava ao executar o end packet pela segunda vez, arduino sendo resetado para lidar com este problema
  LoRa.beginPacket(); 
  LoRa.print(out);
  LoRa.endPacket();

}

String coleta(){
  String msg;
  msg += coleta_temperatura();
  msg += coleta_TDS();
  msg += coleta_PH();
  return msg;
}

String coleta_temperatura(){
  temp = thermistor.read();
  return String(temp) + ",";   // Read temperature);
}

String coleta_TDS(){
  gravityTds.setTemperature(temp);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  float tdsValue = gravityTds.getTdsValue();  // then get the value
  return String(tdsValue) + ",";
}

String coleta_PH(){
  float media = 0.0;
  for(int i = 0; i < 10; i++){
    media += (analogRead(PH_PIN) * 5.0) / 1024.0;
  }
  media = media/10.0;

  float valor_pH = (-5.70 * media) + valor_calibracao; //Calcula valor de pH
  return String(valor_pH) + ";";
}

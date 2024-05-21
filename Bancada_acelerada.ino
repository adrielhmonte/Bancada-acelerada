#include "DHT.h"
#include <MQUnifiedsensor.h>
#include <LiquidCrystal_I2C.h>
/************************Hardware (MQ-9)***********************************/
#define Board "Arduino UNO"
#define Pin A0  //Analog input 4 of your arduino
/***********************Software (MQ-9)***********************************/
#define Type "MQ-9" //MQ9
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ9CleanAir 9.6 //RS / R0 = 60 ppm 
/*****************************SENSOR DHT***********************************************/
#define DHTPIN 5
#define DHTTYPE DHT22

//Global
char* valvState;
int lcdColumns = 16;
int lcdRows = 4;
int buttonPin = 2;
int relePin = 10;
int lcdTempC = 0, lcdTempR = 4, lcdUmiC = 0, lcdUmiR = 1, lcdValvC = 0, lcdValvR = 2, lcdGasC = 0, lcdGasR = 3;
//SCL = A5; SDA = A4
float umid, temp, CH4;
//Declarar os sensores
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void setup() {

  Serial.begin(9600);
  lcd.init();
  lcd.clear();
  //Equação para calcular a concentração PPM a partir dos valores em Volts do sensor
  MQ9.setRegressionMethod(1); //_PPM =  a*ratio^b
  /*****************************  MQ Inicialização ********************************************/ 
  //Remarks: Configure the pin of arduino as input.
  pinMode(A0, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW); 
  /************************************************************************************/ 
  MQ9.init(); 
  /* 
    //If the RL value is different from 10K please assign your RL value with the following method:
    MQ9.setRL(10);
  */
  /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
   // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrando sensor MQ-9. Aguarde...");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ9.update(); //Atualizar a variável, o arduino irá ler a voltagem da entrada analógica
    calcR0 += MQ9.calibrate(RatioMQ9CleanAir);
    Serial.print(".");
  }
  MQ9.setR0(calcR0/10);
  Serial.println(" Calibrado! ");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  
    /*
  Regressão exponencial (Valores para ajustar de acordo com o gás):
  GAS     | a      | b
  LPG     | 1000.5 | -2.186
  CH4     | 4269.6 | -2.648
  CO      | 599.65 | -2.244
  */
  Serial.println(F("Inicializando sensor DHT..."));
  dht.begin();
  
}

void loop() {

  MQ9.setA(4269.6); MQ9.setB(-2.648); // Configura a equação para calcular gás

  umid = dht.readHumidity();
  temp = dht.readTemperature();

  if (isnan(umid) || isnan (temp)) {
    Serial.println("Falha na leitura do sensor DHT.");
    lcd.clear();
    lcd.setCursor(lcdTempC, lcdTempR);
    lcd.print("Erro no sensor");
    return;
  }

  valvState = "OFF";
  lcd.clear();
  lcd.setCursor(lcdTempC, lcdTempR);
  lcd.print("Temp: ");
  lcd.print(temp); lcd.print(" C");
  lcd.setCursor(lcdUmiC, lcdUmiR);
  lcd.print("Umidade: ");
  lcd.print(umid); lcd.print(" %");
  lcd.setCursor(lcdValvC, lcdValvR);
  lcd.print("Valvula: "); lcd.print(valvState);
  lcd.setCursor(lcdGasC, lcdGasR);
  lcd.print("Pressione botao");

  while(digitalRead(buttonPin) == HIGH) {
    delay(50);
  }

  int n = 1, m = 1;
  while (n <= 1) {
    while (m <= 1) {
      for (int j = 1; j <= 6; j++) {
        for (int i = 1; i <= 30; i++) {
          Serial.print("BUMP TEST "); Serial.println(i);
          umid = dht.readHumidity();
          temp = dht.readTemperature();
          MQ9.update();
          CH4 = MQ9.readSensor();
          digitalWrite(relePin, HIGH);
          valvState = "ON";
          lcd.clear();
          lcd.setCursor(lcdTempC, lcdTempR);
          lcd.print("Temp: ");
          lcd.print(temp); lcd.print("C");
          lcd.setCursor(lcdUmiC, lcdUmiR);
          lcd.print("Umidade: ");
          lcd.print(umid); lcd.print(" %");
          lcd.setCursor(lcdValvC, lcdValvR);
          lcd.print("Valvula:"); lcd.print(valvState); lcd.print("  BUMP:");lcd.print(i);
          lcd.setCursor(lcdGasC,lcdGasR);
          lcd.print("CH4:"); lcd.print(CH4); lcd.print(" PPM");
          delay(15000);
          MQ9.update();
          CH4 = MQ9.readSensor();
          lcd.clear();
          lcd.setCursor(lcdTempC, lcdTempR);
          lcd.print("Temp: ");
          lcd.print(temp); lcd.print("C");
          lcd.setCursor(lcdUmiC, lcdUmiR);
          lcd.print("Umidade: ");
          lcd.print(umid); lcd.print(" %");
          lcd.setCursor(lcdValvC, lcdValvR);
          lcd.print("Valvula:"); lcd.print(valvState); lcd.print(" BUMP:");lcd.print(i);
          lcd.setCursor(lcdGasC,lcdGasR);
          lcd.print("CH4:"); lcd.print(CH4); lcd.print(" PPM");
          digitalWrite(relePin, LOW);
          valvState = "OFF";
          lcd.clear();
          lcd.setCursor(lcdTempC, lcdTempR);
          lcd.print("Temp: ");
          lcd.print(temp); lcd.print("C");
          lcd.setCursor(lcdUmiC, lcdUmiR);
          lcd.print("Umidade: ");
          lcd.print(umid); lcd.print(" %");
          lcd.setCursor(lcdValvC, lcdValvR);
          lcd.print("Valvula:"); lcd.print(valvState); lcd.print(" BUMP:");lcd.print(i);
          lcd.setCursor(lcdGasC,lcdGasR);
          lcd.print("CH4:"); lcd.print(CH4); lcd.print(" PPM");
          delay(30000);
        }
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK: ");lcd.print(j);
        delay(15000);
        digitalWrite(relePin, HIGH);
        valvState = "ON";
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Valvula: "); lcd.print(valvState);
        delay(30000);
        MQ9.update();
        CH4 = MQ9.readSensor();
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK: ");lcd.print(j);
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Valvula: "); lcd.print(valvState);
        lcd.setCursor(lcdGasC, lcdGasR);
        lcd.print("CH4: "); lcd.print(CH4); lcd.print(" PPM");
        delay(15000);
        MQ9.update();
        CH4 = MQ9.readSensor();
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK: ");lcd.print(j);
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Valvula: "); lcd.print(valvState);
        lcd.setCursor(lcdGasC, lcdGasR);
        lcd.print("CH4: "); lcd.print(CH4); lcd.print(" PPM");
        lcd.setCursor(lcdGasC, lcdGasR);
        lcd.print("CH4: "); lcd.print(CH4); lcd.print(" PPM");
        delay(15000);
        MQ9.update();
        CH4 = MQ9.readSensor();
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK: ");lcd.print(j);
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Valvula: "); lcd.print(valvState);
        lcd.setCursor(lcdGasC, lcdGasR);
        lcd.print("CH4: "); lcd.print(CH4); lcd.print(" PPM");
        delay(30000);
        digitalWrite(relePin, LOW);
        valvState = "OFF";
        MQ9.update();
        CH4 = MQ9.readSensor();
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK: ");lcd.print(j);
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Valvula: "); lcd.print(valvState);
        lcd.setCursor(lcdGasC, lcdGasR);
        lcd.print("CH4: "); lcd.print(CH4); lcd.print(" PPM");
        lcd.setCursor(lcdGasC, lcdGasR);
        delay(5000);
        lcd.clear();
        lcd.setCursor(lcdTempC, lcdTempR);
        lcd.print("CHECK"); lcd.print(j);
        lcd.setCursor(lcdUmiC, lcdUmiR);
        lcd.print("Aguarde");
        delay(40000);
      }
      lcd.clear();
      lcd.setCursor(lcdTempC, lcdTempR);
      lcd.print("AJUSTE");
      lcd.setCursor(lcdUmiC, lcdUmiR);
      lcd.print("Pressione botao");
      while(digitalRead(buttonPin) == HIGH) {delay(50);}
      m++;
    }
    n++;
  }
  lcd.clear();
  lcd.setCursor(lcdTempC, lcdTempR);
  lcd.print("Ciclo concluido!");
  while(true) {}
}
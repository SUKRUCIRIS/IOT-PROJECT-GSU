#include <MPU6050.h>  
#include <LiquidCrystal.h>
#include <Wire.h>

#define wfname "Sukru"
#define wfpassword "molek12118"
#define trigPin 6
#define echoPin 7
#define periodms 20000 //thingspeak message update interval is limited to >15 seconds

String data="";
MPU6050 accelgyro;
int16_t ax=0, ay=0, az=0;
int16_t gx=0, gy=0, gz=0;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
unsigned long lost_data=0,timestamp=0,loopstart=0,loopend=0;
float distance=0;
bool setupdone=false;
int input=0;

void(*resetFunc) (void) = 0;

void esp_command(String command, int try_n, char* reaction, bool datasend=false){
  int worked=0;
  for(int i=0;i<try_n;i++){
    Serial.println(command);
    delay(2);
    if(Serial.find(reaction)){
      worked=1;
      break;
    }
  }
  if(worked==0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ESP8266 Failed:");
    lcd.setCursor(0,1);
    lcd.print(command);
    if(datasend){
      lost_data++;
    }
    if(!setupdone){
      while(1);
    }
  }
  else{
    if(setupdone){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Running Ok");
      lcd.setCursor(0,1);
      lcd.print("Lost data: "+String(lost_data));
    }
  }
}

void setup() {
  lcd.begin(16, 2);

  lcd.clear();
  lcd.print("IDLE");
  while(input<1000){
    input=analogRead(A0);
  }

  lcd.clear();
  lcd.print("Starting");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.begin(115200);
  Wire.begin();
  accelgyro.initialize();
  
  esp_command("AT",5,"OK");

  esp_command("AT+CWMODE=1",5,"OK");

  esp_command("AT+CWJAP=\""+String(wfname)+"\",\""+String(wfpassword)+"\"",100,"OK");
  
  if(!accelgyro.testConnection()){
    lcd.clear();
    lcd.print("MPU6050 Failed");
    while(1);
  }

  lcd.clear();
  lcd.print("Running Ok");
  setupdone=true;
}

void loop() {
  loopstart=millis();

  input=analogRead(A0);
  if(input<10){
    resetFunc();
  }

  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  distance=(pulseIn(echoPin, HIGH)/2) * 0.0343;

  esp_command("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80",10,"CONNECTED");

  data="GET https://api.thingspeak.com/update?api_key=RB4YH95INCIQ0BIB";
  data+="&field1=";
  data+=String(ax);
  data+="&field2=";
  data+=String(ay);
  data+="&field3=";
  data+=String(az);
  data+="&field4=";
  data+=String(gx);
  data+="&field5=";
  data+=String(gy);
  data+="&field6=";
  data+=String(gz);
  data+="&field7=";
  data+=String(distance);
  data+="&field8=";
  data+=String(timestamp);

  esp_command("AT+CIPSEND="+String(data.length()+4),5,">",true);

  Serial.println(data);

  esp_command("AT+CIPCLOSE",5,"OK"); 

  loopend=millis();
  if(loopend<loopstart+periodms){
    delay(loopstart+periodms-loopend);
    timestamp+=periodms;
  }
  else{
    timestamp+=(loopend-loopstart);
  }
}

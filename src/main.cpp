#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define ARDUINO_RX 16
#define ARDUINO_TX 17
#define nBuffer 100
#define nBuffer2 1920

SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);
const char * ssid = "emiro"; 
const char * pwd = "Balisa16";

const char * udpAddress = "192.168.0.101";
const int udpPort = 8080;
int counter = 0;
const int maxCounter = 100;

WiFiUDP udp, udp2;

static int8_t Send_buf[8] = {0};
static uint8_t ansbuf[10] = {0};

String mp3Answer;

#define CMD_NEXT_SONG     0X01
#define CMD_PREV_SONG     0X02
#define CMD_PLAY_W_INDEX  0X03
#define CMD_VOLUME_UP     0X04
#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06

#define CMD_SNG_CYCL_PLAY 0X08
#define CMD_SEL_DEV       0X09
#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F
#define CMD_STOP_PLAY     0X16
#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0x18
#define CMD_SET_SNGL_CYCL 0X19
#define CMD_SET_DAC 0X1A
#define DAC_ON  0X00
#define DAC_OFF 0X01
#define CMD_PLAY_W_VOL    0X22
#define CMD_PLAYING_N     0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f
#define DEV_TF            0X02


char bufferData[nBuffer] = "hello world";
int bufferData2[nBuffer2];
String nodeName = "Node1";

IPAddress local_IP(192, 168, 0, 102);
IPAddress gateway_IP(192, 168, 0, 254);
IPAddress dns0_IP(192, 168, 0, 254);
IPAddress dns1_IP(0, 0, 0, 0);
IPAddress subnet_IP(255, 255, 255, 0);

LiquidCrystal_I2C lcd (0x27, 16,2);
void sendCommand(int8_t command, int16_t dat);

int outputPin = 18;
const int chOut = 0;
const int FreqOut =8000;
const int resOut = 8;

int lcdBacklight = 2;

void setup(){

  // Serial.begin(115200);
  // Set Output
  pinMode(lcdBacklight, INPUT);
  ledcSetup(chOut, FreqOut, resOut);
  ledcAttachPin(outputPin, chOut);

  lcd. init ();
  lcd. backlight ();

  if(!WiFi.config(local_IP, gateway_IP, subnet_IP, dns0_IP, dns1_IP))
  {
    lcd.setCursor(0,0);
    lcd.print("Configuration Failed ... ");
  }
  mp3.begin(9600);
  delay(500);
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.setCursor(0,0);
    lcd.print("Trying Connected");
    lcd.setCursor(1,1);
    lcd. print ("Please wait ...");
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Node 1 IP :");
  lcd.setCursor(1,1);
  lcd. print (WiFi.localIP());
  //This initializes udp and transfer buffer
  udp.begin(udpPort);
  udp.beginPacket(udpAddress, udpPort);
  udp.print("Node 1 Connected");
  udp.endPacket();
}

String sbyte2hex(uint8_t b)
{
  String shex;

  shex = "0X";
  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}

String sanswer(void)
{
  uint8_t i = 0;
  String mp3answer = "";

  // Get only 10 Bytes
  while (mp3.available() && (i < 10))
  {
    uint8_t b = mp3.read();
    ansbuf[i] = b;
    i++;
    mp3answer += sbyte2hex(b);
  }

  // if the answer format is correct.
  if ((ansbuf[0] == 0x7E) && (ansbuf[9] == 0xEF))
  {
    return mp3answer;
  }

  return "???: " + mp3answer;
}

void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Send_buf[0] = 0x7e;
  Send_buf[1] = 0xff;
  Send_buf[2] = 0x06;
  Send_buf[3] = command;
  Send_buf[4] = 0x01;
  Send_buf[5] = (int8_t)(dat >> 8);
  Send_buf[6] = (int8_t)(dat);
  Send_buf[7] = 0xef;
  for (uint8_t i = 0; i < 8; i++)
    mp3.write(Send_buf[i]) ;
}

String decodeMP3Answer() {
  String decodedMP3Answer = "";

  decodedMP3Answer += sanswer();

  switch (ansbuf[3]) {
    case 0x3A:
      decodedMP3Answer += " -> Memory card inserted.";
      break;

    case 0x3D:
      decodedMP3Answer += " -> Completed play num " + String(ansbuf[6], DEC);
      break;

    case 0x40:
      decodedMP3Answer += " -> Error";
      break;

    case 0x41:
      decodedMP3Answer += " -> Data recived correctly. ";
      break;

    case 0x42:
      decodedMP3Answer += " -> Status playing: " + String(ansbuf[6], DEC);
      break;

    case 0x48:
      decodedMP3Answer += " -> File count: " + String(ansbuf[6], DEC);
      break;

    case 0x4C:
      decodedMP3Answer += " -> Playing: " + String(ansbuf[6], DEC);
      break;

    case 0x4E:
      decodedMP3Answer += " -> Folder file count: " + String(ansbuf[6], DEC);
      break;

    case 0x4F:
      decodedMP3Answer += " -> Folder count: " + String(ansbuf[6], DEC);
      break;
  }

  return decodedMP3Answer;
}


void sendMP3Command(char *c) {
  lcd.setCursor(1,1);
  switch (*c)
  {
  case '1':
    sendCommand(CMD_FOLDER_CYCLE, 1);
    lcd. print ("Playing Song 1    ");
    break;
  case '2':
    sendCommand(CMD_FOLDER_CYCLE, 2);
    lcd. print ("Playing Song 2    ");
    break;
  case '3':
    sendCommand(CMD_FOLDER_CYCLE, 3);
    lcd. print ("Playing Song 2    ");
    break;
  case '4':
    sendCommand(CMD_FOLDER_CYCLE, 4);
    lcd. print ("Playing Song 3    ");
    break;
  case '5':
    sendCommand(CMD_FOLDER_CYCLE, 5);
    lcd. print ("Playing Song 1    ");
    break;
  case 'c': // Status check
    udp.beginPacket(udpAddress, udpPort);
    udp.print(nodeName);
    udp.endPacket();
    memset(bufferData, 0, nBuffer);
    break;
  case 's': // Stop Command
    sendCommand(CMD_STOP_PLAY, 0);
    lcd. print (WiFi.localIP() + "  ");
    break;
  default:
    break;
  }
}
void reboot()
{
  ESP.restart();
}

char data[1];
char data2[] = "";
void loop(){
  if(digitalRead(lcdBacklight)) lcd.noBacklight();
  else lcd.backlight();
  udp.parsePacket();
  if(udp.read(data, 1) > 0){
    // Serial.println((char *)data);
    if (data[0] == 'r')
    {
      // Serial.println((char *)data);
      lcd.setCursor(1,1);
      lcd. print ("Hearing ...     ");
      data[0] = ' ';
      int k = 0;
      char data3[3] = " ";
      while (true)
      {
        udp.parsePacket();
        if(udp.read(data2, 1) > 0)
        {
            // Serial.println((char *)data2);
          if (data2[0] == 'z')
          {
            // Serial.println((char *)data2);
            data2[0] = ' ';
            lcd.setCursor(1,1);
            lcd. print (WiFi.localIP());
            break;
          }
          if(data2[0] == ' ') continue;
          if(data2[0] == ',')
          {
            int pwm = atoi(data3);
            ledcWrite(chOut, pwm);
            //Serial.println(pwm);
            data3[0] = '0';
            data3[1] = ' ';
            data3[2] = ' ';
            k = 0;
          }
          else
          {
            // Serial.println(k);
            data3[k] = data2[0];
            k++;
          }
        }
      }
    }
    else
    {
      sendMP3Command((char *)data);
    }
  }
}

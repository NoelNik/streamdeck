#include <EncButton.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#define SerialMon Serial
#define APPLEMIDI_DEBUG SerialMon
#include <AppleMIDI.h>

char ssid[] = "Max";        //  your network SSID (name)
char pass[] = "ML87654312"; // your network password (use for WPA, or use as key for WEP)

unsigned long t0 = millis();
int8_t isConnected = 0;

APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();
#define calibirate 0                //запуск калибровки кнопок
#define analogPin A0                //pin омметра
int enc_value[3] = {100, 100, 100}; //значения энкодера
int button_N = 4;                   //количество кнопок
int buttons_res[4]{7, 26, 61, 147}; // резисторы на кнопках
int analog_ohms = 0;                // показания с аналог пина
const float Vin = 3.3;              //опорное напряжение платы
float Vout = 0;                     // напряжение на резисторе
const float R_const = 1000;         // эталонный резистор
float R_find = 0;                   // искомый резистор
int button_ohms = 0;                // запись ohms()
int button_found = 0;               //запись найденной кнопки
int midi_channel = 1;               //выбор  канала midi

EncButton<EB_CALLBACK, 5, 4> enc1;
EncButton<EB_CALLBACK, 14, 12> enc2;
EncButton<EB_CALLBACK, 13, 16> enc3;

void isr()
{
  enc1.tickISR();
  enc2.tickISR();
  enc3.tickISR();
}

void ICACHE_RAM_ATTR isr();

//smartdelay
void smartdelay(unsigned long smartdelaytime)
{
  unsigned long realtime = millis();
  while ((millis() - realtime) < smartdelaytime)
  {
    ESP.wdtFeed();
  }
}

// //омметр
int ohms()
{
  analog_ohms = analogRead(A0);
  if (analog_ohms)
  {
    float ohm_count = analog_ohms * Vin; // буфер для расчета сопротивления
    Vout = (ohm_count) / 1024.0;
    ohm_count = (Vout / Vin);
    R_find = R_const * ohm_count;
  }
  return int(R_find);
}

// //среднее значение омметра
int middle_oms()
{
  int middle = 0;
  int maxmiddle = 0;
  for (int i = 0; i < 5; i++) //считывание среднего
  {
    middle = ohms();
    if (middle > maxmiddle)
    {
      maxmiddle = middle;
    }
    smartdelay(10);
  }
  return maxmiddle;
}

// калибровка
void calibiration()
{
  Serial.println("START"); //начало калибровки
  for (int i = 0; i < button_N; i++)
  {
    Serial.print("Press button ");
    Serial.println(i + 1);
    while (ohms() > 200)
    {
      ESP.wdtFeed();
    }
    buttons_res[i] = middle_oms();
    Serial.println(middle_oms());
    Serial.println('OK');
    smartdelay(2000);
  }
  Serial.println("buttons");
  for (int i = 0; i < button_N; i++)
  {
    Serial.println(buttons_res[i]);
  }
  smartdelay(1000);
}

// поиск кнопки
int button_find()
{
  button_ohms = middle_oms();
  int i = 0;
  while (i < button_N) //поиск кнопки
  {
    if (abs(buttons_res[i] - button_ohms) < buttons_res[i] * 0.3)
    {
      return i + 1;
    }
    i++;
  }
  return 0;
}

//опрос кнопок
void read()
{
  button_found = button_find();
}

// midi control send
void enc_send()
{
  if (enc1.isTurn())
  {
    enc_value[0] += enc1.getDir();
    if (enc_value[0] > 127)
    {
      enc_value[0] = 127;
    }
    if (enc_value[0] < 0)
    {
      enc_value[0] = 0;
    }
    MIDI.sendControlChange(byte(20), byte(enc_value[0]), byte(midi_channel));
  }
  if (enc2.isTurn())
  {
    enc_value[1] += enc2.getDir();
    if (enc_value[1] > 127)
    {
      enc_value[1] = 127;
    }
    if (enc_value[1] < 0)
    {
      enc_value[1] = 0;
    }
    MIDI.sendControlChange(byte(21), byte(enc_value[1]), byte(midi_channel));
  }
  if (enc3.isTurn())
  {
    enc_value[2] += enc3.getDir();
    if (enc_value[2] > 127)
    {
      enc_value[2] = 127;
    }
    if (enc_value[2] < 0)
    {
      enc_value[2] = 0;
    }
    MIDI.sendControlChange(byte(22), byte(enc_value[2]), byte(midi_channel));
  }
}

// midi button send
void button_send()
{
  if (button_found > 0) //button control change send
  {
    MIDI.sendNoteOn(byte(button_found), byte(0), byte(midi_channel));
    while (button_find() == button_found)
    {
    }
  }
}

void setup()
{
  smartdelay(2000);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(5), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(4), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(12), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(14), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(13), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(16), isr, CHANGE);
  DBG_SETUP(9600);
  DBG("Booting");

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DBG("Establishing connection to WiFi..");
  }
  DBG("Connected to network");
  DBG(F("Add device named Arduino with Host"), WiFi.localIP(), "Port", AppleMIDI.getPort(), "(Name", AppleMIDI.getName(), ")");
  MIDI.begin();

  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t &ssrc, const char *name)
                               {
                                 isConnected++;
                                 DBG(F("Connected to session"), ssrc, name);
                               });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t &ssrc)
                                  {
                                    isConnected--;
                                    DBG(F("Disconnected"), ssrc);
                                  });

  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity)
                       { DBG(F("NoteOn"), note); });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity)
                        { DBG(F("NoteOff"), note); });
  MIDI.setHandleControlChange([](byte channel, byte control, byte value)
                              { DBG(F("ControlChange"), control); });

  if (calibirate)
  {
    calibiration();
  }
  pinMode(16, INPUT);
  digitalWrite(16, 0);
}

void loop()
{
  MIDI.read();
  if ((isConnected > 0))
  {
    read();
    enc_send();
    button_send();
  }
}
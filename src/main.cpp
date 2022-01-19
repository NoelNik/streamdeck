#include <Arduino.h>
#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <EncButton.h>
#include <AppleMIDI.h>
#include <ESP8266WiFi.h>

#define analogPin A0 //pin омметра

bool button_midi_states[3];         //switcherы
int buttons_res[3] = {20, 60, 220}; // резисторы на кнопках
int enc_value[3] = {0, 0, 0};
int button_N = 3;              //количество кнопок
int analog_ohms = 0;           // показания с аналог пина
const int Vin = 5;             //опорное напряжение платы
float Vout = 0;                // напряжение на резисторе
const float R_const = 1000;    // эталонный резистор
float R_find = 0;              // искомый резистор
int button_ohms = 0;           // запись ohms()
int button_found = 0;          //запись найденной кнопки
int midi_channel = 0;          //выбор  канала midi
int ohms_none = R_const - 200; //сопротивление, пока не нажата кнопка
int k = 0;                     //k-костыль

EncButton<EB_TICK, 0, 1> enc1;
EncButton<EB_TICK, 2, 3> enc2;
EncButton<EB_TICK, 4, 7> enc3;

void isr()
{
  enc1.tickISR();
  enc2.tickISR();
  enc3.tickISR();
  // тикер в прерывании
  // Не вызывает подключенные коллбэки в нутри прерывания!!!
}

//smartdelay
void smartdelay(unsigned long smartdelaytime)
{
  unsigned long realtime = millis();
  while ((millis() - realtime) < smartdelaytime)
  {
  }
}

// омметр
int ohms()
{
  analog_ohms = analogRead(analogPin);
  if (analog_ohms)
  {
    float ohm_count = analog_ohms * Vin; // буфер для расчета сопротивления
    Vout = (ohm_count) / 1024.0;
    ohm_count = (Vout / Vin);
    R_find = R_const * ohm_count;
  }
  return int(R_find);
}

//среднее значение омметра
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

//midi
void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
void programChange(byte channel, byte program)
{
  midiEventPacket_t event = {0x0C, 0xC0 | channel, program, 0};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
void noteOn(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}
void noteOff(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void calibiration()
{
  Serial.println("START"); //начало калибровки
  for (int i = 0; i < button_N; i++)
  {
    Serial.print("Press button ");
    Serial.println(i + 1);
    while (ohms() > R_const - R_const * 0.3)
    {
    }
    buttons_res[i] = middle_oms();
    Serial.println(middle_oms());
    smartdelay(1000);
  }
  Serial.println("buttons");
  for (int i = 0; i < 3; i++)
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
    if ((abs(button_ohms) / buttons_res[i]) < 1.1)
    {
      return i + 1;
    }
    i++;
  }
  return 0;
}

//опрос всего
void read()
{
  button_found = button_find();
  enc1.tick();
  enc2.tick();
  enc3.tick();
}

//midi button send
void button_send()
{
  if (button_found > 0) //button control change send
  {
    noteOn(midi_channel, button_found, 127);
    while (button_find() == button_found)
    {
    }
  }
}

//midi enc send
void enc_send()
{
  if ((enc1.turn()))
  {
    enc_value[0] += enc1.getDir();

    if (enc_value[0] > 127)
    {
      enc_value[0] = 127;
    }
    else if (enc_value[0] < 0)
    {
      enc_value[0] = 0;
    }
    controlChange(midi_channel, 20, enc_value[0]);
  }
  if ((enc2.turn()))
  {
    enc_value[1] += enc2.getDir();

    if (enc_value[1] > 127)
    {
      enc_value[1] = 127;
    }
    else if (enc_value[1] < 0)
    {
      enc_value[1] = 0;
    }
    controlChange(midi_channel, 21, enc_value[1]);
  }
  if ((enc3.turn()))
  {
    enc_value[2] += enc3.getDir();

    if (enc_value[2] > 127)
    {
      enc_value[2] = 127;
    }
    else if (enc_value[2] < 0)
    {
      enc_value[2] = 0;
    }
    controlChange(midi_channel, 22, enc_value[2]);
  }
}

void setup()
{
  Serial.begin(9600);
  // smartdelay(5000);
  // if (Serial)
  // {
  //   calibiration();
  // }
  // прерывание обеих фаз энкодера на функцию isr
  attachInterrupt(0, isr, CHANGE);
  attachInterrupt(1, isr, CHANGE);
  attachInterrupt(2, isr, CHANGE);
  attachInterrupt(3, isr, CHANGE);
  attachInterrupt(4, isr, CHANGE);
}

void loop()
{
  read();
  enc_send();
  button_send();
}
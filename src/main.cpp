#include <Arduino.h>
#include <EncButton.h>
#include <AppleMIDI.h>
#define analogPin A0 //pin омметра

int enc_value[3] = {100, 100, 100}; //значения энкодера
int buttons_res[3] = {20, 60, 220}; // резисторы на кнопках
int button_N = 3;                   //количество кнопок
int analog_ohms = 0;                // показания с аналог пина
const int Vin = 5;                  //опорное напряжение платы
float Vout = 0;                     // напряжение на резисторе
const float R_const = 1000;         // эталонный резистор
float R_find = 0;                   // искомый резистор
int button_ohms = 0;                // запись ohms()
int button_found = 0;               //запись найденной кнопки
int midi_channel = 0;               //выбор  канала midi

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

//омметр
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

void calibiration()
{
  Serial.println("START"); //начало калибровки
  for (int i = 0; i < button_N; i++)
  {
    Serial.print("Press button ");
    Serial.println(i + 1);
    while (ohms() < 4000)
    {
      ESP.wdtFeed();
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
  // button_found = button_find();
}

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
    Serial.print("1 ");
    Serial.println(enc_value[0]);
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
    Serial.print("2 ");
    Serial.println(enc_value[1]);
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
    Serial.print("3 ");
    Serial.println(enc_value[2]);
  }
}

//midi button send
// void button_send()
// {
//   if (button_found > 0) //button control change send
//   {
//     Serial.println(button_found);
//     // MIDI.noteOn(midi_channel, button_found, 127);
//     while (button_find() == button_found)
//     {
//     }
//   }
// }

void setup()
{
  Serial.begin(115200);
  smartdelay(5000);
  // if (Serial)
  // {
  //   calibiration();
  // }
  attachInterrupt(digitalPinToInterrupt(5), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(4), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(12), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(14), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(13), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(16), isr, CHANGE);
}

void loop()
{
  // read();
  enc_send();
  // button_send();
  Serial.println(analogRead(analogPin));
  // Serial.println(ohms());
  // Serial.println(button_find());
}
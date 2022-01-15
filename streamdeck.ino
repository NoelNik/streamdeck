
//https://mido.readthedocs.io/en/latest/message_types.html
// 96	Data Bound Increment (+1)			cc control change
// 97	Data Bound Decrement (-1)
#include <Arduino.h>
#include <frequencyToNote.h>
#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <pitchToFrequency.h>
#include <pitchToNote.h>
#include <GyverEncoder.h>

#define calibiration_status 0 //запуск калибировки
#define CLK1 2
#define DT1 3
#define SW1 4
#define analogPin A0 //pin омметра

bool button_midi_states[3];         //switcherы
int buttons_res[3] = {20, 60, 220}; // резисторы на кнопках
int button_N = 3;                   //количество кнопок
int analog_ohms = 0;                // показания с аналог пина
const int Vin = 5;                  //опорное напряжение платы
float Vout = 0;                     // напряжение на резисторе
const float R_const = 1000;         // эталонный резистор
float R_find = 0;                   // искомый резистор
float ohm_count = 0;                // буфер для расчета сопротивления
int button_ohms = 0;                // запись ohms()
int button_found = 0;               //запись найденной кнопки
int midi_channel = 0;               //выбор  канала midi
int ohms_none = R_const - 200;      //сопротивление, пока не нажата кнопка
unsigned long middle = 0;
int k = 0;                    //k-костыль
Encoder enc1(CLK1, DT1, SW1); //установка энкодера 1pi

//delay
void smartdelay(int delaytime)
{
  unsigned long realtime = millis();
  while ((millis() - realtime) < delaytime)
  {
  }
}
//delay

// омметр
int ohms()
{
  analog_ohms = analogRead(analogPin);
  if (analog_ohms)
  {
    ohm_count = analog_ohms * Vin;
    Vout = (ohm_count) / 1024.0;
    ohm_count = (Vout / Vin);
    R_find = R_const * ohm_count;
  }
  return int(R_find);
}
// омметр

//среднее значение омметра
int middle_oms()
{

  middle = 0;
  for (int i = 0; i < 5; i++) //считывание среднего
  {

    if (ohms() > 600)
    {

      middle += ohms();
      smartdelay(10);
    }
    middle = middle / 5;
    return middle;
  }
}
  //среднее значение омметра

  void calibiration()
  {
    Serial.println("START"); //начало калибровки
    for (int i = 0; i < button_N; i++)
    {
      Serial.print("Press button ");
      Serial.println(i + 1);
      while (ohms() > 600)
      {
      }
      middle = 0;
      for (int i = 0; i < 5; i++) //считывание среднего
      {
        middle += ohms();
        smartdelay(100);
      }
      buttons_res[i] = middle / 5;
      Serial.println(middle / 5);
      smartdelay(400);
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
  //поиск кнопки

  //опрос всего
  void read()
  {
    button_found = button_find();
    // enc1.tick(); //считывание энкодера
  }
  //опрос всего

  //midi button send
  void button_send()
  {
    if (button_found > 0) //button control change send
    {
      // Serial.println(button_found);
      controlChange(midi_channel, 64 + button_found, 127);
      MidiUSB.flush();

      while (button_find() == button_found)
      {
      }
      controlChange(midi_channel, 20 + button_found, 0);
      MidiUSB.flush();
    }
  }
  //midi button send

  //midi control change message
  void controlChange(byte channel, byte control, byte value)
  {
    midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
    MidiUSB.sendMIDI(event);
  }
  //midi control change message

  // 96	Data Bound Increment (+1)			cc control change
  // 97	Data Bound Decrement (-1)
  void setup()
  {
    smartdelay(4000);
    Serial.begin(9600);
    if (calibiration_status)
    {
      calibiration();
    }
    // enc1.setType(TYPE1);    //установка типа энкодера (1 работает)
    // enc1.setTickMode(AUTO); //установка прерывания для энкодера
  }

  void loop()
  {
    // Serial.println(middle_oms());
    read();
    button_send();
  }
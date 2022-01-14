//https://mido.readthedocs.io/en/latest/message_types.html
// 96	Data Bound Increment (+1)			cc control change
// 97	Data Bound Decrement (-1)

#include <frequencyToNote.h>
#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <pitchToFrequency.h>
#include <pitchToNote.h>
#include <GyverEncoder.h>

#define CLK1 2
#define DT1 3
#define SW1 4
#define analogPin A0 //pin омметра

int buttons_res[3];        // резисторы на кнопках
int button_N = 3;          //количество кнопок
int analog_ohms = 0;       // показания с аналог пина
const int Vin = 5;         //опорное напряжение платы
float Vout = 0;            // напряжение на резисторе
const int R_const = 10000; // эталонный резистор
float R_find = 0;          // искомый резистор
float ohm_count = 0;       // костыль для расчета сопротивления
int button_ohms = 0;       // запись ohms()
int button_found = 0;      //запись найденной кнопки
int midi_channel = 0;      //выбор  канала midi
int ohms_none = 0;         //сопротивление, пока не нажата кнопка
unsigned long middle = 0;
Encoder enc1(CLK1, DT1, SW1); //установка энкодера 1

//delay
void smartdelay(int delaytime)
{
  int realtime = millis();
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
    ohm_count = (Vin / Vout) - 1;
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
    middle += ohms();
    smartdelay(40);
  }
  middle = middle / 5;
  return
}
//среднее значение омметра

void calibiration()
{

  ohms_none = ohms() *2;
  for (int i = 0; i < button_N; i++)
  {
    Serial.println("START"); //начало калибровки
    smartdelay(1000);
    Serial.println("Press button ");
    Serial.println(i + 1);
    while (ohms() < ohms_none)
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
    if ((abs(button_ohms) / buttons_res[i]) < 1.3)
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

// 96	Data Bound Increment (+1)			cc control change
// 97	Data Bound Decrement (-1)
void setup()
{
  smartdelay(4000);
  Serial.begin(9600);
  calibiration();
  // enc1.setType(TYPE1);    //установка типа энкодера (1 работает)
  // enc1.setTickMode(AUTO); //установка прерывания для энкодера
}

void loop()
{
  read();               //опрос
  if (button_found > 0) //button control change send
  {
    MidiUSB.controlChange(midi_channel, 64 + button_found, 127);
    MidiUSB.controlChange(midi_channel, 64 + button_found, 0);
    button_found = 0;
  }
}

//midi функции
void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
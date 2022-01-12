
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

void smartdelay(int delaytime)
{
  int realtime = millis();
  while ((millis() - realtime) < delaytime)
  {
  }
}
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
  //приделать delay (мб не надо)
}
// омметр
void calibiration()
{

  ohms_none = ohms() + ohms() * 2;
  for (int i = 0; i < button_N; i++)
  {
    Serial.println("START"); //начало калибровки
    delay(1000);
    Serial.println("Press button ");
    Serial.println(i + 1);
    while (ohms() < ohms_none)
    {
    }
    middle = 0;
    for (int i = 0; i < 5; i++) //считывание среднего
    {
      middle += ohms();
      delay(100);
    }
    buttons_res[i] = middle / 5;
    Serial.println(middle / 5);
    delay(400);
  }
  Serial.println("buttons");
  for (int i = 0; i < 3; i++)
  {
    Serial.println(buttons_res[i]);
  }
  delay(1000);
}

// поиск кнопки
int button_find()
{
  middle = 0;
  for (int i = 0; i < 5; i++) //считывание среднего
  {
    middle += ohms();
    delay(40);
  }
  button_ohms = middle / 5;
  int i = 0;
  while (i < button_N) //поиск кнопки
  {
    if ((abs(button_ohms) / buttons_res[i]) < 1.5)
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

//contol send для кнопок
void button_send()
{
  if (button_found != 0)
  {
  }
  else
  {
    for (int i = 0; i < button_N; i++)
    {
    }
  }
  button_found = 0;
}
//contol send для кнопок

void setup()
{
  delay(4000);
  Serial.begin(9600);
  calibiration();
  // enc1.setType(TYPE1);    //установка типа энкодера (1 работает)
  // enc1.setTickMode(AUTO); //установка прерывания для энкодера
}

void loop()
{
  // Serial.println(ohms());
  if (ohms() > ohms_none)
  {
    Serial.println(button_find());
  }
  // Serial.println(ohms());
  // read();
}

//midi функции
void controlChange(byte channel, byte control, byte value)
{
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
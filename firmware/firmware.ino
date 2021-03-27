#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

const uint8_t VOLTAGE_CHANNEL = 0;
const uint8_t CURRENT_CHANNEL = 1;
const uint8_t PIN_PWM = 9;
const uint8_t pwm_1a = 128;
const uint8_t pwm_2a = 255;

float voltage = 0.0f;
float current = 0.0f;

int voltage_raw;
int current_raw;

bool show_realtime = true;

void setup() {
  // put your setup code here,to run once:
  Wire.begin();
  Serial.begin(115200);
  // 0.512V full scale

}

int16_t measurement_channel(uint8_t channel)
{
  int32_t sum = 0;
  uint8_t count = 16;

  for(uint8_t i=0; i<count; ++i)
  {
    sum += static_cast<int16_t>(ads.readADC_SingleEnded(channel));
  }
  return sum/count;
}
void measurement()
{
    ads.setGain(GAIN_FOUR);
  voltage_raw = measurement_channel(VOLTAGE_CHANNEL);
    ads.setGain(GAIN_EIGHT);
  current_raw = measurement_channel(CURRENT_CHANNEL);

  voltage = voltage_raw*0.000491f*2.f;
  current = current_raw*0.000161f;
}

void battery_test()
{
  float v_1a, v_2a;
  float c_1a, c_2a;
  Serial.println(F("Battery test"));
  // set current 1A
  analogWrite(PIN_PWM, pwm_1a);
  delay(100);
  measurement();
  v_1a = voltage;
  c_1a = current;
  
  // set current 2A
  analogWrite(PIN_PWM, pwm_2a);
  delay(100);
  measurement();
  v_2a = voltage;
  c_2a = current;
  analogWrite(PIN_PWM, 0);
  Serial.print(F("R1A:"));
  Serial.print(v_1a);
  Serial.print(F(","));
  Serial.println(c_1a);
  Serial.print(F("R2A:"));
  Serial.print(v_2a);
  Serial.print(F(","));
  Serial.println(c_2a);
  float diff = c_2a-c_1a;
  if( diff > 1e-5 )
  {
    Serial.print(F("Rdiff:"));
    Serial.println((v_1a-v_2a)/diff);
  }
  else
  {
    Serial.println(F("Error:Current diff too low"));
  }
}

void handle_command(const char *cmd)
{

  if( strcmp_P(cmd, PSTR("btest")) == 0 )
  {
    battery_test();
  }
  else if( strcmp_P(cmd, PSTR("on_1a")) == 0 )
  {
    analogWrite(PIN_PWM, pwm_1a);
  }
  else if( strcmp_P(cmd, PSTR("on_2a")) == 0 )
  {
    analogWrite(PIN_PWM, pwm_2a);
  }
  else if( strcmp_P(cmd, PSTR("off_1a")) == 0 )
  {
    analogWrite(PIN_PWM, 0);
  }
  else if( strcmp_P(cmd, PSTR("off_2a")) == 0 )
  {
    analogWrite(PIN_PWM, 0);
  }
  else if( strcmp_P(cmd, PSTR("rt")) == 0 )
  {
    show_realtime = true;
  }
  else if( strcmp_P(cmd, PSTR("nort")) == 0 )
    show_realtime = false;
  else if( strcmp_P(cmd, PSTR("PING")) == 0 )
    Serial.println(F("PONG"));
  else if( strstr_P(cmd, PSTR("pwm")) )
  {
    char *arg = strchr(cmd, ':');
    uint8_t pwm = atoi(arg+1);
    Serial.print("pwma:");
    Serial.println(pwm);
    analogWrite(PIN_PWM, pwm);
  }
  else
  {
    Serial.print(F("Error: available cmds - btest, on_1a, on_2a, off_1a, off_2a,"));
    Serial.println(F("rt,nort,pwm"));
  }

}

void handle_serial()
{
  static char cmd_buffer[8];
  static uint8_t pos = 0;
  while(Serial.available())
  {
    char ch = Serial.read();
   cmd_buffer[pos] = ch;
   if( ch == '\r' || ch == '\n')
   {
    cmd_buffer[pos] = 0;
    handle_command(cmd_buffer);
    pos = 0;
    Serial.read(); // read out
   } else
   pos++;
   if( pos == sizeof(cmd_buffer) )
   {
    pos = 0;
   }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(show_realtime)
  {
    measurement();
    Serial.print(F("meas:"));
    Serial.print(voltage);
    Serial.print(',');
    Serial.println(current);
  }

  handle_serial();
}

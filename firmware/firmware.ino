#include <Wire.h>
#include <Adafruit_ADS1015.h>

const char help_string[] PROGMEM =
"Battery tester\n"
"Available commands:\n"
"\thelp - show this help\n"
"\tbtest - start battery resistance measurement\n"
"\ton_1a,on_2a,off_1a,off_2a - on/off 1a/2a current\n"
"\trt,nort - show runtime values\n"
"\tpwm - set pwm value (set current)\n"
"\tload_on, load_off - on/off external load relay\n"
;

static uint8_t i2cread(void) {
#if ARDUINO >= 100
  return Wire.read();
#else
  return Wire.receive();
#endif
}
static void i2cwrite(uint8_t x) {
#if ARDUINO >= 100
  Wire.write((uint8_t)x);
#else
  Wire.send(x);
#endif
}


static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite((uint8_t)reg);
  i2cwrite((uint8_t)(value >> 8));
  i2cwrite((uint8_t)(value & 0xFF));
  Wire.endTransmission();
}
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite(reg);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)2);
  return ((i2cread() << 8) | i2cread());
}


class Fast_ADS1115: public  Adafruit_ADS1115
{
  public:
    Fast_ADC1115(uint8_t i2cAddress)
    {
      m_i2cAddress = i2cAddress;
      m_conversionDelay = 2;
      m_bitShift = 0;
    }
uint16_t readADC_SingleEnded(uint8_t channel) {
  if (channel > 3) {
    return 0;
  }

  // Start with default values
  uint16_t config =
      ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
      ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
      ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
      ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
      7<<5 |   // 1600 samples per second (default)
      ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel) {
  case (0):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
    break;
  case (1):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
    break;
  case (2):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
    break;
  case (3):
    config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
    break;
  }

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(m_conversionDelay);
  return readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
}
};

Fast_ADS1115 ads;

const uint8_t VOLTAGE_CHANNEL = 0;
const uint8_t CURRENT_CHANNEL = 1;
const uint8_t PIN_PWM = 9;
const uint8_t PIN_LOAD = 3;
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
  Serial.println("started");
  pinMode(PIN_LOAD, OUTPUT);
  // 0.512V full scale

  TCCR1B &= 0xF8;
  TCCR1B |= 1;

}

int16_t measurement_channel(uint8_t channel)
{
  int32_t sum = 0;
  uint8_t count = 4;

  for(uint8_t i=0; i<count; ++i)
  {
    sum += static_cast<int16_t>(ads.readADC_SingleEnded(channel));
  }
  return sum/count;
}
void measurement()
{
    ads.setGain(GAIN_FOUR); // 1.024
  voltage_raw = measurement_channel(VOLTAGE_CHANNEL);
    ads.setGain(GAIN_FOUR); // 1.024
  current_raw = measurement_channel(CURRENT_CHANNEL);

  const float voltage_coef = 0.000491f*2.f;
  const float current_coef = 0.000161f*2.f;

  voltage = voltage_raw*voltage_coef;
  current = current_raw*current_coef;
}

void battery_test()
{
  float v_1a, v_2a;
  float c_1a, c_2a;
  Serial.println(F("Battery test"));
  // set current 1A
  analogWrite(PIN_PWM, pwm_1a);
  delay(10);
  measurement();
  v_1a = voltage;
  c_1a = current;
  
  // set current 2A
  analogWrite(PIN_PWM, pwm_2a);
  delay(10);
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
  else if( strcmp_P(cmd, PSTR("load_on")) == 0 )
  {
    digitalWrite(PIN_LOAD, HIGH);
  }
  else if( strcmp_P(cmd, PSTR("load_off")) == 0 )
  {
    digitalWrite(PIN_LOAD, LOW);
  }
  else if( strcmp_P(cmd, PSTR("help")) == 0 )
  {
    Serial.println(reinterpret_cast<const __FlashStringHelper*>(help_string));
  }
  else
  {
    Serial.print(F("Error: available cmds - help, btest, on_1a, on_2a, off_1a, off_2a,"));
    Serial.println(F("rt,nort,pwm, load_on, load_off"));
  }

}

void handle_serial()
{
  static char cmd_buffer[12];
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

#include<SPI.h>
#include<math.h>
#include<SoftwareSerial.h>

#define RxD 4
#define TxD 5
#define SQUARE_WAVE_FREQ_FACTOR 30.0
#define RAMP_WAVE_FREQ_FACTOR 0.1
#define TRIANGLE_WAVE_FREQ_FACTOR 8
#define SINE_WAVE_FREQ_FACTOR 5

SoftwareSerial ESP32(RxD,TxD);

volatile uint32_t phAcc = 0;

int sample_rate = 16000000;
volatile bool send_sample;

const int PIN_CS = 10;
const int GAIN_1 = 0x1;
const int GAIN_2 = 0x0;


void setup() {
  
  TCCR1A = 0;
  TCCR1B = (1<<WGM12) | (1<<CS10);
  OCR1A = F_CPU/(2*sample_rate) - 1;
  TIMSK1 = (1<<OCIE1A);

  pinMode(PIN_CS, OUTPUT);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  Serial.begin(115200);
  ESP32.begin(115200);
  ESP32.flush();

}

void setOutput(unsigned int val)
{
  byte lB = val & 0xff;
  byte hB = ((val >> 8) & 0xff) | 0x10;

  PORTB &= 0xfb;
  SPI.transfer(hB);
  SPI.transfer(lB);
  PORTB |= 0x4;
}

void setOutput(byte channel, byte gain, byte shutdown, unsigned int val)
{
  byte lB = val & 0xff;
  byte hB = ((val >> 8) & 0xff) | channel << 7 | gain << 5 |
shutdown << 4;

  PORTB &= 0xfb;
  SPI.transfer(hB);
  SPI.transfer(lB);
  PORTB |= 0x4;
}

unsigned int dacVal;
int k = 1;
int i=0;
int flag = 1;

void loop() 
{
  i=0;
  float amplitude;
  float frequency=0;
  char type;
  float dutyCycle;
  String parameters = "";
  Serial.println("NEW INPUT");
  if(ESP32.available())
  {
    Serial.println("NEW INPUT2");
    char c;
    while(ESP32.available())
    {
      c = ESP32.read();
      parameters+=c;
    }   
    Serial.println(parameters);
  

  type = parameters[0]; 
  char amp[5]; 
  char freq[8];    
  char dc[4];    
   
  for(int j = 0; j<4 ; j++)
  {
    amp[j]=parameters[j+1];
  }
  amp[4] = 0;
  for(int j = 0; j<3 ; j++)
  {
    dc[j]=parameters[j+5];
  }
  dc[3] = 0;
  for(int j = 0; j<7 ; j++)
  {
    freq[j]=parameters[j+8];
  }
  freq[7]=0;




  
  
  for(int l=0;freq[l]!='\0';l++)
  {
    frequency = frequency*10 + (freq[l]-'0');
  }



  switch (type) {
    case '1':      //sine
      
       amplitude = atoi(amp) / 1000.0;
       frequency = frequency;
       break;

    case '2':     //square
      
      amplitude = atoi(amp) / 1000.0;
      dutyCycle = atof(dc);
      dutyCycle /= 100.0;
      frequency = frequency;
      break;
    
     case '3':     //triangle
    
      amplitude = atoi(amp) / 1000.0;
      frequency = frequency;
      break;

    case '4':     //ramp
      
      amplitude = atoi(amp) / 1000.0;
      frequency = frequency;
      dutyCycle = atof(dc);
      dutyCycle/=100.0;
      
      break;

  
  }

  }


  uint32_t tuningWord = pow(2, 32) * frequency / (sample_rate);
  uint16_t dacval_amp = amplitude*4095/5;


  while(ESP32.available() == 0)
  {
    if(send_sample)
    {
      uint8_t count = phAcc>>20;

      phAcc += tuningWord;

      send_sample = false;
      
      if(type == '1')
      {
        setOutput(round((sin((TWO_PI*count/256)))*(2047.5)+(2047)));
      }
      else if(type == '2')
      {
        setOutput(count>round(dutyCycle*256.0)? dacval_amp:0);
       
      }
      else if(type == '3')
      {
        if(flag>=0)
        {
          setOutput(i);
          i+=64;
          if(i>=dacval_amp)
          {
            flag*=-1;
            i=0;
          }
        }
        else if(flag<0)
        {
          setOutput(dacval_amp-i);
          i+=64;
          if(i>=dacval_amp)
          {
            i=0;
            flag*=-1;
            break;
          }
        }
      }
      else if(type == '4')
      {
        if(dutyCycle == 1000)
        {
          setOutput(i);
          i++;
          if(i>dacval_amp)
            i=0;
        }
        else if(dutyCycle == 0)
        {
          setOutput(i);
          i--;
          if(i<0)
            i=dacval_amp;
        }
      }
    }   
  }
}

ISR(TIMER1_COMPA_vect)
{
  send_sample = true;
}

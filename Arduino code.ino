//Petrusan George-Cristian
#define HISTEREZA 0.5
#define TEMPERATURA 35
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1


int initiala = -1;

void reset7segment()
{
  PORTD &= 0x0F;
  PORTB &= 0xF8;
}

void set7segment(int x)
{
  switch(x)
  {
    
    //litera P
    case 0:
    reset7segment();
    PORTD |= 0B00110000;
    PORTB |= 0B00000111;
    break;
    
    //litera G
    case 1:
    reset7segment();
    PORTD |= 0B11010000;
    PORTB |= 0B00000011;
    break;
    
    //litera C
    case 2:
    reset7segment();
    PORTD |= 0B10010000;
    PORTB |= 0B00000011;
    break;
  }
}
void setuptimer0()
{
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR0A|=(1<<WGM01); //CTC
  TCCR0B|=(1<<CS02) | (1<<CS00); // prescaler 256
  TIMSK0|=(1<<OCIE0A); //interrupt enable
  OCR0A = 255;

}

void setuptimer1()
{
  TCCR1A = 0;
  TCCR1B |= 1 << CS10 | 1 << CS12; // prescalerul 1024
  TIMSK1 |= 1 << OCIE1A; // interrupt enable
  OCR1A = 15625;  // F_CPU / Prescaler -> 1s
}

void setuptimer2()
{
  TCCR2A = 0;
  TCCR2B = 0;
  TCCR2A |= (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Fast PWM
  TCCR2B |= (1 << CS22) | (1 << CS20);   // Prescaler 1024
  OCR2A = 0;
}

void adc_init()//adc initialization
{
  /*ADCSRA |= ((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0));
  //set division factor between system clock
  //frequency and the input clock to the ADC‐ 128
  ADMUX |= (1<<REFS0);//AVcc with external capacitor at Aref pin
  ADCSRA |= (1<<ADEN);//enable ADC
  ADCSRA |= (1<<ADSC);//ADC start conversion*/

  ADCSRA |= ((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0));
  ADCSRA |= 1 << ADEN; // Alimentarea perifericului.
  ADCSRA |= 1 << ADIE; // Pornim intreruperile ADC.
  ADMUX = 0;
  ADMUX |= 1 << REFS0; // Setam ca referinta 5V.

  ADCSRA |= 1 << ADSC; // Pornim prima conversie.
}
uint16_t read_adc(uint8_t channel)
{
  ADMUX &= 0xF0;//set input AO to A5
  ADCSRA |= 1<<ADIE; //enable interrupt 
  ADMUX |= channel;//select chanel AO to A5
  ADCSRA |= (1<<ADSC);//start conversion
  while(ADCSRA & (1<<ADSC));//wait wile adc conversion are not updated
  return ADC;//read and return voltage
}

void USART_Init(unsigned int ubrr)
{
  /*Set baud rate */
  UBRR0H = (unsigned char)(ubrr>>8);
  UBRR0L = (unsigned char)ubrr;
  /* Enable receiver and transmitter + intrerupere*/
  UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
  /* Set frame format: 8data, 2stop bit */
  UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

unsigned char USART_Receive(void)
{
  /* Wait for data to be received */
  while (!(UCSR0A & (1<<RXC0)));
  /* Get and return received data from buffer */
  return UDR0;
}

void USART_Transmit(unsigned char data)
{
  /* Wait for empty transmit buffer */
  while (!(UCSR0A & (1<<UDRE0)));
  /* Put data into buffer, sends the data */
  UDR0 = data;
}

int main(void)
{
  cli();
  DDRD = 0xFF;
  DDRB = 0xFF;

  USART_Init(MYUBRR);
  adc_init();
  setuptimer0();
  setuptimer1();
  setuptimer2();
  
  sei();

  
  while(1)
  {
      
  }
}

int contor = 0;
int currState;

ISR(TIMER0_COMPA_vect)
{
 
  switch(currState)
  {
    case 0:
      contor++;
      if (contor >= 30)
        {
          currState = 1;
          contor = 0;
        }
      break;
    case 1:
      OCR2A+=8;
      if (OCR2A >= 245)
        {
          currState = 2;
          OCR2A = 255;
          }
      break;
    case 2:
      contor++;
      if (contor >= 30)
      {
        currState = 3;
        contor = 0;
      }
      break;
    case 3:
      OCR2A-=8;
      if (OCR2A <= 10)
      {
        currState = 0;
        OCR2A = 0;
      }
      break;
  }
  TCNT0 = 0;
}

ISR(TIMER1_COMPA_vect)
{
  if(initiala>=2)
    initiala = -1;
  ++initiala; 
  TCNT1 = 0; // Fara CTC
  PORTB ^= (1<<5);
  set7segment(initiala);
  //Serial.println(initiala);
}

ISR(USART_RX_vect)
{
  char aux = UDR0;
  if(aux == 'A' || aux == 'a')
    PORTD |= 1<<PD2;
    else if (aux == 'S' || aux == 's')
    PORTD &= ~(1<<PD2);
}
volatile int numar_CAN;

ISR(ADC_vect)
{
  numar_CAN = ADC;
  float volti = (numar_CAN * 5) / 1023.0;
  int intvolti = volti*100;
  float temperatura = (70/1023.0 * numar_CAN) - 20;
  int inttemp = temperatura * 100;
  int tempfractionar = inttemp%100, tempintreg = inttemp/100;
  if (inttemp < 0)
    tempfractionar *= -1;

  char mesaj[100];
  memset(mesaj, 0, sizeof(mesaj));
  sprintf(mesaj, "CAN:   %d\tVolti: %d.%dV\tTemp: %d.%d *C\n", numar_CAN, intvolti/100, intvolti%100, tempintreg, tempfractionar);
  
  for (int i = 0; i < strlen(mesaj); i++)
    USART_Transmit(mesaj[i]);

  if (temperatura < TEMPERATURA - HISTEREZA)
    PORTB &= ~(1<<4);
  else if (temperatura > TEMPERATURA + HISTEREZA)
    PORTB |= 1<<4;
  
  ADMUX |= 1 << REFS0; //referinta 5V
  ADCSRA |= 1<<ADSC; //start conversie
}

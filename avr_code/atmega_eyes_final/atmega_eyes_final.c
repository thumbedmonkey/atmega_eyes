#define F_CPU 16000000UL // 16MHz clock
 
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8) // Calculate the value needed for the CTC match value in OCR1A
#define	ATOMIC_BLOCK

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define CLK_HIGH()  PORTB |= (1<<PB2)
#define CLK_LOW()   PORTB &= ~(1<<PB2)
#define CS_HIGH()   PORTB |= (1<<PB1)
#define CS_LOW()    PORTB &= ~(1<<PB1)
#define DATA_HIGH() PORTB |= (1<<PB0)
#define DATA_LOW()  PORTB &= ~(1<<PB0)
#define INIT_PORT() DDRB |= (1<<PB0) | (1<<PB1) | (1<<PB2)

const __flash uint8_t eye[8] = {
        0b00111100,
        0b01000010,
        0b10000001,
        0b10011001,
        0b10011001,
        0b10000001,
        0b01000010,
        0b00111100};

const __flash uint8_t eye1[8] = {
        0b00111100,
        0b01000010,
        0b10000001,
        0b10110001,
        0b10110001,
        0b10000001,
        0b01000010,
        0b00111100};

const __flash uint8_t eye2[8] = {
        0b00111100,
        0b01000010,
        0b10000001,
        0b10001101,
        0b10001101,
        0b10000001,
        0b01000010,
        0b00111100};

void spi_send(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++, data <<= 1)
    {
	CLK_LOW();
	if (data & 0x80)
	    DATA_HIGH();
	else
	    DATA_LOW();
	CLK_HIGH();
    }
    
}

void max7219_writec(uint8_t high_byte, uint8_t low_byte)
{
    CS_LOW();
    spi_send(high_byte);
    spi_send(low_byte);
    CS_HIGH();
}

void max7219_clear(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
	max7219_writec(i+1, 0);
    }
}

void max7219_init(void)
{
    INIT_PORT();
    // Decode mode: none
    max7219_writec(0x09, 0);
    // Intensity: 3 (0-15)
    max7219_writec(0x0A, 1);
    // Scan limit: All "digits" (rows) on
    max7219_writec(0x0B, 7);
    // Shutdown register: Display on
    max7219_writec(0x0C, 1);
    // Display test: off
    max7219_writec(0x0F, 0);
    max7219_clear();
}


uint8_t display[8];

void update_display(void)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
	max7219_writec(i+1, display[i]);
    }
}

void image(const __flash uint8_t im[8])
{
    uint8_t i;

    for (i = 0; i < 8; i++)
	display[i] = im[i];
}

void set_pixel(uint8_t r, uint8_t c, uint8_t value)
{
    switch (value)
    {
    case 0: // Clear bit
	display[r] &= (uint8_t) ~(0x80 >> c);
	break;
    case 1: // Set bit
	display[r] |= (0x80 >> c);
	break;
    default: // XOR bit
	display[r] ^= (0x80 >> c);
	break;
    }
}

volatile unsigned long timer1_millis;
long milliseconds_since1 = 0;
long milliseconds_since2 = 1000;
long milliseconds_since3 = 2000;
long milliseconds_since4 = 3000;
long waitTime = 4000;

ISR (TIMER1_COMPA_vect)
{
    timer1_millis++;
}

unsigned long millis()
{
  unsigned long millis_return;

  //ATOMIC_BLOCK(ATOMIC_FORCEON) { // Ensure this cannot be disrupted
  millis_return = timer1_millis;
   // }
 
  return millis_return;
}

void flash_led ()
{
  if (millis() >= milliseconds_since1) {
    image(eye);
    update_display();
    milliseconds_since1 += waitTime;
    }

  if (millis() >= milliseconds_since2) {
    image(eye1);
    update_display();
    milliseconds_since2 += waitTime;
    }

  if (millis() >= milliseconds_since3) {
    image(eye);
    update_display();
    milliseconds_since3 += waitTime;
    }

  if (millis() >= milliseconds_since4) {
    image(eye2);
    update_display();
    milliseconds_since4 += waitTime;
    }
}
 

int Pressed = 0;
int Pressed_Confidence_Level = 0; //Measure button press cofidence
int Released_Confidence_Level = 0; //Measure button release confidence

void button(void)
{
    //if (bit_is_clear(PINB, 0))
    if (bit_is_clear(PIND, 4))
    {
      Pressed_Confidence_Level ++; //Increase Pressed Conficence
      Released_Confidence_Level = 0; //Reset released button confidence since there is a button press
      if (Pressed_Confidence_Level >500) //Indicator of good button press
      {
        if (Pressed == 0)
        {
          // Do stuff here
          //PORTB ^= 1 << PINB3; // Toggle LED
          PORTD ^= 1 << PIND7; // Toggle LED
          Pressed = 1;
        }
        //Zero it so a new pressed condition can be evaluated
        Pressed_Confidence_Level = 0;
      }
    }

    else
    {
      Released_Confidence_Level ++; //This works just like the pressed
      Pressed_Confidence_Level = 0; //Reset pressed button confidence since the button is released
      if (Released_Confidence_Level >500)
      {
        Pressed = 0;
        Released_Confidence_Level = 0;
      }
    }
    //return 0;
}

void setColor(int red, int green, int blue)
{
  OCR2B = 255 - red; // Set PWM rate 0 - 255 RED
  OCR0B = 255 - green; // Set PWM rate 0 - 255 GREEN
  OCR0A = 255 - blue; // Set PWM rate 0 - 255 BLUE
}

int main(void)
{
  DDRD |= 1 << PD7; // Set PB2 as output
  DDRD &= ~(1 << PD4); // Set PB2 as input
  PORTD |= 1 << PD4; // Set PB2 as input

  DDRD |= (1 << PD3); // PD3/pin5 is now an output
  DDRD |= (1 << PD5); // PD5/pin11 is now an output
  DDRD |= (1 << PD6); // PD/pin12 is now an output

  TCCR2A =  (1 << COM2B1) | (1<<COM0A1) | (1<<WGM00) | (1 << WGM20);
  TCCR2B = (1 << CS22) | (1<<CS01);
  TCCR0A = (1<<WGM00) | (1<<COM0A1) | (1 << COM2B1) | (1 << WGM20);
  TCCR0B = (1<<CS01);


  TCCR1B |= (1 << WGM12) | (1 << CS11); // CTC mode, Clock/8
 
  OCR1AH = (CTC_MATCH_OVERFLOW >> 8); // Load the high byte, then the low byte into the output compare
  OCR1AL = (unsigned char) CTC_MATCH_OVERFLOW; // Added (unsigned char)

  TIMSK1 |= (1 << OCIE1A); // Enable the compare match interrupt

  sei(); // Now enable global interrupts
    
  //uint8_t i;
  
  max7219_init();
  while (1)
  {
  button();
  flash_led();
  //setColor(0, 255, 0); // GREEN
  //setColor(0, 0, 255); // BLUE
  setColor(255, 0, 0); // RED
  }
}


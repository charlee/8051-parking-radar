#include <8052.h>


#define LED_R P3_7
#define LED_G P3_6
#define LED_B P3_5

#define TRIG P3_4

#define SEGS P1
#define GROUP P2


// #define T0_INTERVAL 12000
// #define reset_T0() { TL0 = (65536 - T0_INTERVAL) % 256; TH0 = (65536 - T0_INTERVAL) / 256; }
#define reset_T0() { TL0 = 0; TH0 = 0; }


// 4ms
#define T1_INTERVAL 2000
#define reset_T1() { TL1 = (65536 - T1_INTERVAL) % 256; TH1 = (65536 - T1_INTERVAL) / 256; }



__code unsigned char sevenseg_hex[] = {
  0x3F, 0x06, 0x5B,  0x4F,
  0x66, 0x6D, 0x7D, 0x07,
  0x7F, 0x6F, 0x77, 0x7C,
  0x39, 0x5E, 0x79, 0x71
};


// distance in centimeter, maximum 400 (HC-SR01 limit)
unsigned int distance = 400;

unsigned char digits[4];


void show() {
  digits[2] = distance % 10;
  digits[1] = (distance / 10) % 10;
  digits[0] = distance / 100;
}

void set_leds() {
  if (distance > 150) {
    LED_G = 0;
    LED_R = 1;
  } else if (distance > 80) {
    LED_G = 0;
    LED_R = 0;
  } else {
    LED_G = 1;
    LED_R = 0;
  }
}


void init() {
  // Turn off all digits
  GROUP = 0xff;
  SEGS = 0xff;

  // Turn off LEDs
  LED_G = 1;
  LED_R = 1;
  LED_B = 1;

  TRIG = 0;

  // setup timer 0 and timer 1
  TMOD = 0x11;

  EA = 1;
  ET0 = 1;    // enable timer0
  ET1 = 1;    // enable timer1
  TCON = 0x01;  // set both INT0 and INT1 to trigger on falling edge

  // Start timer 1
  TR1 = 1;
}

void delay(int n){
  while (n--);
}

void start_detect_distance() {

  // reset T0 before sending signal to avoid extra cycles caused by the assignment
  reset_T0();

  // disable display timer (timer1) to make sure timing is correct.
  // ET1 = 0;
  TRIG = 1;
  __asm
  nop
  nop
  nop
  nop
  nop
  __endasm;
  TRIG = 0;
  // ET1 = 1;

  EX0 = 1;
  TR0 = 1;    // start timer1 for timeout
}



void main() {

  init();

  while(1) {
    show();
    set_leds();
    delay(50000);
    start_detect_distance();
  }

}

/**
 * Ultrasound sensor echo input
 */
void echo_stop_trigger() __interrupt 0 {

  // echo triggered, stop timer0 and output distance
  EX0 = 0;
  TR0 = 0;

  distance = (TH0 * 256 + TL0) / 29;
}

/**
 * timer0 is used to time the ultrasonic echo.
 */
void timer0() __interrupt 1 {
  EX0 = 0;
  TR0 = 0;
}


unsigned char scan_pos = 0;

/**
 * timer1 is used to handle the 7seg scanning.
 */
void timer1() __interrupt 3 {

  SEGS = 0xff;

  // GROUP connections are G3=P3.0, G2=P3.2, G1=P3.4
  GROUP = ~(0x01 << scan_pos);

  if (scan_pos == 0) {
    // show blinking dot
    SEGS = ~(sevenseg_hex[digits[scan_pos]] | 0x80);
  } else {
    SEGS = ~sevenseg_hex[digits[scan_pos]];
  }

  scan_pos++;
  if (scan_pos == 3) {
    scan_pos = 0;
  }

  reset_T1();
}

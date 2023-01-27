/*
Chris Blangiardi - 4.29.22
Junior Design Project - COVID-19 Distance System
*/

#include "avr/io.h"
#include "util/delay.h"
#include "i2c.h"
#include "SSD1306.h"

#define F_CPU 16000000

#define echoPin PB0  // HC-SR04 echo enable - pin 14 of ATmega328P
#define trigPin PB1  // HC-SR04 trig enable - pin 15 of ATmega328P

#define DIG4 PB2  // DIG4 enable - pin 17 of ATmega328P
#define DIG3 PB3  // DIG3 enable - pin 16 of ATmega328P
#define DIG2 PB4  // DIG4 enable - pin 15 of ATmega328P

#define rLED PC0  // Green LED enable - pin 27 of ATmega328P
#define gLED PC1  // Red LED enable - pin 28 of ATmega328P
#define button PC2  // Push button enable - pin 25 of ATmega328P

unsigned char violations, start_shift;
int myCounter, v_count;

void disp3Digits(unsigned char, unsigned char, unsigned char);  // function to display digits on seven-seg
int get_distance();  // function to obtain distance from HC-SR04

int main(){

    DDRD = 0xFF;  // set all PORTD pins as output
    DDRC = (1 << rLED) | (1 << gLED) | ~(1 << button);  // set PORTC pins 0,1,3-5 as input/output
    DDRB = (1 << DIG2) | (1 << DIG3) | (1 << DIG4) | (1 << trigPin) | ~(1 << echoPin);  // set PORTB pins 0-2 as output

    PORTB &= ~(1 << trigPin);
    PORTC |= (1 << button);  // enable pull-up resistor

    myCounter = 0;  // initialize counter to track # of customers
    start_shift = 0;  // initialize our system to start in a not running state
    violations = 0;  // initialize counter to track an individual customer violation
    v_count = 0;  // initialize counter to track total # of customer violations

    OLED_Init();  // initialize the OLED
    OLED_Clear(); // clear OLED screen
    

    TCCR0A = 0;  // timer mode normal
    TCCR0B = 5;
    TCNT0 = 0;

    while (1){
        unsigned char digit1, digit2, digit3, feet, inches, distance, new_c;  // list of variables for further compuation

        // shift started message on button activation
        if ((PINC & (1<<button)) == 0){
            OLED_Clear();
            OLED_GoToLine(0);
            OLED_DisplayString("Shift Started. Enjoy!");
            start_shift = 1;
            _delay_ms(1000);
            OLED_Clear();
            OLED_GoToLine(0);
            OLED_DisplayString("Total Customer: ");
            OLED_DisplayNumber(10, myCounter, 2);
            OLED_GoToNextLine();
            OLED_GoToNextLine();
            OLED_DisplayString("Total Violations: ");
            OLED_DisplayNumber(10, v_count, 2);
        }

       // code to be executed on shift activation
        if (start_shift == 1){
           // record data when a person is within 10 ft
            if ((distance = get_distance()) < 96){
                myCounter += 1;
                new_c = 1;
                // keep calculating distance until customer leaves
                while (distance < 96){
                    feet = distance / 12;  // # of feet in calculated distance
                    inches = distance % 12;  // # of remaining inches in calculated distance

                    digit1 = inches % 10;  // ones position of inches
                    digit2 = inches / 10;  // tens position of inches
                    digit3 = feet;  // store # of feet in digit3

                    /*
                     * if a customer is within 6 feet warn them they are too close with a red LED, otherwise
                     * inform them to come close with a green LED
                     */
                    if (digit3 < 6){
                        PORTC |= (1<<rLED);  // turn on red LED
                        PORTC &= ~(1<<gLED);  // turn off green LED
                        violations = 1;  // increment violations counter
                    } else {
                        PORTC |= (1<<gLED);  // turn on green LED
                        PORTC &= ~(1<<rLED);  // turn off red LED
                    }

                    disp3Digits(digit1, digit2, digit3);  // display the previous calculated digits
                    distance = get_distance();  // get the current distance of customer
                }

            }
            /*
             * if an individual customer had a distance violation, increase total number of violations
             */
            if (violations == 1){
                v_count += 1;
                violations = 0;
            }
            
            /*
             * on button activation, end the current shift.
             * resets statistics and displays a message before turning off
             */
            if ((PINC & (1<<button)) == 0){
                start_shift = 0;
                OLED_Clear();
                OLED_GoToLine(0);
                OLED_DisplayString("Shift Ended!");     
                myCounter = 0;
                v_count = 0;
                _delay_ms(1000);  
                OLED_Clear();
            }

        }

        /* 
         * Updates the information printed on the screen. Only when a new customer is detected
         */
        if (new_c == 1){
            OLED_Clear();
            OLED_GoToLine(0);
            OLED_DisplayString("Total Customer: ");
            OLED_DisplayNumber(10, myCounter, 2);
            OLED_GoToNextLine();
            OLED_GoToNextLine();
            OLED_DisplayString("Total Violations: ");
            OLED_DisplayNumber(10, v_count, 2);
            new_c = 0;
        }

        PORTB = (1 << DIG2) | (1 << DIG3) | (1 << DIG4);  // turn the seven-seg off
        PORTC &= ~(1 << rLED);
        PORTC &= ~(1 << gLED);

    }

    return 0;
}

/*
 * Print the distance of the customer on the seven segment display
 */
void disp3Digits(unsigned char d1, unsigned char d2, unsigned char d3) {
    unsigned char ledDigits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67};
    unsigned char i;

    for (i = 0; i < 5; i++) { // this code will refresh at 5x30=150 mS
        PORTD = ledDigits[d3]; // 10's digit
        PORTB = (1 << DIG3) | (1 << DIG4); // enable DIG2, disable DIG3 & DIG4, 0000 0011
        _delay_ms(10);
        PORTD = ledDigits[d2] + 128; // 1's digit w decimal pt
        PORTB = (1 << DIG2) | (1 << DIG4); // enable DIG3, disable DIG2,DIG4  0000 0101
        _delay_ms(10);
        PORTD = ledDigits[d1]; // 0.1's digit
        PORTB = (1 << DIG2) | (1 << DIG3); // enable DIG4, disable DIG2,DIG3 0000 0110
        _delay_ms(10);
    }
    return;
}

/*
 * calculates the distance of the customer from the counter
 */
int get_distance(){
    long duration;
    unsigned int distance, RiseEdgeTime, FallEdgeTime;

    TCNT0 = 0x00;
    PORTB |= (1<<trigPin);
    _delay_us(10);
    PORTB &= ~(1<<trigPin);

    while ((PINB & (1 << echoPin)) == 0);
    RiseEdgeTime = TCNT0;

    while (!(PINB & (1 << echoPin)) == 0);
    FallEdgeTime = TCNT0;

    duration = (FallEdgeTime - RiseEdgeTime)*1.098;
    distance = duration * 0.3937;

    return distance;
}
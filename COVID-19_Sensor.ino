/*
* Chris Blangiardi - 3.11.22
* Junior Design Project - COVID-19 Distance System
*/

#include <SPI.h>  
#include <Adafruit_SSD1306.h>
#include "avr/io.h"
#include "util/delay.h"

#define DIG2 PB2  // enables DIG2. ATmega328P pin PB2/Arduino Uno pin 10
#define DIG3 PB1  // enables DIG3. ATmega328P pin PB1/Arduino Uno pin 9
#define DIG4 PB0  // enables DIG4. ATmega328P pin PB0/Arduino Uno pin 8

#define trigPin A0  // attach pin A0 Arduino to pin Trig of HC-SR04
#define echoPin A1  // attach pin A1 Arduino to pin Echo of HC-SR04

#define greenLED 12  // attach green LED to pin 12 of Arduino
#define redLED 13  // attach red LED to pin 13 of Arduino

#define button A5  // attach button to pin A5 of Arduino

Adafruit_SSD1306 display(128,64, &Wire, 4); 
unsigned char violations[400];
unsigned char start_shift, myCounter;

void disp3Digits(unsigned char, unsigned char, unsigned char);  // function to display digits to seven-seg

void setup() {
    DDRD = 0xFF;  // 7segment pins
    DDRB = (1 << DIG2) | (1 << DIG3) | (1 << DIG4);  // digit enables  


    // clear receptionist display on start up
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE); 
    display.display(); 
    
    myCounter = 0;  // track number of customers throughout shift 
    start_shift = 0;  // track if the shift has been started yet

    pinMode(button, INPUT_PULLUP);  // push-button setup
    
    pinMode(greenLED, OUTPUT);  // LED enables
    pinMode(redLED, OUTPUT);

    pinMode(trigPin, OUTPUT); // sets the trigPin as an OUTPUT
    pinMode(echoPin, INPUT); // sets the echoPin as an INPUT
}
    
void loop() {
    unsigned char digit1, digit2, digit3, LED, feet, inches, distance;
    unsigned char min_distance = 72;  // minimum distance of customers in inches
    
    // flash a message when shift is started by push button
    if (digitalRead(button) == LOW){
        start_shift = 1;
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Shift started. Enjoy!");
        display.println("");
        display.print("Total Customers: ");
        display.println(myCounter);
        display.display();
        _delay_ms(10);
    }

    // only execute the following code when the shift has been started
    if (start_shift == 1){
        // when a person is within 10 feet start recording data
        if ((distance = get_distance()) < 120){
            myCounter +=1; 
        
            while (distance < 120){  
                feet = distance / 12;  // customer distance in feet to the nearest inch
                inches = distance % 12; 
          
                digit1 = inches % 10;  // # of inches to display
                digit2 = inches / 10;  
                digit3 = feet;  // # of feet to display

                /*
                 * if a customer is more than six feet away indicate they can 
                 * come closer with a green LED otherwise indicate for them 
                 * to move back with a red LED 
                 */
                if (digit3 < 6){
                    digitalWrite(redLED, HIGH);
                    digitalWrite(greenLED, LOW);
                    LED = 5;
                    min_distance = distance;
                    violations[myCounter-1] = min_distance;
                } else {
                    digitalWrite(greenLED, HIGH);
                    digitalWrite(redLED, LOW);
                    LED = 4;
                }
            
                disp3Digits(digit1, digit2, digit3, LED);  // display the previous calculated digits
                distance = get_distance();  // get the current distance of customer
            }  
            
            // update the display with new customer information   
            display.clearDisplay(); 
            display.setCursor(0,0);
            display.print("Total Customers = ");
            display.println(myCounter);
            display.println("\nRecent Violations:");
            int i;
            if (myCounter > 4){
                i = myCounter - 5;
            } else {
                i = 0;
            }
            for (i; i<myCounter; i++){
                if (violations[i] != 0){
                    display.print(violations[i]/12);  // print feet
                    display.print("'");
                    display.print(violations[i]%12);
                    display.println("\"");   
                }                            
            }
            display.display();             
        }
        
        // end the shift
        if (digitalRead(button) == LOW){
            start_shift = 0;
            display.clearDisplay(); 
            display.setCursor(0,0);
            display.print("Shift Ended!");
            display.display();
            _delay_ms(1000);
            display.clearDisplay();
            display.display();
            myCounter = 0;
        }
      
    }  

    PORTB = (1 << DIG2) | (1 << DIG3) | (1 << DIG4);  // turn the seven-seg off
  
}


/* Illuminate 3 digits of the 4 digit 7 segment display
* The 7 segments (a, b, c, d, e, f, g) correspond to ATmega328P pins D0-D7, 
* Arduino Uno pins 0-6. Enable pins DIG2, DIG3, DIG4 corresspond to PB0, PB1, PB0,
* which correspondsd to Arduino Uno pins 8, 9, 10 */
void disp3Digits(unsigned char d1, unsigned char d2, unsigned char d3, unsigned char LED) {
    unsigned char ledDigits[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67};

    for (unsigned char i = 0; i < 5; i++) { //this code will refresh at 5x30=150 mS
        PORTD = ledDigits[d3]; //10's digit
        PORTB = (1 << DIG3) | (1 << DIG4) | (1 << LED); //enable DIG2, disable DIG3 & DIG4, 0000 0011
        _delay_ms(10);
        PORTD = ledDigits[d2] + 128; //1's digit w decimal pt
        PORTB = (1 << DIG2) | (1 << DIG4) | (1 << LED); //enable DIG3, disable DIG2,DIG4  0000 0101
        _delay_ms(10);
        PORTD = ledDigits[d1]; //0.1's digit
        PORTB = (1 << DIG2) | (1 << DIG3) | (1 << LED); //enable DIG4, disable DIG2,DIG3 0000 0110
        _delay_ms(10);
    }
    return;
}

/*
 * calculate the distance of the of customer
 */
int get_distance(){
    long duration;
    int distance;
  
    digitalWrite(trigPin, LOW);  // Clears the trigPin condition
    _delay_ms(5);
    digitalWrite(trigPin, HIGH);  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
    _delay_ms(10);
    digitalWrite(trigPin, LOW); 
    duration = pulseIn(echoPin, HIGH);  // Reads the echoPin, returns the sound wave travel time in microseconds
    distance = (duration * 0.034 / 2)*0.3937 ; // Speed of sound wave divided by 2 (converted to inches) 

    return distance;
}

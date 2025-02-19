/*******************************************************
This program was created by the
CodeWizardAVR V3.14 Advanced
Automatic Program Generator
? Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : Clock With RTC (Timer 2) And Alarm
Version : 1.0.0
Date    : 1/6/2025
Author  : Mehdi Salmanzadeh
Company : 
Comments: 

Chip type               : ATmega32A
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <mega32a.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>
#include <alcd.h>

// Global variables
unsigned char h = 0, m = 0, s = 0;
unsigned char alarm_h = 0, alarm_m = 0, alarm_s = 0;
char clock_buffer[17]; // Buffer for clock display
char alarm_buffer[17]; // Buffer for alarm display
bit alarm_set = 0; // Flag to indicate if alarm is set

// Function prototypes
void display_info(void);
void get_time(void);
void set_time(void);
void set_alarm(void);
void check_alarm(void);

// Timer2 overflow interrupt handler
interrupt [TIM2_OVF] void timer2_ovf_isr(void)
{
    static unsigned int timer2_overflow_count = 0;
    
    // With 32.768kHz crystal, prescaler 128, and 8-bit timer:
    // Frequency = 32768/128 = 256Hz
    // Timer overflows every 256 counts (8-bit timer)
    // So we need 256Hz/256 = 1 overflow per second
    timer2_overflow_count++;
    if(timer2_overflow_count >= 1)  // One second has passed
    {
        timer2_overflow_count = 0;
        s++;
        if(s >= 60)
        {
            s = 0;
            m++;
            if(m >= 60)
            {
                m = 0;
                h++;
                if(h >= 24)
                {
                    h = 0;
                }
            }
        }
    }
}

void main(void)
{
    // Port A initialization: All pins as input
    DDRA = 0x00;
    PORTA = 0x00;

    // Port B initialization: PB4 as output, others as input
    DDRB = (1 << DDB4);
    PORTB = 0x00;

    // Port C initialization: All pins as input
    DDRC = 0x00;
    PORTC = 0x00;

    // Port D initialization: All pins as input
    DDRD = 0x00;
    PORTD = 0x00;

    // Timer/Counter 2 initialization for RTC
    // Crystal frequency = 32.768 kHz
    // Timer clock = Crystal frequency / 128 = 256 Hz
    ASSR = (1<<AS2);    // Enable asynchronous operation
    TCNT2 = 0x00;       // Start counting from 0
    OCR2 = 0x00;
    TCCR2 = (1<<CS22) | (0<<CS21) | (1<<CS20);  // Clock source: TOSC1 pin, prescaler 128

    // Wait for ASSR bits to be cleared
    while(ASSR & ((1<<TCN2UB)|(1<<OCR2UB)|(1<<TCR2UB)));
    
    // Enable Timer2 overflow interrupt
    TIMSK = (1<<TOIE2);

    // Reset initial time
    h = 0;
    m = 0;
    s = 0;

    // Enable global interrupts
    #asm("sei")

    // Alphanumeric LCD initialization
    lcd_init(16);

    // Display name and student ID
    display_info();

    while (1)
    {
        // Get and display the time
        get_time();
        delay_ms(100);  // Reduced delay for more responsive display

        // Check if any button is pressed to set the time
        if (PINB & (1 << PINB0) || PINB & (1 << PINB1) || PINB & (1 << PINB2) || PINB & (1 << PINB3))
        {
            set_time();
        }

        // Check if the alarm set button is pressed
        if (PINB & (1 << PINB5))
        {
            set_alarm();
        }

        // Check the alarm
        check_alarm();
    }
}

// Function to display name and student ID
void display_info(void)
{
    lcd_gotoxy(0, 0);
    
    lcd_gotoxy(0, 1);
    
    delay_ms(3000);
    lcd_clear();
}

// Function to get and display the time
void get_time(void)
{
    char period[3];
    unsigned char display_h = h;

    // Determine AM/PM period
    if (h >= 12)
    {
        strcpy(period, "PM");
        if (h > 12)
        {
            display_h = h - 12;  // Convert from 24h to 12h format
        }
        else
        {
            display_h = 12;      // 12 PM case
        }
    }
    else
    {
        strcpy(period, "AM");
        if (h == 0)
        {
            display_h = 12;      // 12 AM case
        }
        else
        {
            display_h = h;       // Regular AM hours
        }
    }

    lcd_clear();
    lcd_gotoxy(0, 0);
    sprintf(clock_buffer, "Time: %02d:%02d:%02d%s", display_h, m, s, period);
    lcd_puts(clock_buffer);

    // Display the alarm time
    lcd_gotoxy(0, 1);
    if (alarm_set)
    {
        sprintf(alarm_buffer, "Alarm: %02d:%02d:%02d", alarm_h, alarm_m, alarm_s);
    }
    else
    {
        strcpy(alarm_buffer, "Alarm: Not set");
    }
    lcd_puts(alarm_buffer);
}

// Function to set the time using buttons
void set_time(void)
{
    // Disable Timer2 interrupt while setting time
    TIMSK &= ~(1<<TOIE2);
    
    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_puts("Set Time:");

    while (1)
    {
        // Check if button on PB0 is pressed to increment hour
        if (PINB & (1 << PINB0))
        {
            h = (h + 1) % 24;
            lcd_gotoxy(0, 1);
            sprintf(clock_buffer, "Hour: %02d", h);
            lcd_puts(clock_buffer);
            delay_ms(50);
            while (PINB & (1 << PINB0));
            delay_ms(50);
        }

        // Check if button on PB1 is pressed to increment minute
        if (PINB & (1 << PINB1))
        {
            m = (m + 1) % 60;
            lcd_gotoxy(0, 1);
            sprintf(clock_buffer, "Minute: %02d", m);
            lcd_puts(clock_buffer);
            delay_ms(50);
            while (PINB & (1 << PINB1));
            delay_ms(50);
        }

        // Check if button on PB2 is pressed to increment second
        if (PINB & (1 << PINB2))
        {
            s = (s + 1) % 60;
            lcd_gotoxy(0, 1);
            sprintf(clock_buffer, "Second: %02d", s);
            lcd_puts(clock_buffer);
            delay_ms(50);
            while (PINB & (1 << PINB2));
            delay_ms(50);
        }

        // Check if the save button on PB3 is pressed to save the time
        if (PINB & (1 << PINB3))
        {
            lcd_clear();
            lcd_gotoxy(0, 0);
            lcd_puts("Time Saved");
            delay_ms(1000);
            while (PINB & (1 << PINB3));
            delay_ms(50);
            break;
        }
    }

    // Reset Timer2 counter after setting time
    TCNT2 = 0x00;
    // Re-enable Timer2 interrupt
    TIMSK |= (1<<TOIE2);
    
    // Clear the LCD after setting the time
    lcd_clear();
}

// Function to set the alarm time using buttons
void set_alarm(void)
{
    unsigned char temp_m = 0, temp_s = 0;

    lcd_clear();
    lcd_gotoxy(0, 0);
    lcd_puts("Set Alarm:");

    while (1)
    {
        // Check if button on PB1 is pressed to increment minute
        if (PINB & (1 << PINB1))
        {
            temp_m = (temp_m + 1) % 60;
            lcd_gotoxy(0, 1);
            sprintf(alarm_buffer, "Minute: %02d", temp_m);
            lcd_puts(alarm_buffer);
            delay_ms(50);
            while (PINB & (1 << PINB1));
        }

        // Check if button on PB2 is pressed to increment second
        if (PINB & (1 << PINB2))
        {
            temp_s = (temp_s + 1) % 60;
            lcd_gotoxy(0, 1);
            sprintf(alarm_buffer, "Second: %02d", temp_s);
            lcd_puts(alarm_buffer);
            delay_ms(50);
            while (PINB & (1 << PINB2));
        }

        // Check if the save button on PB3 is pressed to save the alarm time
        if (PINB & (1 << PINB3))
        {
            // Calculate the alarm time based on the current time plus the user-defined duration
            alarm_s = s + temp_s;
            alarm_m = m + temp_m + (alarm_s / 60);
            alarm_h = h + (alarm_m / 60);
            alarm_s %= 60;
            alarm_m %= 60;
            alarm_h %= 24;
            alarm_set = 1;
            
            lcd_clear();
            lcd_gotoxy(0, 0);
            lcd_puts("Alarm Set");
            delay_ms(1000);
            get_time();
            break;
        }
    }
}

// Function to check if the alarm time matches the current time
void check_alarm(void)
{
    if (alarm_set && h == alarm_h && m == alarm_m && s == alarm_s)
    {
        PORTB |= (1 << PORTB4); // Turn on the buzzer
        delay_ms(1000); // Sound the buzzer for 1 second
        PORTB &= ~(1 << PORTB4); // Turn off the buzzer
        alarm_set = 0; // Reset the alarm
    }
}

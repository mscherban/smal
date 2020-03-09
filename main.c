//***************************************************************************************

//***************************************************************************************
#include <smal.h>

void led_on() {
    volatile unsigned int i;

    while (1) {
        P1OUT |= 0x01;                      // Toggle P1.0 using exclusive-OR

        i = 10000;                          // SW Delay
        do i--;
        while(i != 0);
    }
}

void led_off() {
    volatile unsigned int i;

    while (1) {
        P1OUT &= ~0x01;                      // Toggle P1.0 using exclusive-OR

        i = 50;                          // SW Delay
        do i--;
        while(i != 0);
    }
}

void main(void) {
    //uint8_t pid;
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    P1DIR |= 0x01;                          // Set P1.0 to output direction

    initialize();

    ready(create(led_on, 10));
    ready(create(led_off, 10));

    start_small();

    /* will never reach here */
    for(;;) {

    }
}



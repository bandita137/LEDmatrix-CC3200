/*
 * BTN_handler.c
 *
 *  Created on: Jan 13, 2018
 *      Author: chajdu
 */


#include <ti/sysbios/knl/Clock.h>

#include <osi.h>

#include <hw_types.h>
#include <hw_memmap.h>
#include <rom_map.h>
#include <gpio.h>
#include <pin.h>

#include "pingpong_RTOS_common.h"


// Global flags
extern int BTN_ScoreReset, BTN1_ScoreDec, BTN1_ScoreInc, BTN2_ScoreDec, BTN2_ScoreInc;


static void BTN_Init() {
    // Activate pullups for the corresponding GPIO pins
    MAP_PinConfigSet(PIN_06, PIN_STRENGTH_2MA, PIN_TYPE_STD_PU);
    MAP_PinConfigSet(PIN_07, PIN_STRENGTH_2MA, PIN_TYPE_STD_PU);
}


void BTN_Handler() {
    // Previous button values
    static int BTN1_Prev, BTN2_Prev;

    // Button press start times
    static int BTN1_PressStartTime, BTN2_PressStartTime;

    // Flags to avoid setting score decrement flags multiple times for the same event
    static int BTN1_ScoreDecAlreadySet, BTN2_ScoreDecAlreadySet;


    // Initialize buttons
    BTN_Init();

    while (1) {
        // Read button values and translate them
        // - 0: depressed
        // - 1: pressed
        int BTN1_Val = MAP_GPIOPinRead(GPIOA1_BASE, GPIO_PIN_7) ? 0 : 1;
        int BTN2_Val = MAP_GPIOPinRead(GPIOA2_BASE, GPIO_PIN_0) ? 0 : 1;


        // BTN1 currently pressed
        if (BTN1_Val) {
            // Save press start time at rising edge
            if (!BTN1_Prev) BTN1_PressStartTime = Clock_getTicks();

            // Button has been pressed longer than the reset timeout
            else if (Clock_getTicks() - BTN1_PressStartTime > RESET_TIMEOUT_MS)
                BTN_ScoreReset = 1;

            // Button has been pressed longer than the decrement timeout but not long enough for a reset
            else if (Clock_getTicks() - BTN1_PressStartTime > SCORE_DEC_TIMEOUT_MS) {
                // Only set global flag once
                if (!BTN1_ScoreDecAlreadySet)
                    BTN1_ScoreDec = 1;

                // Set flag to avoid raising global flag again
                BTN1_ScoreDecAlreadySet = 1;
            }
        }
        // BTN1 not pressed
        else {
            // Clear flag: global ScoreDec flag may be raised again if the timeout is exceeded
            BTN1_ScoreDecAlreadySet = 0;

            // Falling edge on BTN1 (equivalent to !BTN1_Val && BTN1_Prev)
            if (BTN1_Prev)
                if ( (DEBOUNCE_TIMEOUT_MS < Clock_getTicks() - BTN1_PressStartTime) && (Clock_getTicks() - BTN1_PressStartTime < SCORE_DEC_TIMEOUT_MS) )
                    BTN1_ScoreInc = 1;
        }

        // BTN2 currently pressed
        if (BTN2_Val) {
            // Save press start time at rising edge
            if (!BTN2_Prev) BTN2_PressStartTime = Clock_getTicks();

            // Button has been pressed longer than the reset timeout
            else if (Clock_getTicks() - BTN2_PressStartTime > RESET_TIMEOUT_MS)
                BTN_ScoreReset = 1;

            // Button has been pressed longer than the decrement timeout but not long enough for a reset
            else if (Clock_getTicks() - BTN2_PressStartTime > SCORE_DEC_TIMEOUT_MS) {
                // Only set global flag once
                if (!BTN2_ScoreDecAlreadySet)
                    BTN2_ScoreDec = 1;

                // Set flag to avoid raising global flag again
                BTN2_ScoreDecAlreadySet = 1;
            }
        }
        // BTN2 not pressed
        else {
            // Clear flag: global ScoreDec flag may be raised again if the timeout is exceeded
            BTN2_ScoreDecAlreadySet = 0;

            // Falling edge on BTN2 (equivalent to !BTN2_Val && BTN2_Prev)
            if (BTN2_Prev)
                if ( (DEBOUNCE_TIMEOUT_MS < Clock_getTicks() - BTN2_PressStartTime) && (Clock_getTicks() - BTN2_PressStartTime < SCORE_DEC_TIMEOUT_MS) )
                    BTN2_ScoreInc = 1;
        }

        // Save previous button values
        BTN1_Prev = BTN1_Val;
        BTN2_Prev = BTN2_Val;

        // Sleep before polling buttons again
        osi_Sleep(BTNH_SLP_DURATION);
    }
}


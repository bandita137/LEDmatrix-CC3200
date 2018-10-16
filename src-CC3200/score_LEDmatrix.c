/*
 * score_LEDmatrix.c
 *
 *  Created on: Dec 14, 2017
 *      Author: chajdu
 */


#include <string.h>
#include <stdio.h>

#include <osi.h>

#include "score_LEDmatrix.h"
#include "dot_matrix_font_5x7.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "gpio.h"


// Button press flags
int BTN_ScoreReset, BTN1_ScoreDec, BTN1_ScoreInc, BTN2_ScoreDec, BTN2_ScoreInc;

char PP_player_names[2][64] = { "Andr\xc3\xa1s", "Gy\xc3\xb6rgy" };
char PP_player_names_t[2][9];
unsigned char PP_player_name_u = 1;     // Player name update flag

unsigned int PP_score[2] = {0, 0};      // If the score underflows, it will automatically reset everything
unsigned char PP_score_u = 1;           // Score update flag


char LED_R_map[32][64];
char LED_G_map[32][64];
char LED_B_map[32][64];


// Disable optimization for the LED functions. Otherwise, the compiler will ignore the volatile directives and start optimizing like crazy
#pragma FUNCTION_OPTIONS(LED_CLK, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_OEn, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_STB, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_R0, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_R1, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_G0, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_G1, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_B0, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_B1, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_A, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_B, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_C, "--opt_level=off")
#pragma FUNCTION_OPTIONS(LED_D, "--opt_level=off")

// Functions to set the LED-related signals
// LED_CLK -> GPIO_10, GPIOA1.2, pin 1
inline void LED_CLK(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_2 << 2))) = ucVal << 2;
}
// LED_OEn -> GPIO_11, GPIOA1.3, pin 2
inline void LED_OEn(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_3 << 2))) = ucVal << 3;
}
// LED_STB -> GPIO_00, GPIOA0.0, pin 50
inline void LED_STB(unsigned char ucVal) {
    HWREG(GPIOA0_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_0 << 2))) = ucVal << 0;
}
// LED_R0 -> GPIO_13, GPIOA1.5, pin 4
inline void LED_R0(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_5 << 2))) = ucVal << 5;
}
// LED_R1 -> GPIO_06, GPIOA0.6, pin 61
inline void LED_R1(unsigned char ucVal) {
    HWREG(GPIOA0_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_6 << 2))) = ucVal << 6;
}
// LED_G0 -> GPIO_05, GPIOA0.5, pin 60
inline void LED_G0(unsigned char ucVal) {
    HWREG(GPIOA0_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_5 << 2))) = ucVal << 5;
}
// LED_G1 -> GPIO_08, GPIOA1.0, pin 63
inline void LED_G1(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_0 << 2))) = ucVal << 0;
}
// LED_B0 -> GPIO_12, GPIOA1.4, pin 3
inline void LED_B0(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_4 << 2))) = ucVal << 4;
}
// LED_B1 -> GPIO_04, GPIOA0.4, pin 59
inline void LED_B1(unsigned char ucVal) {
    HWREG(GPIOA0_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_4 << 2))) = ucVal << 4;
}
// LED_A -> GPIO_14, GPIOA1.6, pin 5
inline void LED_A(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_6 << 2))) = ucVal << 6;
}
// LED_B -> GPIO_30, GPIOA3.6, pin 53
inline void LED_B(unsigned char ucVal) {
    HWREG(GPIOA3_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_6 << 2))) = ucVal << 6;
}
// LED_C -> GPIO_07, GPIOA0.7, pin 62
inline void LED_C(unsigned char ucVal) {
    HWREG(GPIOA0_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_7 << 2))) = ucVal << 7;
}
// LED_D -> GPIO_09, GPIOA1.1, pin 64
inline void LED_D(unsigned char ucVal) {
    HWREG(GPIOA1_BASE + (GPIO_O_GPIO_DATA + (GPIO_PIN_1 << 2))) = ucVal << 1;
}



// Transcode string: all Hungarian diacritic characters are transcoded from
//                   UTF-8 to their equivalents in dot_matrix_font_5x7.h
//                   (Character numbers 128-145)
void transcode_str(char * str_in, char * str_out) {
    unsigned char str_out_cnt = 0;
    char * str_out_curr = str_out;


    // Go through the input string character by character
    for (char * str_in_curr = str_in; *str_in_curr; str_in_curr++) {
        // If a 0 remains, it means the character in question hasn't been transcoded
        *str_out_curr = 0;

        // Transcode current character if it is a Hungarian diacritic character
        if ( *str_in_curr == '\xc3' )
            switch ( *(str_in_curr + 1) ) {
                case '\x81'     :   *str_out_curr = 128;    // Á
                                    break;
                case '\x89'     :   *str_out_curr = 129;    // É
                                    break;
                case '\x8d'     :   *str_out_curr = 130;    // Í
                                    break;
                case '\x93'     :   *str_out_curr = 131;    // Ó
                                    break;
                case '\x96'     :   *str_out_curr = 132;    // Ö
                                    break;
                case '\x9a'     :   *str_out_curr = 134;    // Ú
                                    break;
                case '\x9c'     :   *str_out_curr = 135;    // Ü
                                    break;
                case '\xa1'     :   *str_out_curr = 137;    // á
                                    break;
                case '\xa9'     :   *str_out_curr = 138;    // é
                                    break;
                case '\xad'     :   *str_out_curr = 139;    // í
                                    break;
                case '\xb3'     :   *str_out_curr = 140;    // ó
                                    break;
                case '\xb6'     :   *str_out_curr = 141;    // ö
                                    break;
                case '\xba'     :   *str_out_curr = 143;    // ú
                                    break;
                case '\xbc'     :   *str_out_curr = 144;    // ü
                                    break;
            }
        else if ( *str_in_curr == '\xc5' )
            switch ( *(str_in_curr + 1) ) {
                case '\x90'     :   *str_out_curr = 133;    // O"
                                    break;
                case '\xb0'     :   *str_out_curr = 136;    // o"
                                    break;
                case '\x91'     :   *str_out_curr = 142;    // U"
                                    break;
                case '\xb1'     :   *str_out_curr = 145;    // u"
                                    break;
            }

        // Take care of non-transcoded chars
        if ( *str_out_curr == 0 )
            // The characters that are not part of the font are converted to spaces
            if ( *str_in_curr < 32 || *str_in_curr > 128)
                *str_out_curr = 32;
            // Everything else is left as it was
            else
                *str_out_curr = *str_in_curr;

        // Account for 2-byte UTF-8 characters as required by incrementing the input string character pointer
        if ( *str_in_curr == '\xc3' || *str_in_curr == '\xc5' )
            str_in_curr++;

        // Advance output string character pointer
        str_out_curr++;

        // Break if there are 8 characters in the output string (there's only this much space on the display)
        if ( ++str_out_cnt == 8 )
            break;
    }

    // Terminate output string
    *str_out_curr = 0;
}


// Load string into LED display buffer
void LED_load_str(char * str_in, char map[32][64], char text_row) {
    // Start at column 2
    int map_Col = 2;

    // Load string
    for (int Char = 0; Char < strlen(str_in); Char++)
        for (int Col = 0; Col < 6; Col++) {
            for (int Row = 0; Row < 7; Row++)
                if (Col < 5)
                    map[Row + text_row * 8][map_Col] = (Font5x7[str_in[Char] - 32][Col] >> Row) & 0x01;
                else    // Insert space between letters
                    map[Row + text_row * 8][map_Col] = 0;

            // Clear LED in the 8th row
            map[text_row * 8 + 7][map_Col] = 0;

            map_Col++;
        }

    // Clear remaining dots
    for (int Char = strlen(str_in); Char < 10; Char++)
        for (int Col = 0; Col < 6; Col++) {
            for (int Row = 0; Row <= 7; Row++)
                map[Row + text_row * 8][map_Col] = 0;

            map_Col++;
        }
}


// Load string into LED display buffer
// Alternate version: the string is loaded at an arbitrary position specified
//                    as [row, col]
void LED_load_str_arb(char * str_in, char map[32][64], char LED_row, char LED_col) {
    // Load string
    for (int Char = 0; Char < strlen(str_in); Char++)
        for (int Col = 0; Col < 6; Col++) {
            for (int Row = 0; Row < 7; Row++)
                if (Col < 5)
                    map[Row + LED_row][LED_col] = (Font5x7[str_in[Char] - 32][Col] >> Row) & 0x01;
                else    // Insert space between letters
                    map[Row + LED_row][LED_col] = 0;

            // Clear LED in the 8th row
            map[LED_row + 7][LED_col] = 0;

            LED_col++;
        }
}


// Clear LED display buffer
// The limits for clearing are specified as parameters
void LED_clear_buf(char map[32][64], char row_min, char row_max, char col_min, char col_max) {
    // Clear dots
    for (int Row = row_min; Row <= row_max; Row++)
        for (int Col = col_min; Col <= col_max; Col++)
            map[Row][Col] = 0;
}


void score_LEDmatrix() {
    // Initialize all signals
    LED_CLK(0);
    LED_OEn(1);
    LED_STB(0);

    LED_R0(0);
    LED_R1(0);
    LED_G0(0);
    LED_G1(0);
    LED_B0(0);
    LED_B1(0);

    // Clear all LED buffers to be sure
    LED_clear_buf(LED_R_map, 0, 31, 0, 63);
    LED_clear_buf(LED_G_map, 0, 31, 0, 63);
    LED_clear_buf(LED_B_map, 0, 31, 0, 63);


    // Handle key presses, count score, update display buffers, send buffers to display
    while(1) {
// ------------------------------
//  HANDLE BUTTON PRESS FLAGS
// ------------------------------
        // Score reset
        if (BTN_ScoreReset) {
            // Clear flag
            BTN_ScoreReset = 0;

            // Reset scores
            PP_score[0] = 0;
            PP_score[1] = 0;

            // Set score update flag for display
            PP_score_u = 1;
        }
        else {
            // Player 1 score decrement
            if (BTN1_ScoreDec) {
                // Clear flag
                BTN1_ScoreDec = 0;

                // Decrement score
                PP_score[0]--;

                // Set score update flag for display
                PP_score_u = 1;
            }
            // Player 1 score increment
            else if (BTN1_ScoreInc) {
                // Clear flag
                BTN1_ScoreInc = 0;

                // Decrement score
                PP_score[0]++;

                // Set score update flag for display
                PP_score_u = 1;
            }

            // Player 2 score decrement
            if (BTN2_ScoreDec) {
                // Clear flag
                BTN2_ScoreDec = 0;

                // Decrement score
                PP_score[1]--;

                // Set score update flag for display
                PP_score_u = 1;
            }
            // Player 1 score increment
            else if (BTN2_ScoreInc) {
                // Clear flag
                BTN2_ScoreInc = 0;

                // Decrement score
                PP_score[1]++;

                // Set score update flag for display
                PP_score_u = 1;
            }
        }

        // If either score exceeds the max, reset everything (this also covers underflow since they are unsigned)
        if ( (PP_score[0] > SCORE_MAX) || (PP_score[1] > SCORE_MAX) ) {
            // Reset scores
            PP_score[0] = 0;
            PP_score[1] = 0;

            // Set score update flag for display
            PP_score_u = 1;
        }


        // Update player names in display buffers if required
        if ( PP_player_name_u ) {
            // Transcode strings
            for (int i = 0; i < 2; i++)
                transcode_str(PP_player_names[i], PP_player_names_t[i]);

            // Clear LED display area corresponding to player names
            LED_clear_buf(LED_R_map, 6, 25, 2, 50);
            LED_clear_buf(LED_G_map, 6, 25, 2, 50);
            LED_clear_buf(LED_B_map, 6, 25, 2, 50);

            // Load LED display buffers
            LED_load_str_arb(PP_player_names_t[0], LED_R_map, 6, 2);
            LED_load_str_arb(PP_player_names_t[1], LED_G_map, 19, 2);

            // Clear flag
            PP_player_name_u = 0;
        }

        // Update scores in display buffers if required
        if ( PP_score_u ) {
            // Load scores into display buffer
            char tmp_buf[3];

            sprintf(tmp_buf, "%2d", PP_score[0]);
            LED_load_str_arb(tmp_buf, LED_R_map, 6, 51);
            LED_load_str_arb(tmp_buf, LED_G_map, 6, 51);
            LED_load_str_arb(tmp_buf, LED_B_map, 6, 51);
            sprintf(tmp_buf, "%2d", PP_score[1]);
            LED_load_str_arb(tmp_buf, LED_R_map, 19, 51);
            LED_load_str_arb(tmp_buf, LED_G_map, 19, 51);
            LED_load_str_arb(tmp_buf, LED_B_map, 19, 51);

            // Clear flag
            PP_score_u = 0;
        }


// ------------------------------
//  UPDATE DISPLAY
// ------------------------------
        for (int Row = 0; Row < 16; Row++) {
            // -----------------
            //  RED PIXELS
            // -----------------
            // Send out 64 bits
            for (int Col = 0; Col < 64; Col++) {
                LED_CLK(0);

                LED_R0(LED_R_map[Row][Col]);
                LED_R1(LED_R_map[Row + 16][Col]);
                LED_G0(0);
                LED_G1(0);
                LED_B0(0);
                LED_B1(0);

                LED_CLK(1);
            }

            // Disable LED driver output
            LED_OEn(1);

            // Strobe transmission to LED driver output
            LED_STB(1);
            LED_STB(0);

            // Select channel to operate on
            LED_A((Row & 0x01));
            LED_B((Row & 0x02) >> 1);
            LED_C((Row & 0x04) >> 2);
            LED_D((Row & 0x08) >> 3);

            // Enable LED driver output
            LED_OEn(0);

            // -----------------
            //  GREEN PIXELS
            // -----------------
            // Send out 64 bits
            for (int Col = 0; Col < 64; Col++) {
                LED_CLK(0);

                LED_R0(0);
                LED_R1(0);
                LED_G0(LED_G_map[Row][Col]);
                LED_G1(LED_G_map[Row + 16][Col]);
                LED_B0(0);
                LED_B1(0);

                LED_CLK(1);
            }

            // Disable LED driver output
            LED_OEn(1);

            // Strobe transmission to LED driver output
            LED_STB(1);
            LED_STB(0);

            // Select channel to operate on
            LED_A((Row & 0x01));
            LED_B((Row & 0x02) >> 1);
            LED_C((Row & 0x04) >> 2);
            LED_D((Row & 0x08) >> 3);

            // Enable LED driver output
            LED_OEn(0);

            // -----------------
            //  BLUE PIXELS
            // -----------------
            // Send out 64 bits
            for (int Col = 0; Col < 64; Col++) {
                LED_CLK(0);

                LED_R0(0);
                LED_R1(0);
                LED_G0(0);
                LED_G1(0);
                LED_B0(LED_B_map[Row][Col]);
                LED_B1(LED_B_map[Row + 16][Col]);

                LED_CLK(1);
            }

            // Disable LED driver output
            LED_OEn(1);

            // Strobe transmission to LED driver output
            LED_STB(1);
            LED_STB(0);

            // Select channel to operate on
            LED_A((Row & 0x01));
            LED_B((Row & 0x02) >> 1);
            LED_C((Row & 0x04) >> 2);
            LED_D((Row & 0x08) >> 3);

            // Enable LED driver output
            LED_OEn(0);
        }

        // -----------------------
        //  BRIGHTNESS CORRECTION
        // -----------------------
        // Drive the blue pixels of the last row once more. These won't actually
        // get transferred to the output, but by disabling the LED drivers once
        // this is done, we can ensure that the 16th and 32nd blue rows get
        // driven for roughly the same amount of time as every other row, thus
        // these rows won't be a little bit brighter than the others
        for (int Col = 0; Col < 64; Col++) {
            LED_CLK(0);

            LED_R0(0);
            LED_R1(0);
            LED_G0(0);
            LED_G1(0);
            LED_B0(LED_B_map[15][Col]);
            LED_B1(LED_B_map[31][Col]);

            LED_CLK(1);
        }

        // Disable LED driver output
        LED_OEn(1);


        // Sleep to allow the CPU to take care of communication
        osi_Sleep(LDMTX_SLP_DURATION);
    }
}


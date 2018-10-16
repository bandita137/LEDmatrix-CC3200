/*
 * pingpong_RTOS_common.h
 *
 *  Created on: Jan 3, 2018
 *      Author: chajdu
 */

#ifndef PINGPONG_RTOS_COMMON_H_
#define PINGPONG_RTOS_COMMON_H_



// WiFi and server settings
#define WLAN_SSID           "BME"
#define WLAN_SECURITY_TYPE  SL_SEC_TYPE_OPEN
#define WLAN_SECURITY_KEY   ""
//#define WLAN_SSID           "fpgalab"
//#define WLAN_SECURITY_TYPE  SL_SEC_TYPE_WPA_WPA2
//#define WLAN_SECURITY_KEY   "lehetetlenek"

#define SERVER_IP_ADDR      0x9842fc89  // 152.66.252.137, mitpc57.mit.bme.hu
#define SERVER_PORT_NUM     7891
#define BUF_SIZE            1024


// Button durations
#define DEBOUNCE_TIMEOUT_MS 75
#define SCORE_DEC_TIMEOUT_MS 1500
#define RESET_TIMEOUT_MS 3000

// Maximum score
#define SCORE_MAX 99

// OS task priorities
#define WLAN_TASK_PRIORITY  1
#define LDMTX_TASK_PRIORITY 2
#define BTN_TASK_PRIORITY   3       // Highest priority so the execution period is more or less stable
// OSI stack sizes per task
#define WLAN_T_STACK_SIZE   8192
#define LDMTX_T_STACK_SIZE  8192
#define BTN_T_STACK_SIZE    512
// Sleep duration after one iteration of the LED matrix task has run (milliseconds)
#define LDMTX_SLP_DURATION  5
// Button handler task sleep duration
#define BTNH_SLP_DURATION   10




#endif /* PINGPONG_RTOS_COMMON_H_ */

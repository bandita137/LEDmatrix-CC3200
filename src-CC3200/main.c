//*****************************************************************************
//
// Application Name     - Ping pong score counter
// Application Overview - This application counts the score, displays it
//                        on the LED matrix display and provides a simple
//                        TCP interface over WLAN
// Application Details  -
// Based on the TCP Socket example application, see
// http://processors.wiki.ti.com/index.php/CC32xx_TCP_Socket_Application
// or
// docs\examples\CC32xx_TCP_Socket_Application.pdf
//
//*****************************************************************************


//****************************************************************************
//
//! \addtogroup pingpong_TCP
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>

// RTOS includes
#include <osi.h>

// driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "utils.h"

// common interface includes
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif

#include "pin_mux_config.h"

//#include "score_LEDmatrix.h"

#include "WLAN_TCP.h"


// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SOCKET_CREATE_ERROR = -0x7D0,
    BIND_ERROR = SOCKET_CREATE_ERROR - 1,
    LISTEN_ERROR = BIND_ERROR -1,
    SOCKET_OPT_ERROR = LISTEN_ERROR -1,
    CONNECT_ERROR = SOCKET_OPT_ERROR -1,
    ACCEPT_ERROR = CONNECT_ERROR - 1,
    SEND_ERROR = ACCEPT_ERROR -1,
    RECV_ERROR = SEND_ERROR -1,
    SOCKET_CLOSE_ERROR = RECV_ERROR -1,
    DEVICE_NOT_IN_STATION_MODE = SOCKET_CLOSE_ERROR - 1,
    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static void BoardInit();


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
int main()
{
    long lRetVal = -1;


    // Initialize board
    BoardInit();

    // Configure pinmux settings
    PinMuxConfig();

    // Configure UART
    InitTerm();

    UART_PRINT("-------------------------\r\n Ping pong score counter\r\n-------------------------\r\n");

    // Start the SimpleLink Host
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    // Create WLAN handler task
    // Once the WLAN has been initialized, this task will create the LED matrix driver task
    lRetVal = osi_TaskCreate( WLANTask, \
                              (const signed char*) "WLAN handler task", \
                              WLAN_T_STACK_SIZE, NULL, WLAN_TASK_PRIORITY, NULL );
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    // Start the task scheduler
    osi_start();

    // This should never happen...
    VDeleteSimpleLinkSpawnTask();
}


//*****************************************************************************
//
//! \brief  Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
// In case of TI-RTOS vector table is initialize by OS itself
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif //USE_TIRTOS

    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

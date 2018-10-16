/*
 * WLAN_TCP.c
 *
 *  Created on: Dec 22, 2017
 *      Author: chajdu
 */


#include <string.h>
#include <stdio.h>

#include <osi.h>

#include "WLAN_TCP.h"
#include "score_LEDmatrix.h"
#include "BTN_handler.h"


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
static int WLANInit();
static int OpenTCPClientSocket();
static int OpenTCPServerSocket();
static long ConfigureSimpleLinkToDefaultState();
static long WlanConnect();
static long NoIPUpdate();


//*****************************************************************************
//                 EXTERNAL VARIABLES
//*****************************************************************************
// These variables are used for communicating with the LED matrix driver
extern char PP_player_names[2][64];
extern unsigned char PP_player_name_u;

extern unsigned int PP_score[2];
extern unsigned char PP_score_u;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned char  g_ucConnectionStatus = 0;
unsigned char  g_ucSimplelinkstarted = 0;
unsigned long  g_ulIpAddr = 0;

int g_ServerSocket, g_CommSocket;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


void WLANTask() {
    long lRetVal = -1;

    SlSockAddrIn_t CommSocketAddr;
    int CommSocketAddrSize = sizeof(SlSockAddrIn_t);

    char buf_in[BUF_SIZE], buf_out[BUF_SIZE], tmp_buf[BUF_SIZE];
    int tmp;
    int status;


    // Initialize WLAN
    status = WLANInit();

    // Once the initialization is complete, create the LED matrix driver task
    UART_PRINT("Starting LED matrix driver ...");

    lRetVal = osi_TaskCreate( score_LEDmatrix, \
                              (const signed char*) "LED matrix driver", \
                              LDMTX_T_STACK_SIZE, NULL, LDMTX_TASK_PRIORITY, NULL );
    if ( lRetVal < 0 ) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    UART_PRINT(" done.\r\n");


    // Also create the button handler task
    UART_PRINT("Starting button handler ...");

    lRetVal = osi_TaskCreate( BTN_Handler, \
                              (const signed char*) "Button handler", \
                              BTN_T_STACK_SIZE, NULL, BTN_TASK_PRIORITY, NULL );
    if ( lRetVal < 0 ) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    UART_PRINT(" done.\r\n");


    // If the server socket was fired up successfully, fire up a communication socket. Repeat when the current one dies
    if (status)
        while (1) {
            UART_PRINT("Waiting for incoming connection ... ");

            g_CommSocket = SL_EAGAIN;

            // Wait for an incoming TCP connection
            while ( g_CommSocket < 0 ) {
                // Accept connection from TCP client, return SL_EAGAIN if none available
                // otherwise returns SL_EAGAIN
                g_CommSocket = sl_Accept(g_ServerSocket, (struct SlSockAddr_t *) &CommSocketAddr, (SlSocklen_t*) &CommSocketAddrSize);

                // Detect errors
                if ( (g_CommSocket < 0) && (g_CommSocket != SL_EAGAIN) ) {
                    // Close socket
                    sl_Close(g_CommSocket);

                    UART_PRINT("ERROR accepting incoming connection\r\n");
                }
            }

            UART_PRINT("client connected\r\n");

            while (1) {
                status = sl_Recv(g_CommSocket, buf_in, BUF_SIZE, 0);

                // Client disconnected
                if (status == 0) {
                    UART_PRINT("Client disconnected\r\n");

                    // Close socket
                    sl_Close(g_CommSocket);

                    break;
                }
                // Error
                else if (status < 0) {
                    sl_Close(g_CommSocket);
                    ERR_PRINT(RECV_ERROR);
                }

                UART_PRINT("%s\r\n", buf_in);

                // Process command
                if ( sscanf(buf_in, "P1N %[^\n]", tmp_buf) == 1 ) {
                    // Ensure the string fits into the destination buffer
                    tmp_buf[32] = 0;

                    strcpy(PP_player_names[0], tmp_buf);

                    PP_player_name_u = 1;
                }
                else if ( sscanf(buf_in, "P2N %[^\n]", tmp_buf) == 1 ) {
                    // Ensure the string fits into the destination buffer
                    tmp_buf[32] = 0;

                    strcpy(PP_player_names[1], tmp_buf);

                    PP_player_name_u = 1;
                }
                else if ( sscanf(buf_in, "P1S %u", &tmp) == 1 ) {
                    // Saturate score
                    if (tmp < 0)
                        tmp = 0;
                    else if (tmp > 99)
                        tmp = 99;

                    PP_score[0] = tmp;

                    PP_score_u = 1;
                }
                else if ( sscanf(buf_in, "P2S %u", &tmp) == 1 ) {
                    // Saturate score
                    if (tmp < 0)
                        tmp = 0;
                    else if (tmp > 99)
                        tmp = 99;

                    PP_score[1] = tmp;

                    PP_score_u = 1;
                }
                else if (!strcmp(buf_in, "POLL")) {
                    sprintf(buf_out, "Player 1 (%s): %u\r\nPlayer 2 (%s): %u\r\n\0", PP_player_names[0], PP_score[0], PP_player_names[1], PP_score[1]);

                    UART_PRINT(buf_out);
                    sl_Send(g_CommSocket, buf_out, BUF_SIZE, 0);
                }
            }
        }

    // Close socket if open
    if ( g_CommSocket >= 0 ) {
        status = sl_Close(g_CommSocket);
        if (status < 0)
            ERR_PRINT(status);
    }

    UART_PRINT("Shutting down WLAN connection ...");

    // Power off SimpleLink
    sl_Stop(SL_STOP_TIMEOUT);

    UART_PRINT(" done.\r\n");
}


//****************************************************************************
//
//! \brief      Initialize WLAN connection
//!
//! Fire up the WLAN connection and connect to the AP specified in the
//! corresponding macros. Also try connecting to the server specified
//! in a macro via TCP.
//!
//! \param      None
//!
//! \return     0 - TCP connection unsuccessful
//!             1 - TCP connection successful
//
//****************************************************************************
static int WLANInit() {
    long lRetVal = -1;


    //
    // The following function configures the device to a default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at the start of the application
    //
    // Note that all profiles and persistent settings that were made on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();

    if(lRetVal < 0)
    {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
         UART_PRINT("Failed to reset device to defaults\r\n");

      LOOP_FOREVER();
    }

    // Start SimpleLink
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0)
    {
        UART_PRINT("Failed to start the device\r\n");
        LOOP_FOREVER();
    }

    // Connect to WLAN
    UART_PRINT("Connecting to WLAN network: %s ...\r\n", WLAN_SSID);

    lRetVal = WlanConnect();
    if(lRetVal < 0)
    {
        UART_PRINT("Connection to AP failed, running in standalone mode\r\n");

        return 0;
    }

    UART_PRINT("... connected.\r\n");

    UART_PRINT("Device IP: %d.%d.%d.%d\r\n",
                      SL_IPV4_BYTE(g_ulIpAddr,3),
                      SL_IPV4_BYTE(g_ulIpAddr,2),
                      SL_IPV4_BYTE(g_ulIpAddr,1),
                      SL_IPV4_BYTE(g_ulIpAddr,0));

    // Update dynamic DNS address
    UART_PRINT("Starting dynamic DNS update ... ");
    NoIPUpdate();

/*
    // Fire up a TCP client socket
    UART_PRINT("Opening TCP client socket ...\r\n");
    g_CommSocket = OpenTCPClientSocket();

    if (g_CommSocket < 0) {
        UART_PRINT("Unable to connect to server, running in standalone mode\r\n");

        return 0;
    }
*/

    // Fire up a TCP server socket
    UART_PRINT("Opening TCP server socket ...\r\n");
    g_ServerSocket = OpenTCPServerSocket();

    if (g_ServerSocket < 0) {
        UART_PRINT("Unable to open server socket, running in standalone mode\r\n");

        return 0;
    }

    return 1;
}


//****************************************************************************
//
//! \brief      Update NoIP dynamic DNS host
//!
//! Update NoIP dynamic DNS host to current IP so the CC3200 can easily
//! be connected to as a server
//! Code based on the NoIP API:
//! - https://www.noip.com/integrate/request
//! - https://www.noip.com/integrate/response
//! and https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
//!
//! \param      None
//!
//! \return     0 - DNS update unsuccessful
//!             1 - DNS update successful
//
//****************************************************************************
static long NoIPUpdate() {
    const int PortNo = 80;
    char ServerHostName[] = "dynupdate.no-ip.com";
    const char * MessageFormat = "GET /nic/update?hostname=%s&myip=%d.%d.%d.%d HTTP/1.0\r\nHost: dynupdate.no-ip.com\r\nAuthorization: Basic %s\r\nUser-Agent:CC3200-noip-update-client TI-RTOS/0.01a EMAIL-ADDRESS-MASKED\r\n\r\n";
    const char * MyHostName = "HOSTNAME-MASKED";
    const char * AuthBase64 = "AUTHBASE-MASKED";

    long lRetVal;

    unsigned long ServerIPAddr;
    SlSockAddrIn_t ServerAddress;
    int SocketID, SocketStatus;

    int bytes, sent, received, total;
    char Message[1024], Response[1024];


    // Assemble message
    sprintf(Message, MessageFormat, MyHostName, SL_IPV4_BYTE(g_ulIpAddr,3),
                                                SL_IPV4_BYTE(g_ulIpAddr,2),
                                                SL_IPV4_BYTE(g_ulIpAddr,1),
                                                SL_IPV4_BYTE(g_ulIpAddr,0),
            AuthBase64);

    // Lookup IP address
    lRetVal = sl_NetAppDnsGetHostByName((signed char *) ServerHostName, strlen(ServerHostName), &ServerIPAddr, SL_AF_INET);
    if (lRetVal < 0) {
        UART_PRINT("ERROR looking up host\r\n");

        return 0;
    }

    // Set up server address
    ServerAddress.sin_family = SL_AF_INET;
    ServerAddress.sin_port = sl_Htons(PortNo);
    ServerAddress.sin_addr.s_addr = sl_Htonl(ServerIPAddr);

    // Create client socket
    SocketID = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if( SocketID < 0 ) {
        UART_PRINT("ERROR opening socket\r\n");

        return 0;
    }

    // Connect to server
    SocketStatus = sl_Connect(SocketID, (SlSockAddr_t *) &ServerAddress, sizeof(SlSockAddrIn_t));
    if( SocketStatus < 0 ) {
        sl_Close(SocketID);

        UART_PRINT("ERROR connecting socket\r\n");

        return 0;
    }

    // Send message
    total = strlen(Message);
    sent = 0;
    do {
        bytes = sl_Send(SocketID, Message + sent, total - sent, 0);
        if (bytes < 0) {
            UART_PRINT("ERROR writing message to socket\r\n");

            return 0;
        }
        if (bytes == 0)
            break;
        sent += bytes;
    } while (sent < total);

    // Receive response
    memset(Response, 0, sizeof(Response));
    total = sizeof(Response) - 1;
    received = 0;
    do {
        bytes = sl_Recv(SocketID, Response + received, total - received, 0);
        if (bytes < 0) {
            UART_PRINT("ERROR reading response from socket\r\n");

            return 0;
        }
        if (bytes == 0)
            break;
        received += bytes;
    } while (received < total);

    if (received == total)
        UART_PRINT("ERROR storing complete response from socket\r\n");

    // Close socket
    sl_Close(SocketID);

    // Parse response: a double newline indicates the relevant part
    char * ResponsePtr = Response;
    int NewLine = 0;

    while (ResponsePtr) {
        // Newline?
        if ( (*ResponsePtr == '\r' && *(ResponsePtr + 1) == '\n') || (*ResponsePtr == '\n' && *(ResponsePtr + 1) == '\r') ) {
            // Move to the next character - the pointer gets incremented another time in the loop so the entire newline is skipped
            ResponsePtr++;

            // Is this the second newline?
            if (NewLine) {
                // Increment pointer another time to skip the whole newline (the other increment in the loop won't be executed again)
                ResponsePtr++;

                // Exit while loop
                break;
            }
            // If not (second newline), just set the flag
            else
                NewLine = 1;
        }
        // If not (newline), clear the flag
        else
            NewLine = 0;

        // Advance to the next character
        ResponsePtr++;
    }

    UART_PRINT("response: %s", ResponsePtr);

    return 0;
}


//****************************************************************************
//
//! \brief      Open a TCP client side socket
//!
//! This function opens a TCP socket and tries to connect to a server on the
//! port and IP address specified in global defines.
//!
//! \param      None
//!
//! \return     The socket ID.
//
//****************************************************************************
static int OpenTCPClientSocket() {
    SlSockAddrIn_t socketAddress;
    int socketID;
    int socketStatus;


    // Set up socket address
    socketAddress.sin_family = SL_AF_INET;
    socketAddress.sin_port = sl_Htons(SERVER_PORT_NUM);
    socketAddress.sin_addr.s_addr = sl_Htonl(SERVER_IP_ADDR);

    // Create TCP socket
    socketID = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if( socketID < 0 ) {
        ASSERT_ON_ERROR(SOCKET_CREATE_ERROR);
    }

    // Connect to TCP server
    socketStatus = sl_Connect(socketID, ( SlSockAddr_t *) &socketAddress, sizeof(SlSockAddrIn_t));
    if( socketStatus < 0 ) {
        sl_Close(socketID);
        ASSERT_ON_ERROR(CONNECT_ERROR);
    }

    UART_PRINT("Successfully connected to %d.%d.%d.%d:%d\r\n",
              SL_IPV4_BYTE(SERVER_IP_ADDR,3),
              SL_IPV4_BYTE(SERVER_IP_ADDR,2),
              SL_IPV4_BYTE(SERVER_IP_ADDR,1),
              SL_IPV4_BYTE(SERVER_IP_ADDR,0),
              SERVER_PORT_NUM);

    return socketID;
}


//****************************************************************************
//
//! \brief      Open a TCP server side socket
//!
//! This function opens a TCP socket and listens for incoming connections on the
//! port specified in global defines.
//!
//! \param      None
//!
//! \return     The socket ID.
//
//****************************************************************************
static int OpenTCPServerSocket() {
    long            lRetVal;

    SlSockAddrIn_t  ServerSocketAddr;
    int             ServerSocket;
    long            NonBlocking = 1;


    // Fill TCP server socket address
    ServerSocketAddr.sin_family = SL_AF_INET;
    ServerSocketAddr.sin_port = sl_Htons(SERVER_PORT_NUM);
    ServerSocketAddr.sin_addr.s_addr = 0;

    // Create TCP socket
    ServerSocket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if( ServerSocket < 0 )
        ASSERT_ON_ERROR(SOCKET_CREATE_ERROR);

    // Bind TCP socket to TCP server address
    lRetVal = sl_Bind(ServerSocket, (SlSockAddr_t *) &ServerSocketAddr, sizeof(SlSockAddrIn_t));
    if( lRetVal < 0 ) {
        // Close socket
        sl_Close(ServerSocket);

        ASSERT_ON_ERROR(BIND_ERROR);
    }

    // Set socket to listen for incoming TCP connections
    lRetVal = sl_Listen(ServerSocket, 0);
    if( lRetVal < 0 ) {
        // Close socket
        sl_Close(ServerSocket);

        ASSERT_ON_ERROR(LISTEN_ERROR);
    }

    // Make socket non-blocking
//    lRetVal = sl_SetSockOpt(ServerSocket, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &NonBlocking, sizeof(NonBlocking));
//    if( lRetVal < 0 ) {
//        sl_Close(ServerSocket);
//        ASSERT_ON_ERROR(SOCKET_OPT_ERROR);
//    }

    return ServerSocket;
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }

        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }

    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);

/*
    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);
*/

    // Set connection policy to Auto + SmartConfig
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);



    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    // Initialize variables
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));

    return lRetVal; // Success
}


//****************************************************************************
//
//!  \brief Connecting to a WLAN Accesspoint
//!
//!   This function connects to the required AP (SSID_NAME) with Security
//!   parameters specified in te form of macros at the top of this file
//!
//!   \param[in]              None
//!
//!   \return     Status value
//!
//!   \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect()
{
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = (signed char*) WLAN_SECURITY_KEY;
    secParams.KeyLen = strlen(WLAN_SECURITY_KEY);
    secParams.Type = WLAN_SECURITY_TYPE;

    lRetVal = sl_WlanConnect((signed char*) WLAN_SSID, strlen(WLAN_SSID), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    /* Wait */
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        // Wait for WLAN Event
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#endif
    }

    return SUCCESS;

}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(!pWlanEvent)
    {
        return;
    }

    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] Connected to AP: %s,"
                        " BSSID: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT] Device disconnected from AP: %s,"
                "BSSID: %02x:%02x:%02x:%02x:%02x:%02x on application request\r\n",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR] Device disconnected from AP: %s,"
                            "BSSID: %02x:%02x:%02x:%02x:%02x:%02x on an ERROR!\r\n",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\r\n",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(!pNetAppEvent)
    {
        return;
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_ulIpAddr = pEventData->ip;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP = %d.%d.%d.%d, "
                        "Gateway = %d.%d.%d.%d\r\n",
                            SL_IPV4_BYTE(g_ulIpAddr,3),
                            SL_IPV4_BYTE(g_ulIpAddr,2),
                            SL_IPV4_BYTE(g_ulIpAddr,1),
                            SL_IPV4_BYTE(g_ulIpAddr,0),
                            SL_IPV4_BYTE(g_ulGatewayIP,3),
                            SL_IPV4_BYTE(g_ulGatewayIP,2),
                            SL_IPV4_BYTE(g_ulGatewayIP,1),
                            SL_IPV4_BYTE(g_ulGatewayIP,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x]\r\n",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(!pDevEvent)
    {
        return;
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\r\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(!pSock)
    {
        return;
    }

    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\r\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \r\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\r\n",pSock->Event);
          break;
    }

}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

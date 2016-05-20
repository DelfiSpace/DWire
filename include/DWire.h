/*
 * DWire.h
 *
 *  Created on: 16 May 2016
 *      Author: stefa_000
 */

#ifndef DWIRE_DWIRE_H_
#define DWIRE_DWIRE_H_

#define USING_EUSCI_B0
#undef USING_EUSCI_B1
#undef USING_EUSCI_B2
#undef USING_EUSCI_B3

// Bus status: simple int as multiple objects can have different discrete statuses (so not const)
#define BUS_STATUS_NONE 0
#define BUS_STATUS_RDY 1
#define BUS_STATUS_TX 2
#define BUS_STATUS_RX 3

// Similar for the roles
#define BUS_ROLE_MASTER 0
#define BUS_ROLE_SLAVE 1

// Default buffer size in bytes
#define TX_BUFFER_SIZE 32
#define RX_BUFFER_SIZE 32

/* Driverlib */
#include "driverlib.h"

/* Main class definition */
class DWire {
private:
    uint_fast32_t module;

    uint8_t * pTxBufferIndex;
    uint8_t * pTxBuffer;
    uint8_t * pTxBufferSize;

    volatile uint8_t rxReadIndex;
    volatile uint8_t rxReadLength;
    uint8_t * rxBuffer;

    uint8_t slaveAddress;

    uint8_t status;
    uint8_t busRole;

    void (*user_onRequest)( void );
    void (*user_onReceive)( uint8_t );

    void _initMaster( const eUSCI_I2C_MasterConfig * );
    void _initSlave( void );
    void _setSlaveAddress( uint_fast8_t );

    void _handleRequest( void );

public:
    DWire( uint32_t );
    ~DWire( );
    void begin( void );
    void begin( uint8_t );

    void beginTransmission( uint_fast8_t );
    void write( uint8_t );
    void endTransmission( void );

    uint8_t read( void );

    void onRequest( void (*)( void ) );
    void onReceive( void (*)( uint8_t ) );

    void _handleReceive( uint8_t *, uint8_t * );

};

#endif /* DWIRE_DWIRE_H_ */

/*
 * DWire.cpp
 *
 *  Created on: 16 May 2016
 *      Author: Stefan van der Linden
 *
 * DWire (Delft-Wire)
 * Provides a simple library for handling I2C, trying to be similar to Arduino's Wire library
 * Many of the variable names and methods are kept identical to the original (Two)Wire.
 *
 */

/* Standard includes */
extern "C" {
#include <stdint.h>
#include <stdbool.h>
}

#include <hash_map>

#include "DWire.h"

/**** PROTOTYPES AND CLASSES ****/

typedef struct {
    uint32_t module;
    uint8_t * rxBuffer;
    uint8_t * rxBufferIndex;
    uint8_t * rxBufferSize;
    uint8_t * txBuffer;
    uint8_t * txBufferIndex;
    uint8_t * txBufferSize;
} IRQParam;

void IRQHandler( IRQParam );

/**** GLOBAL VARIABLES ****/

// The buffers need to be declared globally, as the interrupts are too
#ifdef USING_EUSCI_B0

uint8_t * EUSCIB0_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB0_txBufferIndex = 0;
uint8_t EUSCIB0_txBufferSize = 0;

uint8_t * EUSCIB0_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB0_rxBufferIndex = 0;
uint8_t EUSCIB0_rxBufferSize = 0;
#endif

#ifdef USING_EUSCI_B1

uint8_t * EUSCIB1_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB1_txBufferIndex = 0;
uint8_t EUSCIB1_txBufferSize = 0;

uint8_t * EUSCIB1_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB1_rxBufferIndex = 0;
uint8_t EUSCIB1_rxBufferSize = 0;
#endif

#ifdef USING_EUSCI_B2

uint8_t * EUSCIB2_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB2_txBufferIndex = 0;
uint8_t EUSCIB2_txBufferSize = 0;

uint8_t * EUSCIB2_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB2_rxBufferIndex = 0;
uint8_t EUSCIB2_rxBufferSize = 0;
#endif

#ifdef USING_EUSCI_B3

uint8_t * EUSCIB3_txBuffer = new uint8_t[TX_BUFFER_SIZE];
uint8_t EUSCIB3_txBufferIndex = 0;
uint8_t EUSCIB3_txBufferSize = 0;

uint8_t * EUSCIB3_rxBuffer = new uint8_t[RX_BUFFER_SIZE];
uint8_t EUSCIB3_rxBufferIndex = 0;
uint8_t EUSCIB3_rxBufferSize = 0;
#endif

// TODO find an efficient way of getting rid of the hashmap
static std::hash_map<uint32_t, DWire *> moduleMap;

// The default eUSCI settings
eUSCI_I2C_MasterConfig i2cConfig = {
EUSCI_B_I2C_CLOCKSOURCE_SMCLK,                   // SMCLK Clock Source
        MAP_CS_getSMCLK( ),                 // Get the SMCLK clock frequency
        EUSCI_B_I2C_SET_DATA_RATE_400KBPS, // Desired I2C Clock of 400khz // TODO make configurable
        0,                                      // No byte counter threshold
        EUSCI_B_I2C_NO_AUTO_STOP                 // No Autostop
        };

/**** CONSTRUCTORS ****/

DWire::DWire( uint32_t module ) {

    this->module = module;

    // The receiver buffer and related variables
    rxReadIndex = 0;
    rxReadLength = 0;
    rxLocalBuffer = new uint8_t[RX_BUFFER_SIZE];

    slaveAddress = 0;

    busRole = 0;

    requestDone = false;

    switch ( module ) {
#ifdef USING_EUSCI_B0
    case EUSCI_B0_BASE:
        pTxBuffer = EUSCIB0_txBuffer;
        pTxBufferIndex = &EUSCIB0_txBufferIndex;
        pTxBufferSize = &EUSCIB0_txBufferSize;

        pRxBuffer = EUSCIB0_rxBuffer;
        pRxBufferIndex = &EUSCIB0_rxBufferIndex;
        pRxBufferSize = &EUSCIB0_rxBufferSize;
        break;
#endif
#ifdef USING_EUSCI_B1
        case EUSCI_B1_BASE:
        pTxBuffer = EUSCIB1_txBuffer;
        pTxBufferIndex = &EUSCIB1_txBufferIndex;
        pTxBufferSize = &EUSCIB1_txBufferSize;
        break;
#endif
#ifdef USING_EUSCI_B2
        case EUSCI_B2_BASE:
        pTxBuffer = EUSCIB2_txBuffer;
        pTxBufferIndex = &EUSCIB2_txBufferIndex;
        pTxBufferSize = &EUSCIB2_txBufferSize;
        break;
#endif
#ifdef USING_EUSCI_B3
        case EUSCI_B3_BASE:
        pTxBuffer = EUSCIB3_txBuffer;
        pTxBufferIndex = &EUSCIB3_txBufferIndex;
        pTxBufferSize = &EUSCIB3_txBufferSize;
        break;
#endif
    default:
        return;
    }

    // Register this instance in the 'moduleMap'
    moduleMap[module] = this;
}

DWire::~DWire( ) {
    // Deregister from the moduleMap
    moduleMap[module] = 0;
}

/**** PUBLIC METHODS ****/

void DWire::begin( void ) {
    // Initialising the given module as a master
    busRole = BUS_ROLE_MASTER;

    _initMaster((const eUSCI_I2C_MasterConfig *) &i2cConfig);
}

void DWire::begin( uint8_t address ) {
    // Initialising the given module as a slave
    busRole = BUS_ROLE_SLAVE;
    slaveAddress = address;
    _initSlave( );
}

/**
 * Begin a transmission as a master
 */
void DWire::beginTransmission( uint_fast8_t slaveAddress ) {
    // Starting a transmission as a master to the slave at slaveAddress
    if ( busRole != BUS_ROLE_MASTER )
        return;

    // Wait in case a previous message is still being sent
    while ( MAP_I2C_masterIsStopSent(module) == EUSCI_B_I2C_SENDING_STOP )
        ;

    if ( slaveAddress != this->slaveAddress )
        _setSlaveAddress(slaveAddress);
}

/**
 * Write a single byte
 */
void DWire::write( uint8_t dataByte ) {
    // Add data to the tx buffer
    pTxBuffer[*pTxBufferIndex] = dataByte;
    (*pTxBufferIndex)++;
}

/**
 * End the transmission and transmit the tx buffer's contents over the bus
 */
void DWire::endTransmission( ) {

    // Wait until any ongoing (incoming) transmissions are finished
    while ( MAP_I2C_isBusBusy(module) == EUSCI_B_I2C_BUS_BUSY )
        ;

    // Send the start condition and initial byte
    (*pTxBufferSize) = *pTxBufferIndex;
    (*pTxBufferIndex)--;

    // Send the first byte, triggering the TX interrupt
    MAP_I2C_masterSendMultiByteStart(module, pTxBuffer[0]);
}

/**
 * Request data from a SLAVE as a MASTER
 */
uint8_t * DWire::requestFrom( uint_fast8_t slaveAddress,
        uint_fast8_t numBytes ) {
    // No point of doing anything else if there we're not a MASTER
    if ( busRole != BUS_ROLE_MASTER )
        return 0;

    // Re-initialise the rx buffer
    *pRxBufferSize = numBytes;
    *pRxBufferIndex = 0;

    MAP_I2C_disableModule(module);

    // Configure the autostop and restart the module
    i2cConfig.byteCounterThreshold = numBytes;
    i2cConfig.autoSTOPGeneration =
    EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD;

    MAP_I2C_initMaster(module, (const eUSCI_I2C_MasterConfig *) &i2cConfig);

    MAP_I2C_enableModule(module);

    // Configure the correct slave
    MAP_I2C_setSlaveAddress(module, slaveAddress);
    this->slaveAddress = slaveAddress;

    // Set the master into receive mode
    MAP_I2C_setMode(module, EUSCI_B_I2C_RECEIVE_MODE);

    // Initialise the correct interrupt
    MAP_I2C_clearInterruptFlag(module,
    EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);

    // Enable the receive interrupt, triggered when the slave responds with data
    MAP_I2C_enableInterrupt(module,
    EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);

    // Send the START
    MAP_I2C_masterReceiveStart(module);

    // Initialize the flag showing the status of the request
    requestDone = false;

    while ( !requestDone )
        ;

    MAP_I2C_disableInterrupt(module,
    EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);
    MAP_I2C_disableModule(module);

    for ( int i = 0; i <= *pRxBufferSize; i++ ) {
        if ( pRxBuffer[i] == 0 ) {
            this->rxLocalBuffer[i] = 0x30;
        } else {
            this->rxLocalBuffer[i] = pRxBuffer[i];
        }
    }

    // Reset the buffer
    (*pRxBufferIndex) = 0;
    (*pRxBufferSize) = 0;

    // Configure the autostop and restart the module
    i2cConfig.byteCounterThreshold = 0;
    i2cConfig.autoSTOPGeneration = EUSCI_B_I2C_NO_AUTO_STOP;

    MAP_I2C_initMaster(module, (const eUSCI_I2C_MasterConfig *) &i2cConfig);

    // Configure the correct slave
    MAP_I2C_setSlaveAddress(module, slaveAddress);
    this->slaveAddress = slaveAddress;

    // Set the master back into transmit mode
    MAP_I2C_setMode(module, EUSCI_B_I2C_TRANSMIT_MODE);

    return rxLocalBuffer;
}

/**
 * Reads a single byte from the rx buffer
 */
uint8_t DWire::read( void ) {

    // Wait if there is nothing to read
    while ( rxReadIndex == 0 && rxReadLength == 0 )
        ;

    uint8_t byte = rxLocalBuffer[rxReadIndex];
    rxReadIndex++;

    // Check whether this was the last byte. If so, reset.
    if ( rxReadIndex == rxReadLength ) {
        rxReadIndex = 0;
        rxReadLength = 0;
    }
    return byte;
}

/**
 * Register the user's interrupt handler
 */
void DWire::onRequest( void (*islHandle)( void ) ) {
    user_onRequest = islHandle;
}

/**
 * Register the interrupt handler
 * The argument contains the number of bytes received
 */
void DWire::onReceive( void (*islHandle)( uint8_t ) ) {
    user_onReceive = islHandle;
}

/**
 * Returns true if the module is configured as a master
 */
bool DWire::isMaster( void ) {
    if ( busRole == BUS_ROLE_MASTER ) {
        return true;
    } else {
        return false;
    }
}

/**** PRIVATE METHODS ****/

/**
 * Called to set the eUSCI module in 'master' mode
 */
void DWire::_initMaster( const eUSCI_I2C_MasterConfig * i2cConfig ) {

    // Initialise the pins
    // TODO make customisable, as these are only used by eUSCI B0
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    // Initializing I2C Master to SMCLK at 400kbs with no autostop
    MAP_I2C_initMaster(module, i2cConfig);

    // Specify slave address
    MAP_I2C_setSlaveAddress(module, 0);

    // Set Master in transmit mode
    MAP_I2C_setMode(module, EUSCI_B_I2C_TRANSMIT_MODE);

    // Enable I2C Module to start operations
    MAP_I2C_enableModule(module);

    // Enable and clear the interrupt flag
    MAP_I2C_clearInterruptFlag(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Enable master interrupts
    MAP_I2C_enableInterrupt(module,
    EUSCI_B_I2C_TRANSMIT_INTERRUPT0 + EUSCI_B_I2C_NAK_INTERRUPT);

    // Register the interrupts on the correct module
    uint32_t intModule;
    switch ( module ) {
    case EUSCI_B0_BASE:
        intModule = INT_EUSCIB0;
        break;
    case EUSCI_B1_BASE:
        intModule = INT_EUSCIB1;
        break;
    case EUSCI_B2_BASE:
        intModule = INT_EUSCIB2;
        break;
    case EUSCI_B3_BASE:
        intModule = INT_EUSCIB3;
        break;
    default:
        return;
    }
    MAP_Interrupt_enableInterrupt(intModule);
}

void DWire::_initSlave( void ) {
    // Init the pins
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
    GPIO_PIN6 + GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    // initialise driverlib
    MAP_I2C_initSlave(module, slaveAddress, EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
    EUSCI_B_I2C_OWN_ADDRESS_ENABLE);

    // Enable the module
    MAP_I2C_enableModule(module);

    // Enable the module and enable interrupts
    MAP_I2C_enableModule(module);
    MAP_I2C_clearInterruptFlag(module,
            EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_STOP_INTERRUPT
                    | EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
    MAP_I2C_enableInterrupt(module,
            EUSCI_B_I2C_RECEIVE_INTERRUPT0 | EUSCI_B_I2C_STOP_INTERRUPT
                    | EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
    MAP_Interrupt_enableSleepOnIsrExit( );
    MAP_Interrupt_enableInterrupt(INT_EUSCIB0);
    MAP_Interrupt_enableMaster( );
}

/**
 * Re-set the slave address (the target address when master or the slave's address when slave)
 */
void DWire::_setSlaveAddress( uint_fast8_t newAddress ) {
    slaveAddress = newAddress;
    MAP_I2C_setSlaveAddress(module, newAddress);
}

/**
 * Handle a request ISL as a slave
 */
void DWire::_handleRequestSlave( void ) {
    if ( !user_onRequest )
        return;
    if ( !(*pTxBufferIndex) ) {
        user_onRequest( );

        *pTxBufferSize = *pTxBufferIndex;
        *pTxBufferIndex = 0;
    }

    if ( *pTxBufferIndex == *pTxBufferSize ) {
        *pTxBufferIndex = 0;
        *pTxBufferSize = 0;
    } else {
        MAP_I2C_slavePutData(module, pTxBuffer[*pTxBufferIndex]);
        (*pTxBufferIndex)++;
    }
}

/**
 * Internal process handling the rx buffers, and calling the user's interrupt handles
 */
void DWire::_handleReceive( uint8_t * rxBuffer ) {
    // No need to do anything if there is no handler registered
    if ( !user_onReceive )
        return;

    // Check whether the user application is still reading out the local buffer.
    // This needs to be tested to make sure it doesn't give any problems.
    if ( rxReadIndex != 0 && rxReadLength != 0 )
        return;

    // Copy the main buffer into a local buffer
    rxReadLength = *pRxBufferIndex;
    rxReadIndex = 0;

    for ( int i = 0; i < rxReadLength; i++ )
        this->rxLocalBuffer[i] = rxBuffer[i];

    // Reset the main buffer
    (*pRxBufferIndex) = 0;

    user_onReceive(rxReadLength);
}

void DWire::_markRequestDone( void ) {
    requestDone = true;
}

/**** ISR/IRQ Handles ****/

/**
 * The main (global) interrupt  handler
 */
void IRQHandler( IRQParam param ) {

    uint_fast16_t status;

    status = MAP_I2C_getEnabledInterruptStatus(param.module);
    MAP_I2C_clearInterruptFlag(param.module, status);

    /* RXIFG */
    // Triggered when data has been received
    if ( status & EUSCI_B_I2C_RECEIVE_INTERRUPT0 ) {

        // If the rxBufferSize > 0, then we're a master performing a request
        if ( *param.rxBufferSize > 0 ) {
            param.rxBuffer[*param.rxBufferIndex] =
            MAP_I2C_masterReceiveMultiByteNext(param.module);
            (*param.rxBufferIndex)++;

            // Otherwise we're a slave receiving data
        } else {
            param.rxBuffer[*param.rxBufferIndex] = MAP_I2C_slaveGetData(
                    param.module);
            (*param.rxBufferIndex)++;
        }
    }

    // As master: triggered when a byte has been transmitted
    // As slave: triggered on request */
    if ( status & EUSCI_B_I2C_TRANSMIT_INTERRUPT0 ) {
        DWire * instance = moduleMap[param.module];

        // If the module is setup as a master, then we're transmitting data
        if ( instance->isMaster( ) ) {
            // If we've transmitted the last byte from the buffer, then send a stop
            if ( !(*param.txBufferIndex) ) {
                MAP_I2C_masterSendMultiByteStop(param.module);

            } else {
                // If we still have data left in the buffer, then transmit that
                MAP_I2C_masterSendMultiByteNext(param.module,
                        param.txBuffer[(*param.txBufferSize)
                                - (*param.txBufferIndex)]);
                (*param.txBufferIndex)--;
            }
            // Otherwise we're a slave and a master is requesting data
        } else {
            instance->_handleRequestSlave( );
        }
    }

    if ( status & EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT ) {
        DWire * instance = moduleMap[param.module];
        if ( instance ) {
            instance->_markRequestDone( );
        }
    }

    // Handle a NAK
    if ( status & EUSCI_B_I2C_NAK_INTERRUPT ) {
        MAP_I2C_masterSendStart(param.module);
        // TODO verify whether this is enough or we need to bring back the buffer by one
    }

    /* STPIFG
     * Called when a STOP is received
     */
    if ( status & EUSCI_B_I2C_STOP_INTERRUPT ) {
        if( *param.txBufferIndex != 0) {

        } else if ( *param.rxBufferIndex != 0 ) {
            DWire * instance = moduleMap[param.module];
            if ( instance ) {
                instance->_handleReceive(param.rxBuffer);
            }
        }
    }
}

#ifdef USING_EUSCI_B0
/*
 * Handle everything on EUSCI_B0
 */
extern "C" {
void EUSCIB0_IRQHandler( void ) {
    IRQParam param;
    param.module = EUSCI_B0_BASE;
    param.rxBuffer = EUSCIB0_rxBuffer;
    param.rxBufferIndex = &EUSCIB0_rxBufferIndex;
    param.rxBufferSize = &EUSCIB0_rxBufferSize;
    param.txBuffer = EUSCIB0_txBuffer;
    param.txBufferIndex = &EUSCIB0_txBufferIndex;
    param.txBufferSize = &EUSCIB0_txBufferSize;

    IRQHandler(param);
}
}

/* USING_EUSCI_B0 */
#endif

#ifdef USING_EUSCI_B1
/*
 * Handle everything on EUSCI_B1
 */
extern "C" {
    void EUSCIB1_IRQHandler( void ) {
        IRQParam param;
        param.module = EUSCI_1_BASE;
        param.rxBuffer = EUSCIB1_rxBuffer;
        param.rxBufferIndex = &EUSCIB1_rxBufferIndex;
        param.rxBufferSize = &EUSCIB1_rxBufferSize;
        param.txBuffer = EUSCIB1_txBuffer;
        param.txBufferIndex = &EUSCIB1_txBufferIndex;
        param.txBufferSize = &EUSCIB1_txBufferSize;

        IRQHandler(param);
    }
}

/* USING_EUSCI_B1 */
#endif

#ifdef USING_EUSCI_B2
/*
 * Handle everything on EUSCI_B2
 */
extern "C" {
    void EUSCIB2_IRQHandler( void ) {
        IRQParam param;
        param.module = EUSCI_B2_BASE;
        param.rxBuffer = EUSCIB2_rxBuffer;
        param.rxBufferIndex = &EUSCIB2_rxBufferIndex;
        param.rxBufferSize = &EUSCIB2_rxBufferSize;
        param.txBuffer = EUSCIB2_txBuffer;
        param.txBufferIndex = &EUSCIB2_txBufferIndex;
        param.txBufferSize = &EUSCIB2_txBufferSize;

        IRQHandler(param);
    }
}

/* USING_EUSCI_B2 */
#endif

#ifdef USING_EUSCI_B3
/*
 * Handle everything on EUSCI_B3
 */
extern "C" {
    void EUSCIB3_IRQHandler( void ) {
        IRQParam param;
        param.module = EUSCI_B3_BASE;
        param.rxBuffer = EUSCIB3_rxBuffer;
        param.rxBufferIndex = &EUSCIB3_rxBufferIndex;
        param.rxBufferSize = &EUSCIB3_rxBufferSize;
        param.txBuffer = EUSCIB3_txBuffer;
        param.txBufferIndex = &EUSCIB3_txBufferIndex;
        param.txBufferSize = &EUSCIB3_txBufferSize;

        IRQHandler(param);
    }
}

/* USING_EUSCI_B0 */
#endif

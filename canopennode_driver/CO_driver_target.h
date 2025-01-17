#ifndef CO_DRIVER_TARGET_H_
#define CO_DRIVER_TARGET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SLCAN_BUFFER_SIZE 200

#if defined(_WIN32)
#include "windows.h"
#else
#include <pthread.h>
#endif

/* Basic definitions. If big endian, CO_SWAP_xx macros must swap bytes. */
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) x
#define CO_SWAP_32(x) x
#define CO_SWAP_64(x) x
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
typedef uint_fast8_t bool_t;
typedef float float32_t;
typedef double float64_t;

/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg) (((CO_CANrx_t *)msg)->ident)
#define CO_CANrxMsg_readDLC(msg) (((CO_CANrx_t *)msg)->DLC)
#define CO_CANrxMsg_readData(msg) (((CO_CANrx_t *)msg)->data)

typedef struct
{
	uint32_t ident;
	uint32_t mask;
	uint8_t DLC;
	uint8_t data[8];
	void *object;
	void (*CANrx_callback)(void *object, void *message);
} CO_CANrx_t;

typedef struct
{
	uint32_t ident;
	uint8_t DLC;
	uint8_t data[8];
	volatile bool_t bufferFull;
	volatile bool_t syncFlag;
} CO_CANtx_t;

/* CAN module object */
typedef struct
{
	void *CANptr; // typeof(sp_t)
	CO_CANrx_t *rxArray;
	uint16_t rxSize;
	CO_CANtx_t *txArray;
	uint16_t txSize;
	uint16_t CANerrorStatus;
	volatile bool_t CANnormal;
	volatile bool_t useCANrxFilters;
	volatile bool_t bufferInhibitFlag;
	volatile bool_t firstCANtxMessage;
	volatile uint16_t CANtxCount;
	uint32_t errOld;
	bool rx_ovf;

	volatile bool thr_run;
#if defined(_WIN32)
	HANDLE thr_rcv;
#else
	pthread_t thr_rcv;
	volatile bool thr_exited;
#endif

	struct
	{
		uint8_t buf[SLCAN_BUFFER_SIZE + 1];
		int32_t pos;
	} slcan;
} CO_CANmodule_t;

/* Data storage object for one entry */
typedef struct
{
	void *addr;
	size_t len;
	uint8_t subIndexOD;
	uint8_t attr;
	/* Additional variables (target specific) */
	void *addrNV;
} CO_storage_entry_t;

/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND(CAN_MODULE)
#define CO_UNLOCK_CAN_SEND(CAN_MODULE)

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY(CAN_MODULE)
#define CO_UNLOCK_EMCY(CAN_MODULE)

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD(CAN_MODULE)
#define CO_UNLOCK_OD(CAN_MODULE)

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew)  \
	{                       \
		CO_MemoryBarrier(); \
		rxNew = (void *)1L; \
	}
#define CO_FLAG_CLEAR(rxNew) \
	{                        \
		CO_MemoryBarrier();  \
		rxNew = NULL;        \
	}

#endif // CO_DRIVER_TARGET_H_

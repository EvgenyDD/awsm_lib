#ifndef CO_TERM_H__
#define CO_TERM_H__

#include "CO_driver_target.h"
#include "rb.h"

#ifdef CO_CONFIG_TERM
#define CANOPEN_TIME_TERMINAL_MACRO()                                \
	if(DLC >= 2)                                                     \
	{                                                                \
		co_term_append_req(TIME->t, CO_CANrxMsg_readData(msg), DLC); \
		return;                                                      \
	}
#else
#define CANOPEN_TIME_TERMINAL_MACRO() ;
#endif

#define CO_TX_CNT_TERM (1)
#define CO_RX_CNT_TERM_LISTENER (1) // terminal listener ()

typedef struct
{
	CO_CANmodule_t *module;
	CO_CANtx_t *co_term_tx_buf;
#ifdef CO_CONFIG_TERM

#ifdef CO_CONFIG_TERM_REQUESTER
	CO_CANtx_t *co_time_tx_buf;
	uint8_t req_id;
	RB_DECL_INST(tx_req, CO_CONFIG_TERM_RX_SZ); // send master request
#endif

	uint8_t rx[CO_CONFIG_TERM_RX_SZ]; // receive master request
	uint32_t rx_cnt;

	RB_DECL_INST(tx_info, CO_CONFIG_TERM_TX_SZ); // send info
#endif
} CO_term_t;

#ifdef CO_CONFIG_TERM_REQUESTER
void co_term_init(CO_term_t *t, CO_CANmodule_t *co_module, CO_CANtx_t *co_time_tx_buf);
#else
void co_term_init(CO_term_t *t, CO_CANmodule_t *co_module);
#endif
void co_term_append_req(CO_term_t *t, const uint8_t *data, uint8_t dlc);
void co_term_send(CO_term_t *t, const uint8_t *data, uint32_t len);
void co_term_poll(CO_term_t *t);
#ifdef CO_CONFIG_TERM_REQUESTER
void co_term_master_request(CO_term_t *t, uint8_t id, const uint8_t *data, uint32_t len);
#endif

extern void co_term_process_rx_req(CO_term_t *t, const uint8_t *data, uint32_t len);
extern void co_term_listener_rx(CO_term_t *t, uint8_t id, const uint8_t *data, uint32_t len);

#endif // CO_TERM_H__

/**
 * Description:
 * terminal uses time object 0x100 + pool 0x101..0x17F
 * messages can be fragmented
 *
 * master send message to slaves with COB-ID = 0x100, data[0] = target id, data[1]...data[7] is payload. '\0' is request end.
 * note: payload can't be CO_TIME_MSG_LENGTH-1 length (or it will be parsed as time object)
 *
 * terminal receiver listens to COB-ID 0x100...0x17F
 *
 * non-master sends info with COB-ID = 0x100 + self_id
 * non-master listens for COB-ID = 0x100 if input terminal enabled
 */

/**
 * Integration notes:
 * insert CANOPEN_TIME_TERMINAL_MACRO(); to CO_TIME.c
 * insert #include "co_term.h" and CO_term_t *t; to CO_TIME.h
 * insert #include "co_term.h" and #ifdef CO_CONFIG_TERM CO_term_t term; #endif to CANopen.h
 * modify #define CO_CNT_ALL_TX_MSGS in CANopen.c
 * modify #define CO_CNT_ALL_RX_MSGS in CANopen.c
 *
			CO->TIME->t = &CO->term;
			co_term_init(&CO->term, CO->CANmodule);
 *
 * define in Makefile
CO_CONFIG_TERM
CO_CONFIG_TERM_LISTENER
CO_CONFIG_TERM_REQUESTER
CO_CONFIG_TERM_RX_SZ=
CO_CONFIG_TERM_TX_SZ=
 * change    .x1012_COB_IDTimeStampObject = 0xC0000100,
 */
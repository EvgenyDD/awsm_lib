#include "co_term.h"
#include "301/CO_TIME.h"
#include "301/CO_driver.h"
#include <string.h>

extern uint8_t g_active_can_node_id;

#ifdef CO_CONFIG_TERM

#ifdef CO_CONFIG_TERM_LISTENER
static void term_rx(void *obj, void *msg)
{
	CO_term_t *t = (CO_term_t *)obj;
	uint8_t DLC = CO_CANrxMsg_readDLC(msg);
	co_term_listener_rx(t, CO_CANrxMsg_readIdent(msg) & 0x7F, CO_CANrxMsg_readData(msg), DLC);
}
#endif

#ifdef CO_CONFIG_TERM_REQUESTER
void co_term_init(CO_term_t *t, CO_CANmodule_t *co_module, CO_CANtx_t *co_time_tx_buf)
#else
void co_term_init(CO_term_t *t, CO_CANmodule_t *co_module)
#endif
{
	t->module = co_module;
	t->rx_cnt = 0;
	RB_INIT(t->tx_info);

#ifdef CO_CONFIG_TERM_REQUESTER
	RB_INIT(t->tx_req);
	t->co_time_tx_buf = co_time_tx_buf;
#endif

	t->co_term_tx_buf = CO_CANtxBufferInit(
		co_module,					  /* CAN device */
		co_module->txSize - 1,		  /* index of specific buffer inside CAN module */
		0x100 + g_active_can_node_id, /* CAN identifier */
		0,							  /* rtr */
		8,							  /* number of data bytes */
		0);							  /* synchronous message flag bit */
#ifdef CO_CONFIG_TERM_LISTENER
	CO_CANrxBufferInit(
		co_module,			   /* CAN device */
		co_module->rxSize - 1, /* rx buffer index */
		0x100,				   /* CAN identifier */
		0x780,				   /* mask */
		0,					   /* rtr */
		(void *)t,			   /* object passed to receive function */
		term_rx);			   /* this function will process received message */
#endif
}

void co_term_append_req(CO_term_t *t, const uint8_t *data, uint8_t dlc)
{
	if(dlc > 1)
	{
		if(g_active_can_node_id == data[0]) // msg match
		{
			for(uint32_t i = 1; i < dlc; i++)
			{
				t->rx[t->rx_cnt++] = data[i];
				if(data[i] == '\0')
				{
					co_term_process_rx_req(t, t->rx, t->rx_cnt);
					t->rx_cnt = 0;
				}
				if(t->rx_cnt == CO_CONFIG_TERM_RX_SZ) t->rx_cnt = 0;
			}
		}
	}
}

void co_term_send(CO_term_t *t, const uint8_t *data, uint32_t len)
{
	RB_APPEND(t->tx_info, data, len);
}

#ifdef CO_CONFIG_TERM_REQUESTER
void co_term_master_request(CO_term_t *t, uint8_t id, const uint8_t *data, uint32_t len)
{
	t->req_id = id; // ! potential bug
	RB_APPEND(t->tx_req, data, len);
}
#endif

void co_term_poll(CO_term_t *t)
{
	if(t->co_term_tx_buf->bufferFull == false && RB_NOT_EMPTY(t->tx_info))
	{
		RB_CONSUME(t->tx_info, t->co_term_tx_buf->data, 8, t->co_term_tx_buf->DLC);
		CO_CANsend(t->module, t->co_term_tx_buf);
	}
#ifdef CO_CONFIG_TERM_REQUESTER
	if(t->co_time_tx_buf->bufferFull == false && RB_NOT_EMPTY(t->tx_req))
	{
		RB_CONSUME(t->tx_req, (&t->co_time_tx_buf->data[1]), 7, t->co_time_tx_buf->DLC);
		t->co_time_tx_buf->data[0] = t->req_id;
		t->co_time_tx_buf->DLC++; // 0st byte is ID
		if(t->co_time_tx_buf->DLC == 6) t->co_time_tx_buf->data[t->co_time_tx_buf->DLC++] = '\0';
		CO_CANsend(t->module, t->co_time_tx_buf);
		t->co_time_tx_buf->DLC = 6; // restore back
	}
#endif
}

#endif
#ifndef __DR_H__
#define __DR_H__

/// Data Router Library

#define DBG_PFX "\t..."

#include "rb_var.h"
#include <algorithm>
#include <list>
#include <shared_mutex>

// #define DBG_DR

typedef void (*cb_rx_t)(const void *data, size_t size);

struct DRRx  
{
	std::string name_tx;
	std::string name_pipe;
	bool synchronous = false;
	bool enabled = true;
	std::mutex rx_mutex;
	cb_rx_t cb;
	rb_t rb; // ring buffer for asynchronous operations
};

typedef std::list<DRRx>::iterator rx_t;

struct DRTx
{
	std::string name_tx;
	std::string name_pipe;
	std::list<rx_t> receivers;
};

typedef std::list<DRTx>::iterator tx_t;

struct DRStorage
{
	static DRStorage &instance()
	{
		static DRStorage inst;
		return inst;
	}

	void lookup_connections(void)
	{
		for(tx_t it_tx = instance().tx.begin(); it_tx != instance().tx.end(); it_tx++)
		{
			for(rx_t it_rx = instance().rx.begin(); it_rx != instance().rx.end(); it_rx++)
			{
				if(it_rx->name_pipe != it_tx->name_pipe) continue;
				if(it_rx->name_tx != "" && it_rx->name_tx != it_tx->name_tx) continue;
				bool found = (std::find(it_tx->receivers.begin(), it_tx->receivers.end(), it_rx) != it_tx->receivers.end());
				if(found == false)
				{
					it_tx->receivers.push_back(it_rx);
#ifdef DBG_DR
					printf(DBG_PFX "Added R '%s':'%s' to T '%s':'%s'\n",
						   it_rx->name_tx.c_str(), it_rx->name_pipe.c_str(),
						   it_tx->name_tx.c_str(), it_tx->name_pipe.c_str());
#endif
				}
			}
		}
	}

	void remove_connections_rx(rx_t tgt_rx)
	{
		for(tx_t it_tx = instance().tx.begin(); it_tx != instance().tx.end(); it_tx++)
		{
			for(auto it_rx = it_tx->receivers.begin(); it_rx != it_tx->receivers.end(); it_rx++)
			{
				if(*it_rx != tgt_rx) continue;
#ifdef DBG_DR
				printf(DBG_PFX "Removed R %s:%s from T %s:%s\n",
					   rx->name_tx.c_str(), rx->name_pipe.c_str(),
					   it_tx->name_tx.c_str(), it_tx->name_pipe.c_str());
#endif
				it_tx->receivers.erase(it_rx++); // remove pointer from the recipient list
			}
		}
	}

	std::list<DRTx> tx;
	std::list<DRRx> rx;
	std::shared_mutex mtx;
};

tx_t dr_tx_push(std::string name_tx, std::string name_pipe);
void dr_tx_pop(tx_t &tx);

rx_t dr_rx_push_sync(std::string name_tx, std::string name_pipe, cb_rx_t cb);
rx_t dr_rx_push_async(std::string name_tx, std::string name_pipe, size_t rb_size);
void dr_rx_pop(rx_t &rx);
void dr_rx_set(rx_t rx, bool state); // enable/disable

int dr_tx(tx_t tx, const void *msg, size_t size);

size_t dr_is_avail(rx_t rx, const void **data);
void dr_consume(rx_t rx);

#endif // __DR_H__
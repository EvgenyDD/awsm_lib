#include "dr.h"
#include <cstring>

tx_t dr_tx_push(std::string name_tx, std::string name_pipe)
{
	auto &inst = DRStorage::instance();
	std::lock_guard<std::shared_mutex> lk(inst.mtx);
	for(tx_t it = inst.tx.begin(); it != inst.tx.end(); it++)
	{
		if(it->name_tx == name_tx && it->name_pipe == name_pipe)
		{
			// printf(LOG_ERROR "TX (%s) with pipe (%s) already exists!", name_tx.c_str(), name_pipe.c_str());
			return /*inst.tx.end();*/ it;
		}
	}
	inst.tx.emplace_back();
	inst.tx.back().name_tx = name_tx;
	inst.tx.back().name_pipe = name_pipe;
#ifdef DBG_DR
	printf(DBG_PFX "created tx '%s' '%s'\n",
		   inst.tx.back().name_tx.c_str(),
		   inst.tx.back().name_pipe.c_str());
#endif
	inst.lookup_connections();
	return --inst.tx.end();
}
void dr_tx_pop(tx_t &it_tx)
{
	auto &inst = DRStorage::instance();
	std::lock_guard<std::shared_mutex> lk(inst.mtx);
	if(it_tx == inst.tx.end())
	{
		printf(DBG_PFX "ERROR deleting T!\n");
		return;
	}
	inst.tx.erase(it_tx);
	it_tx = inst.tx.end();
}

static rx_t _dr_rx_push(std::string name_tx, std::string name_pipe, cb_rx_t cb, size_t rb_size)
{
	auto &inst = DRStorage::instance();
	std::lock_guard<std::shared_mutex> lk(inst.mtx);
	inst.rx.emplace_back();
	inst.rx.back().name_pipe = name_pipe;
	inst.rx.back().synchronous = cb != nullptr;
	inst.rx.back().cb = cb;
	// if(inst.rx.back().synchronous == false)
	{
		memset(&inst.rx.back().rb, 0, sizeof(rb_t));
		inst.rx.back().rb.size = rb_size;
		inst.rx.back().rb.pdata = malloc(rb_size);
	}
#ifdef DBG_DR
	printf(DBG_PFX "created rx %csync '%s' '%s'\n", cb != nullptr ? 's' : 'a', inst.rx.back().name_tx.c_str(), inst.rx.back().name_pipe.c_str());
#endif
	inst.lookup_connections();
	return --inst.rx.end();
}

rx_t dr_rx_push_sync(std::string name_tx, std::string name_pipe, cb_rx_t cb) { return _dr_rx_push(name_tx, name_pipe, cb, 0); }
rx_t dr_rx_push_async(std::string name_tx, std::string name_pipe, size_t rb_size) { return _dr_rx_push(name_tx, name_pipe, nullptr, rb_size); }

void dr_rx_set(rx_t it_rx, bool state) // enable/disable
{
	std::lock_guard<std::shared_mutex> lk(DRStorage::instance().mtx);
	it_rx->enabled = state;
}

void dr_rx_pop(rx_t &it_rx)
{
	std::lock_guard<std::shared_mutex> lk(DRStorage::instance().mtx);
	if(it_rx == DRStorage::instance().rx.end())
	{
		printf(DBG_PFX "ERROR deleting R!\n");
		return;
	}
	DRStorage::instance().remove_connections_rx(it_rx);
	free(it_rx->rb.pdata);
	DRStorage::instance().rx.erase(it_rx);
	it_rx = DRStorage::instance().rx.end();
}

int dr_tx(tx_t tx, const void *msg, size_t size)
{
	int error = 0, succ_tx = 0;

	std::shared_lock<std::shared_mutex> lk(DRStorage::instance().mtx); // for multiple read
	for(std::list<rx_t>::iterator it = tx->receivers.begin(); it != tx->receivers.end(); it++)
	{
		if((*it)->enabled)
		{
			if((*it)->synchronous)
			{
				(*it)->rx_mutex.lock();
				(*it)->cb(msg, size);
				succ_tx++;
				(*it)->rx_mutex.unlock();
			}
			else
			{
				(*it)->rx_mutex.lock();
				if(rb_push((*it)->rb, msg, size))
				{
					error--;
				}
				else
				{
					succ_tx++;
				}
				(*it)->rx_mutex.unlock();
			}
		}
	}

	return error == 0 ? succ_tx : error;
}

size_t dr_is_avail(rx_t rx, const void **data)
{
	rx->rx_mutex.lock();
	size_t sz = rb_is_avail(rx->rb, data);
	rx->rx_mutex.unlock();
	return sz;
}

void dr_consume(rx_t rx)
{
	rx->rx_mutex.lock();
	rb_pop(rx->rb);
	rx->rx_mutex.unlock();
}
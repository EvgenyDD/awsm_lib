#ifndef __RB_VAR_H__
#define __RB_VAR_H__

// Ring buffer with variable element size implementation

#include <cstddef>
#include <stdint.h>

typedef struct
{
	size_t data_size;
	// void *data;
} fifo_element_t;

#define FIFO_ELMNT_SIZE(data_size) (sizeof(size_t) + data_size)

typedef struct
{
	size_t head_ptr;
	size_t tail_ptr;
	size_t skip_ptr;
	size_t size; // in bytes
	void *pdata;
} rb_t;

int rb_push(rb_t &rb, const void *data, size_t size);
size_t rb_is_avail(rb_t &rb, const void **data);
void rb_pop(rb_t &rb);

#endif // __RB_VAR_H__
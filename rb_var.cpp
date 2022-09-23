#include "rb_var.h"
#include <cstring>

static int rb_check_size(rb_t &rb, size_t size)
{
	if(rb.size <= FIFO_ELMNT_SIZE(size)) return 0;
	if(rb.head_ptr == rb.tail_ptr) return 1;

	bool from_start = false;
	if(rb.head_ptr > rb.size - FIFO_ELMNT_SIZE(size)) from_start = true;
	if(rb.head_ptr < rb.tail_ptr)
	{
		if(from_start || rb.tail_ptr - rb.head_ptr < FIFO_ELMNT_SIZE(size)) return 0;
	}
	else if(from_start && FIFO_ELMNT_SIZE(size) > rb.tail_ptr)
	{
		return 0;
	}
	return 1;
}

int rb_push(rb_t &rb, const void *data, size_t size)
{
	if(rb_check_size(rb, size))
	{
		fifo_element_t hdr;
		hdr.data_size = size;
		if(rb.head_ptr > rb.size - FIFO_ELMNT_SIZE(size))
		{
			rb.skip_ptr = rb.head_ptr;
			rb.head_ptr = 0;
		}

		memcpy((uint8_t *)(rb.pdata) + rb.head_ptr, &hdr, sizeof(hdr));
		memcpy((uint8_t *)(rb.pdata) + rb.head_ptr + sizeof(hdr), data, size);

		rb.head_ptr += FIFO_ELMNT_SIZE(size);
		return 0;
	}
	return 1;
}

size_t rb_is_avail(rb_t &rb, const void **data)
{
	if(rb.tail_ptr == rb.skip_ptr) rb.tail_ptr = 0;
	if(rb.head_ptr == rb.tail_ptr) return 0;

	fifo_element_t hdr;
	memcpy(&hdr, (uint8_t *)(rb.pdata) + rb.tail_ptr, sizeof(hdr));
	*data = (uint8_t *)(rb.pdata) + rb.tail_ptr + sizeof(hdr);
	return hdr.data_size;
}

void rb_pop(rb_t &rb)
{
	if(rb.tail_ptr > rb.size - sizeof(size_t)) rb.tail_ptr = 0;
	if(rb.tail_ptr == rb.skip_ptr) rb.tail_ptr = 0;
	if(rb.head_ptr == rb.tail_ptr) return;

	fifo_element_t hdr;
	memcpy(&hdr, (uint8_t *)(rb.pdata) + rb.tail_ptr, sizeof(hdr));
	rb.tail_ptr += FIFO_ELMNT_SIZE(hdr.data_size);
}

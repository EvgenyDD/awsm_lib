static inline void memcpy_byte(uint8_t *dst, uint32_t offset_dst_bits /* < 8 */, const uint8_t src, uint32_t offset_src_bits /* < 8 */, uint32_t size_bits)
{
	// if(size_bits >= 8 && offset_dst_bits == 0 && offset_src_bits == 0)
	// {
	// 	*dst = src;
	// 	return;
	// }
	if(size_bits >= 8) size_bits = 8;
	const uint8_t mask_dst = ((1 << size_bits) - 1) << offset_dst_bits;
	*dst = (*dst & ~mask_dst) | (((src >> offset_src_bits) << offset_dst_bits) & mask_dst);
}

static inline void memcpy_bits(uint8_t *dst, uint32_t off_dst_bits, const uint8_t *src, uint32_t off_src_bits, uint32_t sz_src_bits)
{
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

	const uint32_t off_dst_8 = off_dst_bits & 0x7, off_src_8 = off_src_bits & 0x7;
	const uint32_t off_dst_b = off_dst_bits >> 3, off_src_b = off_src_bits >> 3;
	const uint32_t sz_src_b = sz_src_bits >> 3;
	if(off_dst_8 == 0 && off_src_8 == 0) // aligned by byte
	{
		if(sz_src_bits >= 8) // at least one byte full
		{
			memcpy(&dst[off_dst_b], &src[off_src_b], sz_src_b);
		}
		if(sz_src_bits & 0x07) memcpy_byte(&dst[off_dst_b + sz_src_b], 0, src[off_src_b + sz_src_b], 0, sz_src_bits & 0x7); // last (not full) byte
	}
	else // no align
	{
		for(int32_t copied = 0;;)
		{
			if((int32_t)sz_src_bits == copied) break;

			const int32_t copy_dest = 8 - ((off_dst_8 + copied) & 0x7); // check how many bits of dest can be set
			const int32_t copy_remain = sz_src_bits - copied;
			int32_t copy_src = 8 - ((off_src_8 + copied) & 0x7); // check how many bits of src can copy to dest
			if(copy_src > copy_remain) copy_src = copy_remain;
			const int32_t copy = MIN(copy_src, copy_dest); // AND-ed value
			memcpy_byte(&dst[(off_dst_bits + copied) >> 3], (off_dst_8 + copied) & 0x7,
						src[(off_src_bits + copied) >> 3], (off_src_8 + copied) & 0x7,
						copy);
			copied += copy;
		}
	}
}

#define BIT_SET(val, bit) ((val) |= (1U << (bit)))
#define BIT_CLR(val, bit) ((val) &= (~((uint32_t)(1U << (bit)))))
#define BIT_WRITE(val, bit, state) ((state) != 0 ? BIT_SET(val, bit) : BIT_CLR(val, bit))

void memcpy_bits_unoptimal(uint8_t *dst, size_t off_dst, const uint8_t *src, size_t off_src, size_t size_bits)
{
	for(uint32_t i = 0; i < size_bits; i++)
	{
		uint32_t bit_src = off_src + i;
		uint32_t bit_dst = off_dst + i;
		BIT_WRITE(dst[bit_dst >> 3UL], bit_dst & 0x07UL, src[bit_src >> 3UL] & (1UL << (bit_src & 7UL)));
	}
}

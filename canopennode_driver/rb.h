#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#define RB_DECL_INST(NAME, SZ)          \
	uint8_t NAME[CO_CONFIG_TERM_TX_SZ]; \
	uint32_t NAME##_head, NAME##_tail

#define RB_INIT(NAME) \
	NAME##_head = NAME##_tail = 0;

#define RB_FREE_SZ(h, t, s) (t <= h ? s - 1 - h + t - 1 : t - h - 1)
#define RB_PL_SZ(h, t, s) (t <= h ? h - t : s - 1 - t + h)
#define RB_NOT_EMPTY(NAME) (NAME##_head != NAME##_tail)
#define RB_IS_EMPTY(NAME) (NAME##_head == NAME##_tail)

#define RB_APPEND(NAME, buf, len)                                                       \
	{                                                                                   \
		const uint32_t head = NAME##_head;                                              \
		uint32_t remain = len, free_size = RB_FREE_SZ(head, NAME##_tail, sizeof(NAME)); \
		if(remain > free_size) remain = free_size;                                      \
		if(remain)                                                                      \
		{                                                                               \
			int l = remain > sizeof(NAME) - head ? sizeof(NAME) - head : remain;        \
			int new_head = head + l;                                                    \
			memcpy(&NAME[head], buf, l);                                                \
			remain -= l;                                                                \
			if(remain)                                                                  \
			{                                                                           \
				memcpy(&NAME[0], &buf[l], remain);                                      \
				new_head = remain;                                                      \
			}                                                                           \
			NAME##_head = new_head;                                                     \
		}                                                                               \
	}

#define RB_CONSUME(NAME, buf, max_len, out_len)                              \
	{                                                                        \
		const uint32_t tail = NAME##_tail;                                   \
		uint32_t remain = RB_PL_SZ(NAME##_head, tail, sizeof(NAME));         \
		if(remain > max_len) remain = max_len;                               \
		out_len = remain;                                                    \
		int l = remain > sizeof(NAME) - tail ? sizeof(NAME) - tail : remain; \
		int new_tail = tail + l;                                             \
		memcpy(buf, &NAME[tail], l);                                         \
		remain -= l;                                                         \
		if(remain)                                                           \
		{                                                                    \
			memcpy(&buf[l], NAME, remain);                                   \
			new_tail = remain;                                               \
		}                                                                    \
		NAME##_tail = new_tail;                                              \
	}

#endif // __RING_BUFFER_H__
#pragma once

#include <cstdint>

#if defined(__MACH__) /* Apple OSX section */
// #include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define be16toh(x) OSSwapBigToHostInt16(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

#define Tdc5FreqMhz 300.
#define Tdc5FineMin 19
#define Tdc5FineMax 395

struct tdc5_parse_it {
	int i;
	int cur_chan;
	int is_first;
	uint64_t last_time;
	uint8_t block_flags;
	int finetime_len;
};
struct tdc5_header {
	uint32_t magic;
	uint8_t trig_type;
	uint32_t trig_num;
	uint32_t device_id;
	uint64_t trig_time;
	int finetime_len, tdc_type, freq;
};
struct tdc5_time {
	int channel;
	uint32_t coarse;
	uint32_t fine;
	int is_falling;
};

inline void tdc5_parse_header(tdc5_header *h, tdc5_parse_it *it, const char *buf, int /* n */)
{
	it->i = 0;
	h->magic = be16toh(*(uint16_t*)(buf + it->i)); it->i += 2;
	uint16_t tdcinf = be16toh(*(uint16_t*)(buf + it->i)); it->i += 2;
	h->finetime_len = tdcinf & 0x1f; tdcinf >>= 5;
	h->tdc_type = tdcinf & 0x1f; tdcinf >>= 5;
	h->freq = tdcinf * 10;
	uint32_t trig_type_num = be32toh(*(uint32_t*)(buf + it->i)); it->i += 4;
	h->trig_type = trig_type_num >> 28;
	h->trig_num = trig_type_num & 0x0fffffff;
	h->device_id = be32toh(*(uint32_t*)(buf + it->i)); it->i += 4;
	h->trig_time = be64toh(*(uint64_t*)(buf + it->i)); it->i += 8;
	it->i += 4; // currently unused
	it->cur_chan = 0;
	it->is_first = 1;
	it->finetime_len = h->finetime_len;
}
inline int tdc5_parse_next(tdc5_time *res, tdc5_parse_it *it, const char *buf, int n) // return codes: negative: error; zero: success; positive: continue
{
	int is_channel_empty;
	do {
		if (it->i == n) { // stream finished
			return it->is_first ? 0 : -1;
		}

		is_channel_empty = 0;
		if (it->is_first) { // this is the first word/time in the channel sequence
			if ((it->cur_chan & 7) == 0) { // ... the first channel in a channel block consisting of 8 channels
				if (it->i == n) return -1;
				it->block_flags = buf[it->i++]; // load 8 channel flags
			}
			// check if current channel is empty and shift flags s.t. flag of the next channel is the LSB
			is_channel_empty = !(it->block_flags & 1);
			it->block_flags >>= 1;

			it->last_time = 0;

			if (is_channel_empty) {
				it->is_first = 1;
				++it->cur_chan;
			}
		}
	} while (is_channel_empty);

	// decode times
	if (it->i + 3 > n) return -1;
	uint16_t msbs = be16toh(*(uint16_t*)(buf + it->i)); it->i += 2;
	int is_last = !!(msbs & 0x8000);
	int is_ext = !!(msbs & 0x4000);
	msbs = msbs & 0x3fff;
	int lsb_time_len = 1 + is_ext;
	if (it->i + lsb_time_len > n) return -1;
	uint16_t lsbs = is_ext ? be16toh(*(uint16_t*)(buf + it->i)) : (uint8_t)buf[it->i]; it->i += lsb_time_len;
	uint32_t time = (((uint32_t)msbs) << (lsb_time_len * 8)) | lsbs;
	res->is_falling = time & 1;
	time >>= 1;
	res->fine = time & ((1 << it->finetime_len) - 1);
	time >>= it->finetime_len;
	int32_t diff = time;
	it->last_time = it->is_first ? diff : it->last_time - diff;
	res->coarse = it->last_time;
	res->channel = it->cur_chan;

	it->is_first = is_last; // if this is the last time from the current channel, the next time will be the first one from the next channel...

	if (is_last) {
		++it->cur_chan;
	}

	return 1;
}

#pragma once

#include <cstdint>

#if defined(__MACH__) /* Apple OSX section */
// #include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

struct tdc5_parse_it {
	int i;
	int cur_chan;
	int is_first;
	int64_t last_time;
	uint8_t block_flags;
};
struct tdc5_header {
	uint32_t magic;
	uint8_t trig_type;
	uint32_t trig_num;
	uint32_t device_id;
	uint64_t trig_time;
};
struct tdc5_time {
	int channel;
	uint64_t coarse;
	uint32_t fine;
	int is_falling;
};

inline void tdc5_parse_header(tdc5_header *h, tdc5_parse_it *it, const char *buf, int n)
{
	it->i = 0;
	h->magic = be32toh(*(uint32_t*)(buf + it->i)); it->i += 4;
	uint32_t trig_type_num = be32toh(*(uint32_t*)(buf + it->i)); it->i += 4;
	h->trig_type = trig_type_num >> 24;
	h->trig_num = trig_type_num & 0x00ffffff;
	h->device_id = be32toh(*(uint32_t*)(buf + it->i)); it->i += 4;
	h->trig_time = be64toh(*(uint64_t*)(buf + it->i)); it->i += 8;
	it->cur_chan = 0;
	it->is_first = 1;
}
inline int tdc5_parse_next(tdc5_time *res, tdc5_parse_it *it, const char *buf, int n) // return codes: negative: error; zero: success; positive: continue
{
	do { // repeat until we have a non-empty channel
		if (it->i == n) { // stream finished
			return it->is_first ? 0 : -1;
		}

		if (it->is_first) { // this is the first word/time in the channel sequence
			if ((it->cur_chan & 7) == 0) { // ... the first channel in a channel block consisting of 8 channels
				if (it->i == n) return -1;
				it->block_flags = buf[it->i++]; // load 8 channel flags
			}
			// check if current channel is empty and shift flags s.t. flag of the next channel is the LSB
			int is_channel_empty = !(it->block_flags & 1);
			it->block_flags >>= 1;

			it->last_time = 0;

			if (is_channel_empty) {
				it->is_first = 1;
				++it->cur_chan;
				continue;
			}
		}

		// decode times
		uint32_t raw = be32toh(*(uint32_t*)(buf + it->i));
		int is_last = raw & 0x80000000;
		int is_ext = raw & 0x40000000;
		uint32_t time;
		raw = raw & 0x3fffffff;
		time = is_ext ? raw : raw >> 8;
		int time_len = 3 + is_ext;
		if (it->i + time_len > n) return -1;
		it->i += time_len;
		res->is_falling = time & 1;
		res->fine = (time >> 1) & 0x1ff;
		int32_t diff = time >> 10;
		it->last_time = it->is_first ? -diff : it->last_time + diff;
		res->coarse = it->last_time;
		res->channel = it->cur_chan;

		it->is_first = is_last; // if this is the last time from the current channel, the next time will be the first one from the next channel...

		if (is_last) {
			++it->cur_chan;
		}

		return 1;
	} while (1);
}


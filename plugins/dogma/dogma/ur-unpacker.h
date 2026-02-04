#pragma once

#include <stdint.h>

#include <stdint.h>

/* Detect big-endian hosts */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&                \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define HOST_BIG_ENDIAN 1
#endif

/* Big-endian: no-op */
#ifdef HOST_BIG_ENDIAN

static inline uint16_t be16conv(uint16_t x) { return x; }
static inline uint32_t be32conv(uint32_t x) { return x; }
static inline uint64_t be64conv(uint64_t x) { return x; }

#else /* Little-endian (Windows, macOS, Linux, etc.) */

static inline uint16_t be16conv(uint16_t x) {
  return (uint16_t)((x >> 8) | (x << 8));
}

static inline uint32_t be32conv(uint32_t x) {
  return ((x & 0x000000FFU) << 24) | ((x & 0x0000FF00U) << 8) |
         ((x & 0x00FF0000U) >> 8) | ((x & 0xFF000000U) >> 24);
}

static inline uint64_t be64conv(uint64_t x) {
  return ((uint64_t)be32conv((uint32_t)x) << 32) |
         be32conv((uint32_t)(x >> 32));
}
#endif

struct ur_context {
  int i;
  int cur_chan;
  int is_first;
  uint64_t last_time;
  uint8_t block_flags;
  int coarsetime_len, finetime_len, bitpadding_len, is_small, has_edge_type;
  int ft0_len, coarsetime_offset, edgetype_offset, remaining_ft_bytes;
};
struct ur_config {
  int coarsetime_len, finetime_len, tdc_type, freq, has_edge_type;
};
struct ur_header {
  // uint32_t magic;
  uint8_t trig_type;
  uint32_t trig_num;
  // uint32_t device_id;
  uint64_t trig_time;
  // int coarsetime_len, tdc_type, freq, finetime_len, has_edge_type;
};
struct ur_time {
  int channel;
  uint32_t coarse;
  uint8_t payload[128]; // 1024 bit finetimes are the maximum, because the
                        // header cannot indicate more
  uint32_t fine;
  int is_falling;
};

inline void ur_set_config(ur_context *it, const ur_config *cfg)
{
  it->coarsetime_len = cfg->coarsetime_len;
  it->finetime_len = cfg->finetime_len;
  it->has_edge_type = cfg->has_edge_type;
  // std::cout << it->coarsetime_len << " " << it->finetime_len << " " << it->has_edge_type << std::endl;

  int full_width =
      it->coarsetime_len + it->finetime_len + it->has_edge_type + 2;
  // std::cout << "FULL " << full_width << std::endl;
  it->bitpadding_len = (8 - (full_width % 8)) % 8;
  it->is_small = it->bitpadding_len + full_width <= 24;

  int ft0_len_max = 30 - it->coarsetime_len - it->bitpadding_len -
                    it->has_edge_type; // max amount of FT bits that fit into
                                       // the first 32 bit word

  it->ft0_len =
      std::min(ft0_len_max, it->finetime_len); // the actual amount of FT bits
                                               // in the first 32 bit word
  it->coarsetime_offset = it->ft0_len + it->has_edge_type + it->bitpadding_len;
  it->edgetype_offset = it->has_edge_type ? ft0_len_max : 31;
  it->remaining_ft_bytes =
      (it->finetime_len - it->ft0_len) /
      8; // FT bytes we need to load after the first 32 bit word
}

inline void ur_parse_header(ur_header *h, ur_context *it, const char *buf,
                            int n) {
  it->i = 0;
  h->trig_time = be64conv(*(uint64_t *)(buf + it->i));
  it->i += 8;
  uint32_t trig_type_num = be32conv(*(uint32_t *)(buf + it->i));
  it->i += 4;
  h->trig_type = trig_type_num >> 28;
  h->trig_num = trig_type_num & 0x0fffffff;
  it->cur_chan = 0;
  it->is_first = 1;
}
inline int ur_parse_next(
    ur_time *res, ur_context *it, const char *buf,
    int n) // return codes: negative: error; zero: success; positive: continue
{
  // skip empty channels
  int is_channel_empty;
  do {
    if (it->i == n) { // stream finished
      return it->is_first ? 0 : -1;
    }

    is_channel_empty = 0;
    if (it->is_first) { // this is the first word/time in the channel sequence
      if ((it->cur_chan & 7) == 0) { // ... the first channel in a channel block
                                     // consisting of 8 channels
        if (it->i == n)
          return -1;
        it->block_flags = buf[it->i++]; // load 8 channel flags
      }
      // check if current channel is empty and shift flags s.t. flag of the next
      // channel is the LSB
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
  if (it->i + 3 > n)
    return -1;
  uint16_t msbs = be16conv(*(uint16_t *)(buf + it->i));
  it->i += 2;
  int is_last = !!(msbs & 0x8000);
  int is_ext = !!(msbs & 0x4000);
  msbs = msbs & 0x3fff;
  int lsb_time_len = !it->is_small + is_ext;
  if (it->i + lsb_time_len > n)
    return -1;
  uint16_t lsbs = is_ext && !it->is_small ? be16conv(*(uint16_t *)(buf + it->i))
                  : !is_ext || it->is_small ? (uint8_t)buf[it->i]
                                            : 0;
  it->i += lsb_time_len;
  uint32_t time = (((uint32_t)msbs) << (lsb_time_len * 8)) | lsbs;

  // extract t0, diff and edgetype from "time"
  uint32_t t0 = time & ((1 << it->ft0_len) - 1);
  int32_t diff = time >> it->coarsetime_offset;
  res->is_falling = (time >> it->edgetype_offset) & 1;

  int off = (it->ft0_len + 7) / 8;
  res->fine = t0;
  for (int i = 0; i < off; ++i) {
    res->payload[off - 1 - i] = t0 & 255;
    t0 >>= 8;
  }
  for (int i = 0; i < it->remaining_ft_bytes; ++i) {
    res->payload[off + i] = (uint8_t)buf[it->i++];
  }

  it->last_time = it->is_first ? diff : it->last_time - diff;
  res->coarse = it->last_time;
  res->channel = it->cur_chan;

  it->is_first =
      is_last; // if this is the last time from the current channel, the next
               // time will be the first one from the next channel...

  if (is_last) {
    ++it->cur_chan;
  }

  return 1;
}

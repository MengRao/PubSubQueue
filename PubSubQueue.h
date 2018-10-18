/*
MIT License

Copyright (c) 2018 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

// PubSubQueue is a single publisher(source) multiple subscriber(receiver) message queue.
// Publisher is not affected(e.g. blocked) by or even aware of subscribers.
// Subscriber is a pure reader, if it's not reading fast enough and falls far behind the publisher it'll lose message.
// PubSubQueue can be zero initialized without calling constructor, which facilitates allocating in shared memory
// It is also "crash safe" which means crash of either publisher or subscribers will not corrupt the queue

// Bytes should be at least twice the size of the largest msg, otherwise alloc() could return nullptr
template<uint32_t Bytes>
class PubSubQueue
{
public:
    struct MsgHeader
    {
        // size of this msg, including header itself
        uint32_t size;
        // userdata is used by user, e.g. save the msg_type
        // we assume user msg is 8 types aligned so there should be 4 bytes padding anyway if we don't define userdata
        uint32_t userdata;
    };

    // allocate enough space(after the returned MsgHeader) to save the message to publish
    // return nullptr if size is too large
    MsgHeader* alloc(uint32_t size) {
        size += sizeof(MsgHeader);
        uint32_t blk_sz = toBlk(size);
        uint32_t padding_sz = BLK_CNT - (written_idx % BLK_CNT);
        bool rewind = blk_sz > padding_sz;
        uint32_t advance_sz = blk_sz + (rewind ? padding_sz : 0);
        if(advance_sz > BLK_CNT) { // msg size too large
            return nullptr;
        }
        writing_idx = written_idx + advance_sz;
        asm volatile("" : : "m"(writing_idx) :);
        if(rewind) {
            blk[written_idx % BLK_CNT].header.size = 0;
            asm volatile("" : : "m"(blk), "m"(written_idx) :);
            written_idx += padding_sz;
        }
        MsgHeader& header = blk[written_idx % BLK_CNT].header;
        header.size = size;
        return &header;
    }

    // publish the message allocated by alloc()
    // mark it as a key message so newly joined subscribers can start reading from this message
    void pub(bool key = false) {
        asm volatile("" : : "m"(blk), "m"(written_idx) :);
        uint32_t blk_sz = toBlk(blk[written_idx % BLK_CNT].header.size);
        if(key) last_key_idx = written_idx + 1; // +1 to allow last_key_idx to be zero initialized
        // it's OK that last_key_idx got changed and then process crashed/hang before written_idx can be updated
        // in this case sub(true) is effective sub(false), no big deal
        written_idx += blk_sz;
        asm volatile("" : : "m"(written_idx), "m"(last_key_idx) :);
    }

    // Newly joined subscriber calls sub() to get a message index to start reading at
    // set key to true if wanting to start from the last key message if any
    // or get the next message index the subscriber is going to write(can't read it immediately)
    uint64_t sub(bool key = false) {
        asm volatile("" : "=m"(last_key_idx), "=m"(writing_idx) : :); // force read memory
        if(key && last_key_idx > 0 && last_key_idx + BLK_CNT > writing_idx)
            return last_key_idx - 1;
        else
            return written_idx;
    }

    enum ReadRes
    {
        ReadOK,
        ReadAgain,
        ReadBuffTooShort,
        ReadNeedReSub
    };

    // subscriber provides a message index to read at and a buffer to copy message(including MsgHeader) to
    // return ReadOK: successfully read a message and set idx to the next message
    // return ReadAgain: no message to read now, user should try again later
    // return ReadBuffTooShort: bufsize is too small to hold the message and MsgHeader is copied in the buffer to check
    // return ReadNeedReSub: idx is obsolete and the message is lost, user has to subscribe again to get a new index
    ReadRes read(uint64_t& __restrict__ idx, void* __restrict__ buf, uint32_t bufsize) {
        asm volatile("" : "=m"(written_idx), "=m"(blk) : :); // force read memory
        if(idx >= written_idx) return ReadAgain;
        // size may have been overridden/corrupted by the publisher, we'll check it later...
        uint32_t size = blk[idx % BLK_CNT].header.size;
        uint32_t padding_sz = BLK_CNT - (idx % BLK_CNT);
        if(size == 0) { // rewind
            asm volatile("" : "=m"(writing_idx) : :);
            if(idx + BLK_CNT < writing_idx) return ReadNeedReSub;
            idx += padding_sz;
            if(idx >= written_idx) return ReadAgain;
            size = blk[idx % BLK_CNT].header.size;
            padding_sz = BLK_CNT;
        }
        uint32_t copy_size = std::min(std::min(bufsize, size), padding_sz * (uint32_t)sizeof(Block));
        memcpy(buf, &blk[idx % BLK_CNT], copy_size);
        asm volatile("" : "=m"(writing_idx), "=m"(blk) : :);
        if(idx + BLK_CNT < writing_idx) return ReadNeedReSub;
        if(copy_size < size) return ReadBuffTooShort;
        idx += toBlk(size);
        return ReadOK;
    }

private:
    static inline uint32_t toBlk(uint32_t bytes) {
        return (bytes + sizeof(Block) - 1) / sizeof(Block);
    }

    struct Block
    {
        alignas(64) MsgHeader header; // make it 64 bytes aligned, same as cache line size
    };

    static constexpr uint32_t BLK_CNT = Bytes / sizeof(Block);
    static_assert(BLK_CNT && !(BLK_CNT & (BLK_CNT - 1)), "BLK_CNT must be a power of 2");

    Block blk[BLK_CNT];

    alignas(64) uint64_t written_idx = 0;
    uint64_t last_key_idx = 0;
    uint64_t writing_idx = 0;
};


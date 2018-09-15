#pragma once
#include <string>
#include "shmmap.h"
#include "../PubSubQueue.h"

template<uint32_t N, uint16_t MSGTYPE>
struct Msg
{
    static constexpr uint16_t msg_type = MSGTYPE;
    uint64_t ts;
    int tid;
    int val[N];
};

typedef Msg<1, 1> Msg1;
typedef Msg<3, 2> Msg2;
typedef Msg<8, 3> Msg3;
typedef Msg<11, 4> Msg4;

typedef PubSubQueue<4096> MsgQ;

MsgQ* getMsgQ(const char* topic) {
    std::string path = "/";
    path += topic;
    return shmmap<MsgQ>(path.c_str());
}

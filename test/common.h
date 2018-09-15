#pragma once
#include "shmmap.h"
#include "../PubSubQueue.h"

template<uint32_t N, uint16_t MSGTYPE>
struct Msg
{
    static constexpr uint16_t msg_type = MSGTYPE;
    int64_t ts;
    int tid;
    int val[N];
};

typedef Msg<3, 1> Msg1;
typedef Msg<8, 2> Msg2;
typedef Msg<17, 3> Msg3;
typedef Msg<45, 4> Msg4;

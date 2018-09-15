#include <bits/stdc++.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "timestamp.h"
#include "cpupin.h"
#include "../PubSubQueue.h"

using namespace std;

template<uint32_t N, uint16_t MSGTYPE>
struct Msg
{
    static constexpr uint16_t msg_type = MSGTYPE;
    long val[N];
};

typedef Msg<3, 1> Msg1;
typedef Msg<8, 2> Msg2;
typedef Msg<17, 3> Msg3;
typedef Msg<45, 4> Msg4;

typedef PubSubQueue<4096> MsgQ;

MsgQ _q;
const int MaxNum = 10000000;
const int NumThr = 4;

template<class T>
void sendMsg(MsgQ* q, int& val) {
    MsgQ::MsgHeader* header = q->alloc(sizeof(T));
    assert(header != nullptr);
    header->userdata = T::msg_type;
    T* msg = (T*)(header + 1);
    for(auto& v : msg->val) v = val++;
    q->pub(false);
}

void pubthread(int tid) {

    MsgQ* q = &_q;
    int val = 1;
    while(val <= MaxNum) {
        int tp = rand() % 4 + 1;
        switch(tp) {
            case 1: sendMsg<Msg1>(q, val); break;
            case 2: sendMsg<Msg2>(q, val); break;
            case 3: sendMsg<Msg3>(q, val); break;
            case 4: sendMsg<Msg4>(q, val); break;
            default: assert(false);
        }
        std::this_thread::yield();
    }
}

template<class T>
void handleMsg(MsgQ::MsgHeader* header, int& val, int tid) {
    T* msg = (T*)(header + 1);
    for(auto v : msg->val) {
        if(v <= val) {
            cout << tid << ": bad: got: " << v << " latest: " << val << endl;
            exit(1);
        }
        if(v > val + 1) {
            cout << tid << ": missing data, expect: " << (val + 1) << " got: " << v << endl;
        }
        val = v;
    }
}

void subthread(int tid) {

    MsgQ* q = &_q;
    uint64_t idx = q->sub(true);
    char buf[1024];

    // int tid = (pid_t)::syscall(SYS_gettid);
    int i = 0;
    while(i < MaxNum) {
        auto res = q->read(idx, buf, sizeof(buf));
        if(res == MsgQ::ReadOK) {
            MsgQ::MsgHeader* header = (MsgQ::MsgHeader*)buf;
            auto msg_type = header->userdata;
            switch(msg_type) {
                case 1: handleMsg<Msg1>(header, i, tid); break;
                case 2: handleMsg<Msg2>(header, i, tid); break;
                case 3: handleMsg<Msg3>(header, i, tid); break;
                case 4: handleMsg<Msg4>(header, i, tid); break;
                default: assert(false);
            }
            continue;
        }
        assert(res != MsgQ::ReadBuffTooShort);
        if(res == MsgQ::ReadNeedReSub) {
            cout << tid << ": need resub" << endl;
            idx = q->sub(true);
        }
    }
}


int main() {
    vector<thread> threads;
    for(int i = 0; i < NumThr; i++) {
        threads.emplace_back(subthread, i);
    }
    std::this_thread::yield();
    threads.emplace_back(pubthread, NumThr);

    for(auto& thr : threads) {
        thr.join();
    }

    return 0;
}




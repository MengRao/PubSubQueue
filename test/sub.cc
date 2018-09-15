#include <bits/stdc++.h>
#include "timestamp.h"
#include "cpupin.h"
#include "common.h"

using namespace std;

template<class T>
void handleMsg(MsgQ::MsgHeader* header, const char* topic, uint64_t now) {
    T* msg = (T*)(header + 1);
    auto latency = now - msg->ts;
    cout << "topic: " << topic << " tid: " << msg->tid << " latency: " << (now - msg->ts) << " val:";
    for(auto v : msg->val) {
        cout << " " << v;
    }
    cout << endl;
}

int main(int argc, const char** argv) {
    // cpupin(1);
    if(argc < 2) {
        cout << "usage: " << argv[0] << " TOPIC1 [TOPIC2]..." << endl;
        exit(1);
    }
    vector<pair<MsgQ*, uint64_t>> qs(argc);
    char buf[1024];

    for(int i = 1; i < argc; i++) {
        if(!(qs[i].first = getMsgQ(argv[i]))) exit(1);
        qs[i].second = qs[i].first->sub(true);
    }

    while(true) {
        for(int i = 1; i < argc; i++) {
            auto q = qs[i].first;
            auto& idx = qs[i].second;
            auto res = q->read(idx, buf, sizeof(buf));
            if(res == MsgQ::ReadNeedReSub) {
                cout << "topic: " << argv[i] << " need resub" << endl;
                idx = q->sub(true);
                res = q->read(idx, buf, sizeof(buf));
            }
            if(res == MsgQ::ReadOK) {
                auto now = rdtsc();
                MsgQ::MsgHeader* header = (MsgQ::MsgHeader*)buf;
                auto msg_type = header->userdata;
                switch(msg_type) {
                    case 1: handleMsg<Msg1>(header, argv[i], now); break;
                    case 2: handleMsg<Msg2>(header, argv[i], now); break;
                    case 3: handleMsg<Msg3>(header, argv[i], now); break;
                    case 4: handleMsg<Msg4>(header, argv[i], now); break;
                    default: assert(false);
                }
                continue;
            }
            assert(res != MsgQ::ReadBuffTooShort);
            // res == ReadAgain
        }
    }


    return 0;
}


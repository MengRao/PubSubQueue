#include <bits/stdc++.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "timestamp.h"
#include "cpupin.h"
#include "common.h"

using namespace std;

MsgQ* q;
int tid;
const int MaxNum = 10000000;

template<class T>
void sendMsg(int& val) {
    MsgQ::MsgHeader* header = q->alloc(sizeof(T));
    assert(header != nullptr);
    header->userdata = T::msg_type;
    T* msg = (T*)(header + 1);
    for(auto& v : msg->val) v = val++;
    msg->tid = tid;
    msg->ts = rdtsc();
    q->pub(true);
}


int main(int argc, const char** argv) {
    // cpupin(2);
    if(argc < 2) {
        cout << "usage: " << argv[0] << " TOPIC" << endl;
        exit(1);
    }
    q = getMsgQ(argv[1]);
    if(!q) exit(1);
    tid = (pid_t)::syscall(SYS_gettid);

    int val = 1;
    while(val <= MaxNum) {
        int tp = rand() % 4 + 1;
        switch(tp) {
            case 1: sendMsg<Msg1>(val); break;
            case 2: sendMsg<Msg2>(val); break;
            case 3: sendMsg<Msg3>(val); break;
            case 4: sendMsg<Msg4>(val); break;
            default: assert(false);
        }
        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}





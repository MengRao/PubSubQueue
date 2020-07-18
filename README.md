## PubSubQueue
PubSubQueue is a pub-sub model message queue on localhost, similar to IP multicast on the network, and it's highly optimized for low latency.

It's a simple C++ class suitable for residing in shared memory for IPC, representing a topic on the host.

It supports single publisher(source) and multiple subscriber(receiver), where publisher is not affected(e.g. blocked) by or even aware of subscribers. Subscriber is a pure reader of the queue, if it's not reading fast enough and falls far behind the publisher it'll lose messages.

Msg type in PubSubQueue is blob and msg is guaranteed to be allocated in 8 bytes aligned address by queue, so a C/C++ struct can simply be used without performance penalty.

## Example
Usage for publisher:
```c++
#include "PubSubQueue.h"

using MsgQ = PubSubQueue<1024>;
MsgQ q;

// simply publish an int value of 123
MsgQ::MsgHeader* header = q.alloc(sizeof(int));
*(int*)(header + 1) = 123;
q.pub();
```

Usage for Subscriber:
```c++

char buf[100];
auto idx = q.sub();
while(true) {
  auto res = q.read(idx, buf, sizeof(buf));
  if(res == MsgQ::ReadOK) {
    MsgQ::MsgHeader* header = (MsgQ::MsgHeader*)buf;
    assert(header->size == sizeof(int));
    int msg = *(int*)(header + 1);
    // handle msg...
  }
  // handle other res...
}
```
For more examples, see [test](https://github.com/MengRao/IPC_PubSub/tree/master/test).

## Key msg
One useful feature PubSubQueue supports is "key msg"(it's like key frame in audio/video stream), where publisher can mark the msg it publishes as key msg, and the queue will save the index of the last key msg so any newly joined subscribers can start reading from this message.

Last value caching ([LVC](https://www.safaribooksonline.com/library/view/zeromq/9781449334437/ch05s03.html)) can be easily implemented by marking every msg to key msg.

## Performance
The number of subscriber will not affect the performance. 

The latency of transmitting a msg from publisher to subscriber is stably lower than **1 us** if subscriber is busy polling on the queue. If process scheduling optimization is applied(such as SCHED_FIFO and sched_setaffinity), the latency can be lower than **200 ns**.

## How to support multiple publisher?
There are two solutions:
 * Use multiple topics(thus multiple queues) and have subscribers subscribe for all these topics.
 * Add a [multiple producer single consumer queue](https://github.com/MengRao/MPSC_Queue) and have a dedicated worker consume and publish msgs.
 
## An Implementation for Fixed-Sized Msg
If you are using fixed sized msgs, then take a look at [SPMC_Queue](https://github.com/MengRao/SPMC_Queue) as it's more effient and easy to use.


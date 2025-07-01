# cpp-learning-disruptor

This repository is a learning-oriented **reimplementation** of the core components in [`Abc-Arbitrage/Disruptor-cpp`](https://github.com/Abc-Arbitrage/Disruptor-cpp/tree/master/Disruptor).  
The goal is to gain a deeper understanding of high-performance inter-thread messaging patterns by reproducing the key logic of the Disruptor.  
(some notes may be written in Chinese)

## What is the Disruptor?

The [LMAX Disruptor](https://lmax-exchange.github.io/disruptor/) is a high-performance, lock-free inter-thread messaging library originally developed for low-latency trading systems.  
It replaces traditional queues with a preallocated ring buffer and sequence-based coordination mechanism, minimizing contention and garbage collection.

## TODO


- [ ] Minimal RingBuffer implementation (`MiniRing`)
- [ ] Introduce busy-spin wait strategy
- [ ] Single-producer `Sequencer`
- [ ] Run `RingBuffer` + `Sequencer` + `BusySpin`
- [ ] Compare multiple wait strategies (`BusySpin` / `Yield` / `Sleep`)
- [ ] Multi-producer support example
- [ ] Performance benchmarks
- [ ] Unit testing


### lock-free queue

Generally speaking, a lock-free queue can be implemented using a ring buffer or a linked list. The key point of a lock-free queue is how to maintain coordination and consistency across threads using atomic operations.
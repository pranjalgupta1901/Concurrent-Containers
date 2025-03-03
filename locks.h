#ifndef LOCKS_H
#define LOCKS_H

#include <atomic>
#include <iostream>

using namespace std;

typedef enum {
    PTHREAD_BAR,
    SENSE_SEQ,
    SENSE_REL,
    NO_BAR
}barrier_type;

class barrier{
private:
    atomic<int> cnt;
    atomic<int> sense; 
    int N;
    pthread_barrier_t br;
    bool is_pthread_barrier;

public:    
    
    barrier(barrier_type bar, int num_threads) : cnt(0), sense(false), N(num_threads), is_pthread_barrier(false){
        switch(bar) {
            case PTHREAD_BAR:
                pthread_barrier_init(&br, NULL, num_threads);
                is_pthread_barrier = true;
                break;
            case SENSE_SEQ:
            case SENSE_REL:
                break;
            default:
                break;
        }
    }

    void sense_wait();
    void sense_wait_rel();
    void apply_barrier(barrier_type bar);

    ~barrier() {
        if (is_pthread_barrier) {
            pthread_barrier_destroy(&br);
        }
    }

};
    
enum locks_type {
    PTHREAD_LOCK = 0,
    TAS_LOCK,
    TTAS_LOCK,
    TICKET_LOCK,
    MCS_LOCK,
    PETERSON_LOCK,
    TASREL_LOCK,
    TTASREL_LOCK,
    TICKETREL_LOCK,
    MCSREL_LOCK,
    PETERSONREL_LOCK,
    NO_LOCK
};

class Node {
public:
    atomic<Node*> next;
    atomic<bool> wait;
    
    Node() : next(nullptr), wait(false) {}
};

class locks {
private:
    atomic<int> flag;
    atomic<int> next_num;
    atomic<int> now_serving;
    atomic<Node*> tail;
    atomic<bool> desires[2];
    atomic<int> turn;
    pthread_mutex_t pthread_lk;
    bool is_mcs_lock;
    bool is_pthread_lock;
    static thread_local Node* thread_node;

public:
    locks(locks_type type);
    
    Node* get_thread_node() {
        if (!thread_node) {
            thread_node = new Node();
        }
        return thread_node;
    }
    
    bool test_and_set(memory_order MEM);
    void tas_lock(memory_order MEM);
    void tas_unlock(memory_order MEM);
    void ttas_lock(memory_order MEM1, memory_order MEM2);
    void ttas_unlock(memory_order MEM);
    void ticket_lock(memory_order MEM1, memory_order MEM2);
    void ticket_unlock(memory_order MEM);
    void MCS_lock(Node* myNode);
    void MCS_unlock(Node* myNode);
    void MCSREL_lock(Node *myNode);
    void MCSREL_unlock(Node *myNode);    
    void Peterson_lock(memory_order MEM, int tid);
    void Peterson_unlock(memory_order MEM, int tid);
    void apply_lock(locks_type type, long tid);
    void lock_unlock(locks_type type, long tid);
    
    ~locks(){
        if(is_pthread_lock)
            pthread_mutex_destroy(&pthread_lk);
        else if(is_mcs_lock)
            delete thread_node;
    }
};

#endif

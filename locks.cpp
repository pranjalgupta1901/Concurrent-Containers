#include "locks.h"

void barrier::sense_wait(void) {
        thread_local int my_sense = false;
        if(my_sense == 0)
            my_sense = 1;
        else
            my_sense = 0;
    
        int count = cnt.fetch_add(1, memory_order_seq_cst);
        
        if (count == (N - 1)) {  
            cnt.store(0, memory_order_release);
            sense.store(my_sense, memory_order_seq_cst);
        } else {
            while (sense.load(memory_order_seq_cst) != my_sense) {
            }
        }
}
    
void barrier::sense_wait_rel(void) {
        thread_local int my_sense = false;
        if(my_sense == 0)
            my_sense = 1;
        else
            my_sense = 0;
    
        int count = cnt.fetch_add(1, memory_order_acq_rel);
        
        if (count == (N - 1)) {  
            cnt.store(0, memory_order_release);
            sense.store(my_sense, memory_order_release);
        } else {
            while (sense.load(memory_order_acquire) != my_sense) {
            }
        }
}

void barrier::apply_barrier(barrier_type bar_type){
    switch(bar_type){
        case PTHREAD_BAR:
            pthread_barrier_wait(&br);
            break;
        
        case SENSE_SEQ:
            sense_wait();
            break;
        
        case SENSE_REL:
            sense_wait_rel();
            break;
        
        default:
            break;
            
    }
}


locks::locks(locks_type type) : flag(0), next_num(0), now_serving(0), tail(nullptr), is_mcs_lock(false), is_pthread_lock(false) {
    switch (type) {
        case PTHREAD_LOCK:
            pthread_mutex_init(&pthread_lk, NULL);
            is_pthread_lock = true;
            break;
        case TAS_LOCK:
        case TASREL_LOCK:    
            flag.store(false, memory_order_relaxed);
            break;
        case TTAS_LOCK:
        case TTASREL_LOCK:
            flag.store(false, memory_order_relaxed);
            break;
        case TICKET_LOCK:
        case TICKETREL_LOCK:
            next_num.store(0, memory_order_relaxed);
            now_serving.store(0, memory_order_relaxed);
            break;
        case PETERSON_LOCK:
            desires[0].store(false, memory_order_relaxed);
            desires[1].store(false, memory_order_relaxed);
            turn.store(0, memory_order_relaxed);
            break;
        case MCS_LOCK:
        case MCSREL_LOCK:
            is_mcs_lock = true;
            tail.store(nullptr, memory_order_release);
            break;
        default:
            cout << "Specify the lock correctly" << endl;
            break;
    }
}

thread_local Node* locks::thread_node = nullptr;

bool locks::test_and_set(memory_order MEM) {
    int expected = 0;
    return flag.compare_exchange_strong(expected, true, MEM);
}

void locks::tas_lock(memory_order MEM) {
    while (test_and_set(MEM) == false);
}

void locks::tas_unlock(memory_order MEM) {
    flag.store(false, MEM);
}

void locks::ttas_lock(memory_order MEM1, memory_order MEM2) {
    while (flag.load(MEM1) == true || test_and_set(MEM2) == false);
}

void locks::ttas_unlock(memory_order MEM) {
    flag.store(false, MEM);
}

void locks::ticket_lock(memory_order MEM1, memory_order MEM2) {
    int my_num = next_num.fetch_add(1, MEM1);
    while (now_serving.load(MEM2) != my_num);
}

void locks::ticket_unlock(memory_order MEM) {
    now_serving.fetch_add(1, MEM);
}

void locks::MCS_lock(Node* myNode) {
    Node* oldTail = tail.load(memory_order_seq_cst);
    myNode->next.store(nullptr, memory_order_relaxed);
    while (!tail.compare_exchange_strong(oldTail, myNode, memory_order_seq_cst)) {
        oldTail = tail.load(memory_order_seq_cst);
    }
    if (oldTail != nullptr) {
        myNode->wait.store(true, memory_order_relaxed);
        oldTail->next.store(myNode, memory_order_seq_cst);
        while (myNode->wait.load(memory_order_seq_cst));
    }
}

void locks::MCS_unlock(Node* myNode) {
    Node* m = myNode;
    if (tail.compare_exchange_strong(m, nullptr, memory_order_seq_cst)) {
    } 
    else {
        while (myNode->next.load(memory_order_seq_cst) == nullptr);
        Node* nextNode = myNode->next.load(memory_order_seq_cst);
        nextNode->wait.store(false, memory_order_seq_cst);
    }
}

void locks::MCSREL_lock(Node *myNode){
    Node* oldTail = tail.load(memory_order_acquire);
    myNode->next.store(nullptr, memory_order_relaxed);
    while (!tail.compare_exchange_strong(oldTail, myNode, memory_order_release)) {
        oldTail = tail.load(memory_order_acquire);
    }
      
    if (oldTail != nullptr) {
        myNode->wait.store(true, memory_order_relaxed);      
        oldTail->next.store(myNode, memory_order_release); 
        while (myNode->wait.load(memory_order_acquire));
    }
}

void locks::MCSREL_unlock(Node *myNode){
        Node* m = myNode;
        if (tail.compare_exchange_strong(m, nullptr, memory_order_release)){
        }
        else{
        while (myNode->next.load(memory_order_acquire) == nullptr);        
        Node* nextNode = myNode->next.load(memory_order_relaxed);
        nextNode->wait.store(false, memory_order_release);
        }
}


void locks::Peterson_lock(memory_order MEM1, int tid) {
    desires[tid].store(true, MEM1);
    turn.store(!tid, MEM1);
    while (desires[!tid].load(MEM1) && turn.load(MEM1) == !tid);
}

void locks::Peterson_unlock(memory_order MEM1, int tid) {
    desires[tid].store(false, MEM1);
}

void locks::apply_lock(locks_type type, long tid) {
    switch (type) {
        case TAS_LOCK:
            tas_lock(memory_order_seq_cst);
            break;
        case TTAS_LOCK:
            ttas_lock(memory_order_seq_cst, memory_order_seq_cst);
            break;
        case TICKET_LOCK:
            ticket_lock(memory_order_seq_cst, memory_order_seq_cst);
            break;
        case MCS_LOCK:
            MCS_lock(get_thread_node());
            break;
        case PTHREAD_LOCK:
            pthread_mutex_lock(&pthread_lk);
            break;
        case PETERSON_LOCK:
            Peterson_lock(memory_order_seq_cst, tid);
            break;
        case TASREL_LOCK:
            tas_lock(memory_order_acquire);
            break;
        case TTASREL_LOCK:
            ttas_lock(memory_order_relaxed, memory_order_acquire);
            break;
        case TICKETREL_LOCK:
            ticket_lock(memory_order_acquire, memory_order_acquire);
            break;
        case MCSREL_LOCK:
            MCSREL_lock(get_thread_node());
            break;
        case PETERSONREL_LOCK:
            Peterson_lock(memory_order_acquire, tid);
            break;
        default:
            break;
    }
}

void locks::lock_unlock(locks_type type, long tid) {
    switch (type) {
        case TAS_LOCK:
            tas_unlock(memory_order_seq_cst);
            break;
        case TTAS_LOCK:
            ttas_unlock(memory_order_seq_cst);
            break;
        case TICKET_LOCK:
            ticket_unlock(memory_order_seq_cst);
            break;
        case MCS_LOCK:
            MCS_unlock(get_thread_node());
            break;
        case PTHREAD_LOCK:
            pthread_mutex_unlock(&pthread_lk);
            break;
        case PETERSON_LOCK:
            Peterson_unlock(memory_order_seq_cst, tid);
            break;
        case TASREL_LOCK:
            tas_unlock(memory_order_release);
            break;
        case TTASREL_LOCK:
            ttas_unlock(memory_order_release);
            break;
        case TICKETREL_LOCK:
            ticket_unlock(memory_order_release);
            break;
        case MCSREL_LOCK:
            MCSREL_unlock(get_thread_node());
            break;
        case PETERSONREL_LOCK:
            Peterson_unlock(memory_order_release, tid);
            break;
        default:
            break;
    }
}

#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include "common_header_file.h"
#include <cassert>
#include <fstream>

using namespace std;

static void write_back_to_file(string out_file, vector<atomic<int>>& arr, int size);

class msqueue{
public:
    class node{
        public:
        node(int v):val(v){}
        int val; atomic<node*> next = nullptr;
    };
private:    
    atomic<node*> head, tail;
public:
    msqueue();
    void enqueue(int val);
    int dequeue();
};

msqueue::msqueue(){
    node *dummy = new node(-1);
    head.store(dummy);
    tail.store(dummy);
}
void msqueue::enqueue(int val){
    node *tail_node, *end, *new_node;
    new_node = new node(val);
    node* expected = nullptr;
    while(true){
        tail_node = tail.load(memory_order_acquire); 
        end = tail_node->next.load(memory_order_acquire);
        if(tail_node == tail.load(memory_order_acquire)){
            if(end == NULL && tail_node->next.compare_exchange_strong(expected, new_node, memory_order_acq_rel))
                break;
            
            else if(end!=NULL)
                tail.compare_exchange_strong(tail_node,end, memory_order_acq_rel);
        }
    }
    tail.compare_exchange_strong(tail_node,new_node, memory_order_acq_rel);
}

int msqueue::dequeue(){
    node *tail_node, *dummy_node, *new_node;
    while(true){
        dummy_node = head.load(memory_order_acquire); 
        tail_node = tail.load(memory_order_acquire); 
        new_node = dummy_node->next.load(memory_order_acquire);
        if(dummy_node == head.load(memory_order_acquire)){
            if(dummy_node == tail_node){
                if(new_node == NULL)
                    return -1;
                
                else
                    tail.compare_exchange_strong(tail_node, new_node, memory_order_acq_rel);
            }
        else{
            int ret = new_node->val;
            if(head.compare_exchange_strong(dummy_node, new_node, memory_order_acq_rel))
                return ret;
            }
        }
    }
}
   
    
// This test is designed in such a way that multiple threads will execute the queue methods and
// it is possible that for example during enqueue the sequence can be mistmatched with the values enqueued as in real time
// threads can be executed in interleaved manner. So, to test, summing up the values enqueued should be equal to the sum of the values dequeued.
    
int msqueue_test_advanced(int num_threads, vector<int>&arr){
    int num_thread_for_each_ops = (num_threads / 2);
    
    // Tracking mechanisms
    vector<atomic<int>> test_enqueue_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_dequeue_arr(num_thread_for_each_ops * arr.size());
    
    // Atomic counters for tracking
    atomic<int> enqueue_counter(0);
    atomic<int> dequeue_counter(0);
    atomic<int> test_counter(0);
    
    // Total sum tracking
    atomic<int> sum(0);
    int sum_actual = 0;
    
    msqueue myqueue;
    vector<thread> local_threads;

    // enqueue threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                myqueue.enqueue(arr[j]);
                
                // Track enqueue operation
                int enqueue_index = enqueue_counter.fetch_add(1, memory_order_seq_cst);
                assert(enqueue_index < test_enqueue_arr.size());
                
                // Store enqueued value
                int test_index = test_counter.fetch_add(1, memory_order_seq_cst);
                test_enqueue_arr[test_index].store(arr[j], memory_order_seq_cst);
            }
        }));
    }

    // dequeue threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                int value = myqueue.dequeue();
                
                // Only process valid dequeue values
                if (value != -1) {
                    // Track dequeue operation
                    int dequeue_index = dequeue_counter.fetch_add(1, memory_order_seq_cst);
                    assert(dequeue_index < test_dequeue_arr.size());
                    
                    // Store dequeueped value
                    int index = i * arr.size() + j;
                    test_dequeue_arr[index].store(value, memory_order_seq_cst);
                    
                    // Update sum
                    sum.fetch_add(value, memory_order_seq_cst);
                }
            }
        }));
    }

    // Wait for all threads
    for (auto& t : local_threads) {
        t.join();
    }

    // Calculate expected sum
    for (int j = 0; j < arr.size(); j++) {
        sum_actual += arr[j] * num_thread_for_each_ops;
    }

    // Verification checks
    
    // 1. Check if final Queue is empty
    if (myqueue.dequeue() != -1) {
        cout << "Queue should be empty at this point" << endl;
        return -1;
    }
    
    // 2. Verify enqueue and dequeue counts
    int expected_total_ops = num_thread_for_each_ops * arr.size();
    int actual_enqueue_count = enqueue_counter.load(memory_order_seq_cst);
    int actual_dequeue_count = dequeue_counter.load(memory_order_seq_cst);
    
    cout << "Expected total operations: " << expected_total_ops << endl;
    cout << "Actual enqueue count: " << actual_enqueue_count << endl;
    cout << "Actual dequeue count: " << actual_dequeue_count << endl;
    
    if (actual_enqueue_count != expected_total_ops) {
        cout << "Enqueue count mismatch" << endl;
        return -1;
    }
    
    if (actual_dequeue_count != expected_total_ops) {
        cout << "Dequeue count mismatch" << endl;
        return -1;
    }
    
    // 3. Verify sum
    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }
    
    
    cout << "Test passed successfully" << endl;
    return 0;
}

static void write_back_to_file(string out_file, vector<atomic<int>>& arr, int size) {
    ofstream output_file_var(out_file);

    if (!output_file_var.is_open()) {
        cerr << "Error: Could not create or open the file " << out_file << endl;
        return;
    }

    for (int i = 0; i < size; i++) {
        output_file_var << arr[i].load() << "\n";
    }

    output_file_var.close();
}


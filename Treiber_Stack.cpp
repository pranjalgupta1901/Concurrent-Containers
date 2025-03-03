#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <cassert>
#include "common_header_file.h"

using namespace std;

static void write_back_to_file(string out_file, vector<atomic<int>>& arr);

class tstack {
    class node {
    public:
        atomic<int> val;
        atomic<node*> down;
        node(int v) {
            val.store(v);
            down.store(nullptr);
        }
    };
private:
    atomic<node*> top;
public:
    tstack() {
        top.store(nullptr);
    }
    void push(int val);
    int pop();
};

void tstack::push(int val) {
    node* n = new node(val);
    node* t;
    do {
        t = top.load(memory_order_acquire);
        n->down.store(t, memory_order_release);
    } while (!top.compare_exchange_strong(t, n, memory_order_acq_rel)); // linearization point
}

int tstack::pop() {
    node* t;
    node* n;
    int v;
    do {
        t = top.load(memory_order_acquire);
        if (t == nullptr)
            return -1; // Stack is empty
        n = t->down.load(memory_order_acquire);
        v = t->val.load(memory_order_acquire);
    } while (!top.compare_exchange_strong(t, n, memory_order_acq_rel)); // linearization point

    delete t; // Free memory after pop
    return v;
}

int tstack_test_basic(void) {
    tstack mystack;

    for (int i = 0; i < 5; i++)
        mystack.push(i);

    for (int i = 4; i >= 0; i--) {
        if (mystack.pop() != i) {
            cout << "The stack is not behaving properly and there is some issue" << endl;
            return -1;
        }
    }

    if (mystack.pop() != -1) {
        cout << "should have returned -1 as stack should be empty at this point" << endl;
        return -1;
    }

    cout << "stack is working properly" << endl;
    return 0;
}

int tstack_test_advanced(int num_threads, vector<int>& arr) {
    int num_thread_for_each_ops = (num_threads / 2);
    
    // Tracking mechanisms
    vector<atomic<int>> test_push_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_pop_arr(num_thread_for_each_ops * arr.size());
    
    // Atomic counters for tracking
    atomic<int> push_counter(0);
    atomic<int> pop_counter(0);
    atomic<int> test_counter(0);
    
    // Total sum tracking
    atomic<int> sum(0);
    int sum_actual = 0;
    
    tstack mystack;
    vector<thread> local_threads;

    // Push threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                mystack.push(arr[j]);
                
                // Track push operation
                int push_index = push_counter.fetch_add(1, memory_order_seq_cst);
                assert(push_index < test_push_arr.size());
                
                // Store pushed value
                int test_index = test_counter.fetch_add(1, memory_order_seq_cst);
                test_push_arr[test_index].store(arr[j], memory_order_seq_cst);
            }
        }));
    }

    // Pop threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                int value = mystack.pop();
                
                // Only process valid pop values
                if (value != -1) {
                    // Track pop operation
                    int pop_index = pop_counter.fetch_add(1, memory_order_seq_cst);
                    assert(pop_index < test_pop_arr.size());
                    
                    // Store popped value
                    int index = i * arr.size() + j;
                    test_pop_arr[index].store(value, memory_order_seq_cst);
                    
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
    
    // 1. Check if final stack is empty
    if (mystack.pop() != -1) {
        cout << "Stack should be empty at this point" << endl;
        return -1;
    }
    
    // 2. Verify push and pop counts
    int expected_total_ops = num_thread_for_each_ops * arr.size();
    int actual_push_count = push_counter.load(memory_order_seq_cst);
    int actual_pop_count = pop_counter.load(memory_order_seq_cst);
    
    cout << "Expected total operations: " << expected_total_ops << endl;
    cout << "Actual push count: " << actual_push_count << endl;
    cout << "Actual pop count: " << actual_pop_count << endl;
    
    if (actual_push_count != expected_total_ops) {
        cout << "Push count mismatch" << endl;
        return -1;
    }
    
    if (actual_pop_count != expected_total_ops) {
        cout << "Pop count mismatch" << endl;
        return -1;
    }
    
    // 3. Verify sum
    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }
    
    
    write_back_to_file("Treiber_Push.txt", test_push_arr);
    write_back_to_file("Treiber_Pop.txt", test_pop_arr);
    
    cout << "Test passed successfully" << endl;
    return 0;
}

static void write_back_to_file(string out_file, vector<atomic<int>>& arr) {
    ofstream output_file_var(out_file);

    if (!output_file_var.is_open()) {
        cerr << "Error: Could not create or open the file " << out_file << endl;
        return;
    }

    for (int i = 0; i < arr.size(); i++) {
        output_file_var << arr[i].load() << "\n";
    }

    output_file_var.close();
}

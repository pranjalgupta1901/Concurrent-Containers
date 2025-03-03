#include <vector>
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <thread>
#include <cassert>
#include "common_header_file.h"

using namespace std;

// Global lock for file operations
static mutex file_write_lock;

// Write back to file safely
static void write_back_to_file(string out_file, vector<atomic<int>>& arr, int size) {
    lock_guard<mutex> lock(file_write_lock);  // Prevent concurrent writes to the file
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

mutex sgl_lock;  // Lock for stack/queue operations

class sgl {
private:
    vector<int> arr;

public:
    void sgl_push_stack(int val);
    int sgl_pop_stack();

    void sgl_enqueue_queue(int val);
    int sgl_dequeue_queue();
};

// Stack operations
void sgl::sgl_push_stack(int val) {
    lock_guard<mutex> lock(sgl_lock);
    arr.push_back(val);
}

int sgl::sgl_pop_stack() {
    lock_guard<mutex> lock(sgl_lock);

    if (arr.empty()) return -1;

    int val = arr.back();
    arr.pop_back();
    return val;
}

// Queue operations
void sgl::sgl_enqueue_queue(int val) {
    lock_guard<mutex> lock(sgl_lock);
    arr.push_back(val);
}

int sgl::sgl_dequeue_queue() {
    lock_guard<mutex> lock(sgl_lock);

    if (arr.empty()) return -1;

    int val = arr.front();
    arr.erase(arr.begin());
    return val;
}

// Basic stack test
int sgl_stack_test_basic(void) {
    sgl mystack;

    for (int i = 0; i < 5; i++) mystack.sgl_push_stack(i);

    for (int i = 4; i >= 0; i--) {
        if (mystack.sgl_pop_stack() != i) {
            cout << "The stack is not behaving properly and there is some issue" << endl;
            return -1;
        }
    }

    if (mystack.sgl_pop_stack() != -1) {
        cout << "should have returned -1 as stack should be empty at this point" << endl;
        return -1;
    }

    cout << "SGL stack is working properly" << endl;
    return 0;
}

// Advanced stack test with multiple threads
int sgl_stack_test_advanced(int num_threads, vector<int>& arr) {
    int num_thread_for_each_ops = num_threads / 2;

    vector<atomic<int>> test_push_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_dequeue_arr(num_thread_for_each_ops * arr.size());

    atomic<int> push_counter(0);
    atomic<int> pop_counter(0);
    atomic<int> test_counter(0);

    atomic<int> sum(0);
    int sum_actual = 0;

    sgl mystack;
    vector<thread> local_threads;

    // Push threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                mystack.sgl_push_stack(arr[j]);

                int push_index = push_counter.fetch_add(1, memory_order_seq_cst);
                assert(push_index < test_push_arr.size());

                int test_index = test_counter.fetch_add(1, memory_order_seq_cst);
                test_push_arr[test_index].store(arr[j], memory_order_seq_cst);
            }
        }));
    }

    // Pop threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                int value = mystack.sgl_pop_stack();
                
                if (value != -1) {
                    int pop_index = pop_counter.fetch_add(1, memory_order_seq_cst);
                    assert(pop_index < test_dequeue_arr.size());

                    int index = i * arr.size() + j;
                    test_dequeue_arr[index].store(value, memory_order_seq_cst);

                    sum.fetch_add(value, memory_order_seq_cst);
                }
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& t : local_threads) {
        t.join();
    }

    // Calculate expected sum
    for (int j = 0; j < arr.size(); j++) {
        sum_actual += arr[j] * num_thread_for_each_ops;
    }

    // Verification checks
    if (mystack.sgl_pop_stack() != -1) {
        cout << "Stack should be empty at this point" << endl;
        return -1;
    }

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

    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }

    cout << "Test passed successfully" << endl;
    return 0;
}

// Basic queue test
int sgl_queue_test_basic(void) {
    sgl myqueue;

    for (int i = 0; i < 5; i++) myqueue.sgl_enqueue_queue(i);

    for (int i = 0; i < 5; i++) {
        if (myqueue.sgl_dequeue_queue() != i) {
            cout << "The queue is not behaving properly and there is some issue" << endl;
            return -1;
        }
    }

    if (myqueue.sgl_dequeue_queue() != -1) {
        cout << "should have returned -1 as queue should be empty at this point" << endl;
        return -1;
    }

    cout << "SGL queue is working properly" << endl;
    return 0;
}

// Advanced queue test with multiple threads
int sgl_queue_test_advanced(int num_threads, vector<int>& arr) {
    int num_thread_for_each_ops = num_threads / 2;

    vector<atomic<int>> test_enqueue_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_dequeue_arr(num_thread_for_each_ops * arr.size());

    atomic<int> enqueue_counter(0);
    atomic<int> dequeue_counter(0);
    atomic<int> test_counter(0);

    atomic<int> sum(0);
    int sum_actual = 0;

    sgl myqueue;
    vector<thread> local_threads;

    // Enqueue threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                myqueue.sgl_enqueue_queue(arr[j]);

                int enqueue_index = enqueue_counter.fetch_add(1, memory_order_seq_cst);
                assert(enqueue_index < test_enqueue_arr.size());

                int test_index = test_counter.fetch_add(1, memory_order_seq_cst);
                test_enqueue_arr[test_index].store(arr[j], memory_order_seq_cst);
            }
        }));
    }

    // Dequeue threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                int value = myqueue.sgl_dequeue_queue();

                if (value != -1) {
                    int dequeue_index = dequeue_counter.fetch_add(1, memory_order_seq_cst);
                    assert(dequeue_index < test_dequeue_arr.size());

                    int index = i * arr.size() + j;
                    test_dequeue_arr[index].store(value, memory_order_seq_cst);

                    sum.fetch_add(value, memory_order_seq_cst);
                }
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& t : local_threads) {
        t.join();
    }

    // Calculate expected sum
    for (int j = 0; j < arr.size(); j++) {
        sum_actual += arr[j] * num_thread_for_each_ops;
    }

    // Verification checks
    if (myqueue.sgl_dequeue_queue() != -1) {
        cout << "Queue should be empty at this point" << endl;
        return -1;
    }

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

    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }

    cout << "Test passed successfully" << endl;
    return 0;
}


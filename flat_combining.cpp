#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <cassert>

using namespace std;

static void write_back_to_file(const string& out_file, vector<atomic<int>>& arr, int size) ;
enum OperationType {
    PUSH,
    POP,
    ENQUEUE,
    DEQUEUE
};

class FC {
private:
    vector<int> data; // For stack operations
    struct FlatCombinedStructure {
        OperationType type;
        atomic<bool> completed{false};
        atomic<int> result{0};
        atomic<int> value{0};
    };

    atomic<int> index{0};
    vector<FlatCombinedStructure> ops;

public:
    condition_variable cv;
    mutex fc_mutex;

    FC() : ops(100000) {}

    void flat_combine();
    void push_stack(int val);
    int pop_stack();
    void enqueue_queue(int val);
    int dequeue_queue();
};

void FC::flat_combine() {
    for (int i = 0; i < index.load(memory_order_acquire); ++i) {
        auto& op = ops[i];

        if (op.completed.load(memory_order_acquire)) continue;

        switch (op.type) {
            case PUSH:
                data.push_back(op.value.load(memory_order_acquire));
                op.result.store(1, memory_order_release); // Indicate success
                break;

            case POP:
                if (!data.empty()) {
                    int popped_value = data.back();
                    data.pop_back();
                    op.result.store(popped_value, memory_order_release);
                } else {
                    op.result.store(-1, memory_order_release); // Stack empty
                }
                break;
                
            case ENQUEUE:
                data.push_back(op.value.load(memory_order_acquire));
                op.result.store(1, memory_order_release); // Indicate success
                break;

            case DEQUEUE:
                if (!data.empty()) {
                    int popped_value = data.front();
                    data.erase(data.begin());
                    op.result.store(popped_value, memory_order_release);
                } else {
                    op.result.store(-1, memory_order_release); // Stack empty
                }
                break;                
        }

        op.completed.store(true, memory_order_release);
    }
    cv.notify_all();
}

void FC::push_stack(int val) {
    int current_index = index.fetch_add(1, memory_order_seq_cst);
    auto& new_op = ops[current_index];

    new_op.type = PUSH;
    new_op.value.store(val, memory_order_relaxed);
    new_op.completed.store(false, memory_order_relaxed);

    unique_lock<mutex> lock(fc_mutex);
    flat_combine();
    cv.wait(lock, [&new_op] { return new_op.completed.load(memory_order_acquire); });
}

int FC::pop_stack() {
    int current_index = index.fetch_add(1, memory_order_seq_cst);
    auto& new_op = ops[current_index];

    new_op.type = POP;
    new_op.completed.store(false, memory_order_relaxed);

    unique_lock<mutex> lock(fc_mutex);
    flat_combine();
    cv.wait(lock, [&new_op] { return new_op.completed.load(memory_order_acquire); });

    return new_op.result.load(memory_order_acquire);
}

void FC::enqueue_queue(int val) {
    int current_index = index.fetch_add(1, memory_order_seq_cst);
    auto& new_op = ops[current_index];

    new_op.type = ENQUEUE;
    new_op.value.store(val, memory_order_relaxed);
    new_op.completed.store(false, memory_order_relaxed);

    unique_lock<mutex> lock(fc_mutex);
    flat_combine();
    cv.wait(lock, [&new_op] { return new_op.completed.load(memory_order_acquire); });
}

int FC::dequeue_queue() {
    int current_index = index.fetch_add(1, memory_order_seq_cst);
    auto& new_op = ops[current_index];

    new_op.type = DEQUEUE;
    new_op.completed.store(false, memory_order_relaxed);

    unique_lock<mutex> lock(fc_mutex);
    flat_combine();
    cv.wait(lock, [&new_op] { return new_op.completed.load(memory_order_acquire); });

    return new_op.result.load(memory_order_acquire);
}

static void write_back_to_file(const string& out_file, vector<atomic<int>>& arr, int size) {
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

int fc_stack_test_advanced(int num_threads, vector<int>& arr) {
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
    
    FC mystack;
    vector<thread> local_threads;

    // Push threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                mystack.push_stack(arr[j]);
                
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
                int value = mystack.pop_stack();
                
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
    if (mystack.pop_stack() != -1) {
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
    
    cout << "Test passed successfully" << endl;
    return 0;
}


int fc_queue_test_advanced(int num_threads, vector<int>& arr) {
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
    
    FC myqueue;
    vector<thread> local_threads;

    // enqueue threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                myqueue.enqueue_queue(arr[j]);
                
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
                int value = myqueue.dequeue_queue();
                
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
    if (myqueue.dequeue_queue() != -1) {
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
    return 0;}

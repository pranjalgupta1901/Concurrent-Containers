#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>
#include <cassert>
#include <fstream>

using namespace std;

static void write_back_to_file(string out_file, vector<atomic<int>>& arr);

// Thread-safe random number generator
thread_local std::mt19937 generator(std::random_device{}());

// Node for the lock-free stack
class tstack {
public:
    class node {
    public:
        atomic<int> val;
        atomic<node*> down;

        node(int v) {
            val.store(v, memory_order_relaxed);
            down.store(nullptr, memory_order_relaxed);
        }
    };

private:
    atomic<node*> top = nullptr;

public:
    void elimination_tstack_push(int val);
    int elimination_tstack_pop();
};

// Global elimination array class
class slot_class {
public:
    atomic<bool> is_available;
    atomic<bool> is_push;
    atomic<int> value;

    slot_class() : is_available(true), is_push(false), value(0) {}
};

class e_class {
private:
    vector<slot_class> e_array;
    int size;

public:
    e_class(int size_array) : size(size_array), e_array(size_array) {}

    void init_elimination() {
        for (int i = 0; i < size; i++) {
            e_array[i].is_available.store(true, memory_order_relaxed);
            e_array[i].is_push.store(false, memory_order_relaxed);
            e_array[i].value.store(0, memory_order_relaxed);
        }
    }

    bool elimination(int& val, bool is_push_ops, int timeout_ms);
};

// Spin-lock stack with elimination support
class sgl {
private:
    vector<int> arr;
    mutex sgl_eli_lock;

public:
    void sgl_eli_push_stack(int val);
    int sgl_eli_pop_stack();
};

// Global elimination array instance
e_class eli(1000);

void init_eli() {
    eli.init_elimination();
}

// Elimination function
bool e_class::elimination(int& val, bool is_push_ops, int timeout_ms) {
    std::uniform_int_distribution<int> distribution(0, size - 1);
    int index = distribution(generator);

    clock_t clock_now = clock();
    while (clock_now + timeout_ms > clock()) {
        if (is_push_ops) { // Push operation
            if (e_array[index].is_available.exchange(false, memory_order_acquire)) {
                e_array[index].is_push.store(true, memory_order_release);
                e_array[index].value.store(val, memory_order_release);

                this_thread::sleep_for(chrono::milliseconds(timeout_ms));

                if (e_array[index].is_available.load(memory_order_acquire)) { // Pop succeeded
                    e_array[index].is_available.store(true, memory_order_release);
                    e_array[index].is_push.store(false, memory_order_release);
                    return true;
                } else { // Timeout happened
                    e_array[index].is_available.store(true, memory_order_release);
                    e_array[index].is_push.store(false, memory_order_release);
                    return false;
                }
            }
        } else { // Pop operation
            if (!e_array[index].is_available.load(memory_order_acquire) &&
                !e_array[index].is_push.load(memory_order_acquire)) {
                val = e_array[index].value.load(memory_order_acquire);
                e_array[index].is_available.store(true, memory_order_release);
                return true;
            }
        }
        index = distribution(generator); // Recompute index for fairness
    }
    return false;
}

// Lock-free stack push with retry logic
void tstack::elimination_tstack_push(int val) {
    node* n = new node(val);
    node* t;
    while (true) {
        t = top.load(memory_order_acquire);
        n->down.store(t, memory_order_release);
        if (top.compare_exchange_strong(t, n, memory_order_acq_rel)) {
            return;
        } else {
            // Elimination retry mechanism
            if (!eli.elimination(val, true, 200)) {
                continue;
            }
            return;
        }
    }
}

// Lock-free stack pop with retry logic
int tstack::elimination_tstack_pop() {
    node* t;
    node* n;
    int v;

    while (true) {
        t = top.load(memory_order_acquire);
        if (t == nullptr) {
            return -1;  // Stack is empty, handle it by returning -1
        }

        n = t->down.load(memory_order_acquire);
        v = t->val.load(memory_order_acquire);

        // Try to pop the top of the stack
        if (top.compare_exchange_strong(t, n, memory_order_acq_rel)) {
            delete t;  // Free the popped node
            return v;
        } else {
            // Elimination retry mechanism
            if (!eli.elimination(v, false, 200)) {
                // Retry if elimination fails
                continue;
            } else {
                // Elimination succeeded, return the value
                return v;
            }
        }
    }
}

// Spin-lock stack push with retry logic
void sgl::sgl_eli_push_stack(int val) {
    while (true) {
        if (sgl_eli_lock.try_lock()) {
            arr.push_back(val);
            sgl_eli_lock.unlock();
            return;
        } else {
            // Elimination retry mechanism
            if (!eli.elimination(val, true, 1000)) {
                continue;
            }
            return;
        }
    }
}

// Spin-lock stack pop with retry logic
int sgl::sgl_eli_pop_stack() {
    int val = 0;
    while (true) {
        if (sgl_eli_lock.try_lock()) {
            if (arr.empty()) {
                sgl_eli_lock.unlock();
                return -1;
            }
            val = arr.back();
            arr.pop_back();
            sgl_eli_lock.unlock();
            return val;
        } else {
            // Elimination retry mechanism
            if (!eli.elimination(val, false, 1000)) {
                continue;
            }
            return val;
        }
    }
}

int e_tstack_test_advanced(int num_threads, vector<int>& arr) {
    int num_thread_for_each_ops = num_threads / 2;

    vector<atomic<int>> test_push_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_pop_arr(num_thread_for_each_ops * arr.size());

    atomic<int> push_counter(0);
    atomic<int> pop_counter(0);
    atomic<int> test_counter(0);

    atomic<int> sum(0);
    int sum_actual = 0;

    tstack mystack;
    vector<thread> local_threads;

    // Push threads
    for (int i = 0; i < num_thread_for_each_ops; i++) {
        local_threads.push_back(thread([&, i]() {
            for (int j = 0; j < arr.size(); j++) {
                mystack.elimination_tstack_push(arr[j]);

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
                int value = mystack.elimination_tstack_pop();

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

    // Wait for all threads to complete
    for (auto& t : local_threads) {
        t.join();
    }

    // Calculate expected sum
    for (int j = 0; j < arr.size(); j++) {
        sum_actual += arr[j] * num_thread_for_each_ops;
    }

    // Verification checks
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

    // Verify sum
    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }

    write_back_to_file("Eli_Treiber_Push.txt", test_push_arr);
    write_back_to_file("Eli_Treiber_Pop.txt", test_pop_arr);

    cout << "Test passed successfully" << endl;
    return 0;
}

int e_sgl_stack_test_advanced(int num_threads, vector<int>& arr) {
    int num_thread_for_each_ops = (num_threads / 2);

    vector<atomic<int>> test_push_arr(num_thread_for_each_ops * arr.size());
    vector<atomic<int>> test_pop_arr(num_thread_for_each_ops * arr.size());

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
                mystack.sgl_eli_push_stack(arr[j]);

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
                int value = mystack.sgl_eli_pop_stack();

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

    // Wait for all threads to complete
    for (auto& t : local_threads) {
        t.join();
    }

    // Calculate expected sum
    for (int j = 0; j < arr.size(); j++) {
        sum_actual += arr[j] * num_thread_for_each_ops;
    }

    // Verification checks
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

    // Verify sum
    if (sum_actual != sum.load(memory_order_seq_cst)) {
        cout << "Sum mismatch" << endl;
        return -1;
    }

    write_back_to_file("Eli_SGL_Push.txt", test_push_arr);
    write_back_to_file("Eli_SGL_Pop.txt", test_pop_arr);

    cout << "Test passed successfully" << endl;
    return 0;
}

void write_back_to_file(string out_file, vector<atomic<int>>& arr) {
    ofstream file(out_file);
    for (size_t i = 0; i < arr.size(); i++) {
        file << arr[i].load() << " ";
    }
    file << endl;
    file.close();
}

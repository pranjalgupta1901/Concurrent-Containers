#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
#include "common_header_file.h"

using namespace std;
static void threadWorker(int id);

class SpuriousCondVar {
private:
    condition_variable& cv;
    mutex& mtx;

public:
    SpuriousCondVar(condition_variable& conditionVar, mutex& m) : cv(conditionVar), mtx(m) {}

    void waitForCondition(unique_lock<mutex>& lock, bool& condition) {
        cv.wait(lock, [&condition]() { return condition; });
    }

    void notifyAll() {
        cv.notify_all();
    }
};

static condition_variable cv;
static mutex m;
static bool wakeUpFlag = false;

static void threadWorker(int id) {
    unique_lock<mutex> lock(m);
    SpuriousCondVar spuriousCV(cv, m);
    spuriousCV.waitForCondition(lock, wakeUpFlag);
    cout << "Thread " << id << " is continuing as the condition is met." << endl;
}

void testSpuriousWakeups(int numThreads) {
    vector<thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(thread(threadWorker, i));
    }

    this_thread::sleep_for(chrono::seconds(1));

    {
        lock_guard<mutex> lock(m);
        wakeUpFlag = true;
    }

    cv.notify_all();

    for (auto& t : threads) {
        t.join();
    }
}


#ifndef COMMON_HEADER_FILE_H
#define COMMON_HEADER_FILE_H

#include <vector>

using namespace std;

int tstack_test_advanced(int num_threads, vector<int>&arr);

int msqueue_test_advanced(int num_threads, vector<int>&arr);

int e_tstack_test_advanced(int num_threads, vector<int> &arr);
void init_eli();

int sgl_stack_test_advanced(int num_threads, vector<int>& arr);

int e_sgl_stack_test_advanced(int num_threads, vector<int>& arr);

int sgl_queue_test_advanced(int num_threads, vector<int>& arr);

int fc_stack_test_advanced(int num_threads, vector<int>& arr);
int fc_queue_test_advanced(int num_threads, vector<int>&arr);

void testSpuriousWakeups(int numThreads);

#endif
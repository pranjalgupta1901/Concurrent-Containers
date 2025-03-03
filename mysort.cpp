#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <pthread.h>
#include <climits>
#include <thread>
#include <getopt.h>
#include <mutex>
#include <cstring>
#include "locks.h"
#include "common_header_file.h"

using namespace std;

// Global variables
string input_file = "";
bool print_name = false;
int num_threads = 0;

typedef enum{
    TREIBER_STACK = 0,
    M_and_S_QUEUE,
    SGL_STACK,
    SGL_QUEUE,
    TREIBER_STACK_ELI,
    SGL_STACK_ELI,
    FC_STACK,
    FC_QUEUE,
    SPURIOUS_WAKEUP
}container_type;

container_type container = TREIBER_STACK;


// Command-line argument processing and main function as before
/**
 * @brief Processes command-line arguments to configure input and output files,
 *        sorting algorithm, and other options.
 * 
 * @param argc The argument count.
 * @param argv The argument vector.
 */
void process_args(int argc, char* argv[]){

    const char* const short_args = "i:t:c:";
    const option long_args[] = {
        {"name", no_argument, nullptr, 'x'},
        {"input", required_argument, nullptr, 'i'},          // for input text file
        {"num_threads", required_argument, nullptr, 't'},     // for the number of threads
        {"container", required_argument, nullptr, 'c'},        // for container
        {nullptr, no_argument, nullptr, 0}
    };

    while(1){
        const auto opt = getopt_long(argc, argv, short_args, long_args, nullptr);

        if(opt == -1){
            break;
        }

        switch(opt){
            case 'i' : 
                input_file = string(optarg);
                break;
            
            case 'x':
                print_name = true;
                break;
            
            case 't':
                num_threads = atoi(optarg);
                break;
                          
            case 'c':
                if(strcmp(optarg, "treiber") == 0)
                    container = TREIBER_STACK;
                else if(strcmp(optarg, "m_and_s") == 0)
                    container = M_and_S_QUEUE;
                else if(strcmp(optarg, "sgl_stack") == 0)
                    container = SGL_STACK;
                else if(strcmp(optarg, "sgl_queue") == 0)
                    container = SGL_QUEUE;
                else if(strcmp(optarg, "treiber_eli") == 0)
                    container = TREIBER_STACK_ELI;
                else if(strcmp(optarg, "sgl_stack_eli") == 0)
                    container = SGL_STACK_ELI;
                else if(strcmp(optarg, "fc_stack") == 0)
                    container = FC_STACK;
                else if(strcmp(optarg, "fc_queue") == 0)
                    container = FC_QUEUE;
                else if(strcmp(optarg, "spurious") == 0)
                    container = SPURIOUS_WAKEUP;
                else
                    container = TREIBER_STACK;
                break;
 
            default:
                break;

        }
    }

}

int main(int argc, char* argv[]) {
    vector<int> read_array;
    process_args(argc, argv);
    if (print_name == true) {
        cout << "Pranjal Gupta" << endl;
    }

    if (num_threads == 0){
        num_threads = 4;
        cout<<"Setting the default threads to 4"<<endl;
    }else
        cout<<"Setting maximum threads as given threads in arguments which is "<<num_threads<<endl;
    
    ifstream input_file_var;

    if(container != SPURIOUS_WAKEUP)
        input_file_var.open(input_file);

    if (!input_file_var.is_open() && container != SPURIOUS_WAKEUP ) {
        cout << "File failed to open" << endl;
        return 1;
    }

    int value = 0;
    bool fail = false;
    while (input_file_var >> value)
        read_array.push_back(value);

    input_file_var.close();
    switch(container){
        case TREIBER_STACK:
            if(tstack_test_advanced(num_threads, read_array) != 0){
                cout<<"Treiber test with multiple threads failing"<<endl;
                fail = true;
            }

            break;
        
        case M_and_S_QUEUE:
            if(msqueue_test_advanced(num_threads, read_array) != 0){
                cout<<"msqueue test with multiple threads failing"<<endl;
                fail = true;
            }            
            break;
        
        case TREIBER_STACK_ELI:
            init_eli();
            if(e_tstack_test_advanced(num_threads, read_array) != 0){
                cout<<"Tstack fails in elimination"<<endl;
                fail = true;
            }
            break;
           
        case SGL_STACK_ELI:
            init_eli();
            if(e_sgl_stack_test_advanced(num_threads, read_array) != 0){
                cout<<"SGL stack elimination test with multiple threads failing"<<endl;
                fail = true;
            }   
            break;
            
        case SGL_STACK:
            if(sgl_stack_test_advanced(num_threads, read_array) != 0){
                cout<<"SGL stack test with multiple threads failing"<<endl;
                fail = true;
            }   
            break;
        
        case SGL_QUEUE:
            if(sgl_queue_test_advanced(num_threads, read_array) != 0){
                cout<<"SGL queue test with multiple threads failing"<<endl;
                fail = true;
            }   
            break;
            
         case FC_STACK:
            if(fc_stack_test_advanced(num_threads, read_array) != 0){
                cout<<"FC stack fails"<<endl;
                fail = true;
            }
            break;
            
        case FC_QUEUE:
            if(fc_queue_test_advanced(num_threads, read_array) != 0){
                cout<<"FC stack fails in elimination"<<endl;
                fail = true;
            }
            break;
            
        case SPURIOUS_WAKEUP:
            testSpuriousWakeups(num_threads);
            fail = false;
            break;
        
        default:
            break;

    }
    
    if(fail == true)
        cout<<"test failed"<<endl;
    else
        cout<<"test passed successfully"<<endl;

    return 0;
}


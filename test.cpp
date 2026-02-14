
#include<iostream>
#include<vector>
#include<chrono>
#include<atomic>
#include<thread>

#include "lf_queue.h"
#include "imp_macros.h"
#include "thread_utils.h"

#include<new>
#include<queue>
#include<mutex>
#include<algorithm>


struct Order {
	int stock_id;
	int quantity;
	bool buy;

	Order(int id = 5, int number_of_stocks = 6, bool is_buy = false) {
		stock_id = id;
		quantity = number_of_stocks;
		buy = is_buy;
	}
};

void showBenchmark(std::string text, std::vector<std::chrono::duration<double,std::nano>>& write, std::vector<std::chrono::duration<double,std::nano>>& read ) {
    std::sort(write.begin(),write.end());
    std::sort(read.begin(),read.end());

    int ind_fifty = write.size()/2;
    int ind_nine_zero = (9*write.size())/(10);
    int ind_ninty_nine = (99*write.size())/(100);
    int ind_nine_nine_nine = (999*write.size())/(1000);

    std::cout<<" ========================================= BENCHMARK PERFORMANCE FOR "<<text<<"================================ \n\n\n";
    /* ======================= BENCHMARK RESULTS NOW ===============================  */
    std::cout<<" WRITE LATENCIES \n";
    std::cout<<" P50 : "<<write[ind_fifty].count()<<"\n";
    std::cout<<" P90 : "<<write[ind_nine_zero].count()<<"\n";
    std::cout<<" P99 : "<<write[ind_ninty_nine].count()<<"\n";
    std::cout<<" P99.9 : "<<write[ind_nine_nine_nine].count()<<"\n";

    std::cout<<" READ LATENCIES \n";
    std::cout<<" P50 : "<<read[ind_fifty].count()<<"\n";
    std::cout<<" P90 : "<<read[ind_nine_zero].count()<<"\n";
    std::cout<<" P99 : "<<read[ind_ninty_nine].count()<<"\n";
    std::cout<<" P99.9 : "<<read[ind_nine_nine_nine].count()<<"\n\n";

    std::cout<<"\n========================================================================================================\n\n\n";
}



int main() {
	std::cout<<" Main Thread Executing \n";
    int set_size = 1000000; // 1 Million Orders
    std::vector<Order> test_set(set_size, Order(5,6,false));



    // ======================================= STD QUEUE + MUTEX LOCKS ==============================================
    std::queue<Order> std_queue;
    std::mutex mtx; 
    std::atomic<bool> done = {false};

    std::vector<std::chrono::duration<double,std::nano>> write_times;
    std::vector<std::chrono::duration<double,std::nano>> read_times;


    // writer lambda 
    auto writer_f = [](std::queue<Order>& std_queue,std::vector<Order>& test_set, std::vector<std::chrono::duration<double,std::nano>>& write_times , std::mutex& mtx, std::atomic<bool>& done ) {
        // write million entries with mutex locks

        for(int i = 0 ; i < 1000000; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            mtx.lock();
            std_queue.push(test_set[i]);
            mtx.unlock();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double,std::nano> interval = end - start;
            write_times.push_back(interval);
        }

        done = true;
    };

    // reader lambda
    auto reader_f = [](std::queue<Order>& std_queue, std::mutex& mtx, std::atomic<bool>& done, std::vector<std::chrono::duration<double,std::nano>>& read_times ) {
    int rc = 0;
    while (true) { // Loop forever until we explicitly break
        
        auto start = std::chrono::high_resolution_clock::now();
        mtx.lock();
        
        if (std_queue.empty()) {
            mtx.unlock();
            // If queue is empty AND writer is finished, we can stop.
            if (done) break; 
            
            // Otherwise, writer is still working, so we yield and wait for data
            std::this_thread::yield();
            continue;
        }

        // If we are here, queue has data
        Order entry = std_queue.front();
        std_queue.pop();
        rc += (entry.stock_id > 0 ? 1 : 0);
        mtx.unlock();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::nano> interval = end - start;
        read_times.push_back(interval);
    }
    };


    // writer thread
    auto writer_thread = internal_lib::createAndStartThread(3," std writer ", writer_f, std::ref(std_queue), std::ref(test_set), std::ref(write_times), std::ref(mtx), std::ref(done));
    // reader thread
    auto reader_thread = internal_lib::createAndStartThread(4," std reader ", reader_f, std::ref(std_queue), std::ref(mtx), std::ref(done), std::ref(read_times));
    

    if(writer_thread != nullptr) {
        writer_thread->join();
    }

    if(reader_thread != nullptr) {
        reader_thread->join();
    }


    showBenchmark(" std::queue + mutex ",write_times, read_times);


    // ========================================================   LOCK FREE QUEUE ========================================

    internal_lib::LFQueue<Order> ring_buffer(50000000); //  50 x 1M 

    std::vector<std::chrono::duration<double,std::nano>> lfq_write_times;
    std::vector<std::chrono::duration<double,std::nano>> lfq_read_times;

    // write lambda
    auto wf = [](internal_lib::LFQueue<Order>& comm, std::vector<std::chrono::duration<double,std::nano>>& lfq_write_times) {

        int cnt = 0; // I Million Orders
        while(cnt < 1000000) {
            auto start = std::chrono::high_resolution_clock::now();
            auto write_ptr = comm.getNextWriteIndex();
            while(write_ptr == nullptr) {
                std::this_thread::yield();
                write_ptr = comm.getNextWriteIndex();
            }   

            *write_ptr = Order(cnt, 100, true); 
            comm.updateWriteIndex();
            cnt++;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double,std::nano>  interval = end - start;
            lfq_write_times.push_back(interval);
        
        }
    };

    // read lambda
    auto rf = [](internal_lib::LFQueue<Order>& comm, std::vector<std::chrono::duration<double,std::nano>>& lfq_read_times) {
        int cnt = 0;
        while(cnt < 1000000) {
            auto start = std::chrono::high_resolution_clock::now();
            auto* read_ptr = comm.getNextReadIndex();
            while(read_ptr == nullptr) {
                std::this_thread::yield();
                read_ptr = comm.getNextReadIndex(); 
            }
            comm.updateNextReadIndex();
            cnt++;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double,std::nano>  interval = end - start;
            lfq_read_times.push_back(interval);
        }
    };


    // writer thread to lf_queue
    auto wt = internal_lib::createAndStartThread(6," writer to lq_queue ",wf,std::ref(ring_buffer),std::ref(lfq_write_times));
    // reader thread
    auto rt = internal_lib::createAndStartThread(5," writer to lq_queue ",rf,std::ref(ring_buffer),std::ref(lfq_read_times));


    if(wt != nullptr) {
        wt->join();
    }

    if(rt != nullptr) {
        rt->join();
    }

   showBenchmark(" LF_queue ",lfq_write_times, lfq_read_times);



	return 0;


}



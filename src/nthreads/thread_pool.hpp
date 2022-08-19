#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <future>
#include <unistd.h>
#include <sched.h>
#include "../nthreads/comparer.hpp"
#include "../nthreads/smoother.hpp"
#include "../nthreads/greyscale_converter.hpp"

using namespace std;
using namespace cv;

struct Task {  
    function<float()> f;
    int frame_number;
};

struct CompareTasks {
    /**
     * @brief Return true if t1 is ordered before t2
     * 
     * @param t1 task to compare
     * @param t2 task to compare
     * @return true if t1 is ordered before t2
     * @return false if t2 is ordered before t1 or it is equal to t1
     */
    bool operator()(Task const& t1, Task const& t2){
        return t1.frame_number < t2.frame_number;
    }
};

/**
 * @brief Class that manages the thread pool
 * 
 */
class ThreadPool {

    private: 

        // Values and flags specified by the user
        int nw = 0;
        bool show = false;
        bool times = false;
        bool mapping = false;

        // Variables and values used by the program
        int frame_number = 0;
        float threshold; // threshold to exceed to consider two pixels different
        float percent; // percentage of different pixels between frame and background to detect movement
        Mat background; // background frame

        // Variables for concurrency
        mutex lstop; // lock to set stop flag
        mutex l;
        atomic<int> res_number;
        atomic<int> different_frames;
        atomic<bool> stop; // flag to stop the thread pool
        priority_queue<Task, std::vector<Task>, CompareTasks> queue;
        condition_variable cond;
        vector<thread> tids;
        
        // Stages of the stream
        Smoother * smoother;
        GreyscaleConverter * converter;
        Comparer * comparer;

    public:

        ThreadPool(Smoother * smoother, GreyscaleConverter * converter, Comparer * comparer, int nw, Mat background, 
            float threshold, float percent, bool show, bool times, bool mapping):
            background(background), threshold(threshold), percent(percent), show(show), times(times), smoother(smoother), 
            converter(converter), comparer(comparer), nw(nw), mapping(mapping) {
                this -> stop = false;
                this -> frame_number = -1;
                this -> res_number = 0;
                this -> different_frames = 0;
            }

        /**
         * @brief Insert the initial task (grayscale conversion of the frame), 
         *        in this case check the queue size before inserting
         * 
         * @param t task to insert
         */
        void submit_initial_task(Task t) {
            bool submitted = false;
            while (!submitted) {
                {
                    unique_lock<mutex> lock(this -> l);
                    if (queue.size() <= 10) {
                        queue.push(t);
                        submitted = true;
                    }
                }
                if (!submitted) this_thread::sleep_for(std::chrono::microseconds(1000));
                else cond.notify_one();
            }
        }

        /**
         * @brief Insert a task in the queue and notify one worker
         * 
         * @param t task to insert
         */
        void submit_task(Task t) {
            {
                unique_lock<mutex> lock(this->l);
                queue.push(t);
            }
            cond.notify_one();
        } 

        /**
         * @brief Creates a task to convert a frame in black and white and putss it in the queue
         * 
         * @param m the frame to convert to grayscale
         */
        void submit_conversion_task(Mat * m, int n) {
            // Create the task
            auto f = [this, n] (Mat * m) {
                m = converter -> convert_to_greyscale(m);
                submit_smoothing_task(m, n);
                return (float)2;
            };
            // Insert the task in the queue
            auto fb = bind(f, m);
            Task t;
            t.frame_number = n;
            t.f = fb;
            submit_initial_task(t);
        }

        /**
         * @brief Create a task to performs smoothing on a matrix and puts it in the queue
         * 
         * @param m the matrix to smooth
         */
        void submit_smoothing_task(Mat * m, int n) {
            auto f = [this, n] (Mat * m) {
                m = (this->smoother)->smoothing(m);
                submit_result_task(m, n);
                return (float)3;
            };
            // Insert the task in the queue
            auto fb = (bind(f, m));
            Task t;
            t.frame_number = n;
            t.f = fb;
            submit_task(t);
        }

        /**
         * @brief Creates a task to compare a frame with background and puts it in the queue
         * 
         * @param m the frame to compare to background
         */
        void submit_result_task(Mat * m, int n) {
            // Create the task
            auto f = [this, n] (Mat * m) {
                return this->comparer->different_pixels(m);
            };
            // Insert the task in the queue
            auto fb = (bind(f, m));
            Task t;
            t.frame_number = n;
            t.f = fb;
            submit_task(t);
        }

        

        /**
         * @brief Create the threads of the pool and starts them
         * 
         */
        void start_pool() {

            auto body_global = [&] {
                float res = 0;
                function<float()> f = []() {return -1;};
                Task t;
                // Loop until background subtraction is done for all the frames of the video
                while (this->res_number <= this->frame_number || this->frame_number < 0) {
                    {
                        unique_lock<mutex> lock(this -> l);
                        cond.wait(lock, [&](){return(!queue.empty() || (this->stop) || 
                            (this->res_number == this->frame_number && this->frame_number >= 0));});
                        if (!queue.empty()) {
                            t = queue.top();//front();
                            queue.pop();//pop_front();
                        }
                        else if (this ->stop) {
                            break;
                        }
                        else if (this->res_number == this->frame_number && this->frame_number >= 0) {
                            break;
                        }
                    }
                    // Execute task
                    res = t.f();
                    if (res >= 0 && res <= 1) { // Case background subtraction
                        if (res > this->percent) this->different_frames++;
                        this -> res_number++;
                        if (times) cout << "Frames with movement detected until now: " << this->different_frames << " over " << res_number << " analyzed" << endl;
                    } // If it is the grayscale conversion or smoothing case, it is not needed to do anything here
                    if (this->res_number == this->frame_number && this->frame_number >= 0) break;
                }
                // Stop the pool when all the frame have been analysed
                {
                    unique_lock<mutex> lock(this -> lstop);
                    if (!stop) {
                        this -> stop = true;
                        cond.notify_all();
                    }
                }
            };
            int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
            for (int i=0; i<(this->nw); i++) {
                (this -> tids).push_back(thread(body_global));
                if (mapping) {
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    CPU_SET(i%numCPU, &cpuset);
                    int rc = pthread_setaffinity_np((this -> tids).at(i).native_handle(), sizeof(cpu_set_t), &cpuset);
                    assert(rc == 0);
                }
            }
        }

        /**
         * @brief Communicate the number of frames from the reader when it has finished
         * 
         * @param n the number of frames taken by the video
         */
        void communicate_frames_number(int n) {
            this -> frame_number = n;
            cond.notify_all();
        }

        /**
         * @brief Get the final result from outside the pool
         * 
         * @return the number of frames with movement detected
         */
        int get_final_result() {
            for (int i=0; i<(this->nw); i++) {
                (this -> tids)[i].join();
            }
            // Delete the stages of the pipe
            delete smoother;
            delete converter;
            delete comparer;
            return this->different_frames;
        }

};
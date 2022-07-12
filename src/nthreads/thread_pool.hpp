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
#include "../nthreads/comparer.hpp"
#include "../nthreads/smoother.hpp"
#include "../nthreads/greyscale_converter.hpp"

using namespace std;
using namespace cv;

/**
 * @brief Class that manages the thread pool
 * 
 */
class ThreadPool {

    private: 
        int cm_p = 0;
        int sm_p = 0;
        mutex l; // mutex for smoothing tasks
        mutex lr; // mutex for conversion or comparing tasks
        mutex lstop; // lock to set stop flag
        condition_variable cond; // cond variable for smoothing tasks
        condition_variable cond_res; // cond variable for conversion or comparing tasks
        vector<thread> sm_tids; // number of stream parallel workers for smoothing
        vector<thread> cc_tids; // number of stream parallel workers for grayscale conversion and background subtraction
        deque<function<Mat()>> tasks; // queue for smoothing tasks
        deque<function<float()>> results; // queue for results of smoothing or frames to be converted
        Mat background; // background frame
        atomic<bool> stop; // flag to stop the thread pool
        bool show = false;
        bool times = false; 
        atomic<int> res_number; // number of frames analyzed
        atomic<int> frame_number; // total number of frames
        atomic<int> different_frames; // number of frames with movement detected (different from background)
        float threshold; // threshold to exceed to consider two pixels different
        float percent; // percentage of different pixels between frame and background to detect movement
        promise<int> p; // promise used at the end to get the final result from outside the pool
        future<int> f = (this->p).get_future();
        // Stages of the stream
        Smoother * smoother;
        GreyscaleConverter * converter;
        Comparer * comparer;

    public:
        ThreadPool(Smoother * smoother, GreyscaleConverter * converter, Comparer * comparer, int cm_p, int sm_p, Mat background, 
            float threshold, float percent, bool show, bool times):
            background(background), threshold(threshold), percent(percent), show(show), times(times), smoother(smoother), 
            converter(converter), comparer(comparer), cm_p(cm_p), sm_p(sm_p) {
                this -> stop = false;
                this -> frame_number = -1;
                this -> res_number = 0;
                this -> different_frames = 0;
            }

        /**
         * @brief Creates a task to convert a frame in black and white and putss it in the queue
         * 
         * @param m the frame to convert to grayscale
         */
        void submit_conversion_task(Mat m) {
            // Create the task
            auto f = [this] (Mat m) {
                m = converter -> convert_to_greyscale(m);
                submit_task(m);
                return (float)2;
            };
            // Insert the task in the queue
            auto fb = bind(f, m);
            {
                unique_lock<mutex> lock(this -> lr);
                results.push_back(fb);
            }
            cond_res.notify_one();
        }

        /**
         * @brief Creates a task to compare a frame with background and puts it in the queue
         * 
         * @param m the frame to compare to background
         */
        void submit_result(Mat m) {
            // Create the task
            auto f = [this] (Mat m) {
                return this->comparer->different_pixels(m);
            };
            // Insert the task in the queue
            auto fb = (bind(f, m));
            {
                unique_lock<mutex> lock(this -> lr);
                results.push_back(fb);
            }
            cond_res.notify_one();
        }

        /**
         * @brief Create a task to performs smoothing on a matrix and puts it in the queue
         * 
         * @param m the matrix to smooth
         */
        void submit_task(Mat m) {
            auto f = [this] (Mat m) {
                return (this->smoother)->smoothing(m);
            };
            auto fb = bind(f, m);
            {
                unique_lock<mutex> lock(this -> l);
                tasks.push_back(fb);
            }
            cond.notify_one();
        }

        /**
         * @brief Create the threads of the pool and starts them
         * 
         */
        void start_pool() {

            // Body of the thread that performs smoothing tasks
            auto body_sm = [&] () {
                function<Mat()> t = []() {return Mat::ones(2, 2, CV_32F);};
                while (!(this->stop)) {
                    {
                        unique_lock<mutex> lock(this -> l);
                        cond.wait(lock, [&](){return(!tasks.empty() || (this->stop));});
                        if (!tasks.empty()) {
                            t = tasks.front();
                            tasks.pop_front();
                        }
                        else if (this->stop){
                            return;
                        } 
                    }
                    Mat s = t();
                    this -> submit_result(s);
                    if (this->stop) return;
                }
            };
            
            // Body of the thread that do greyscale conversion or background subtraction depending on the task it takes
            auto body_cc = [&] () {
                float res = 0;
                function<float()> t = []() {return -1;};
                // Loop until background subtraction is done for all the frames of the video
                while (this->res_number <= this->frame_number || this->frame_number < 0) {
                    {
                        unique_lock<mutex> lock(this -> lr);
                        cond_res.wait(lock, [&](){return(!results.empty() || (this->stop) || 
                            (this->res_number == this->frame_number && this->frame_number >= 0));});
                        if (!results.empty()) {
                            t = results.front();
                            results.pop_front();
                        }
                        else if (this ->stop) {
                            break;
                        }
                        else if (this->res_number == this->frame_number && this->frame_number >= 0) {
                            break;
                        }
                    }
                    // Execute task
                    res = t();
                    if (res >= 0 && res <= 1) { // Case background subtraction
                        if (res > this->percent) this->different_frames++;
                        this -> res_number++;
                        cout << "Frames with movement detected until now: " << this->different_frames << " over " << res_number << " analyzed" << endl;
                    } // If it is the grayscale conversion case, it is not needed to do anything here
                    if (this->res_number == this->frame_number && this->frame_number >= 0) break;
                }
                // Stop the pool when all the frame have been analysed
                {
                    unique_lock<mutex> lock(this -> lstop);
                    if (!stop) {
                        this -> stop = true;
                        cond.notify_all();
                        cond_res.notify_all();
                        // Set the value of the promise to communicate it to the main program
                        (this->p).set_value(this->different_frames);
                    }
                }
            };
            // Starts the thread
            for (int i=0; i<(this->cm_p); i++) {
                (this -> cc_tids).push_back(thread(body_cc));
            }
            for (int i=0; i<(this->sm_p); i++) {
                (this -> sm_tids).push_back(thread(body_sm));
            }
        }

        /**
         * @brief Communicate the number of frames from the reader when it has finished
         * 
         * @param n the number of frames taken by the video
         */
        void communicate_frames_number(int n) {
            this -> frame_number = n;
            cond_res.notify_all();
        }

        /**
         * @brief Get the final result from outside the pool
         * 
         * @return the number of frames with movement detected
         */
        int get_final_result() {
            for (int i=0; i<(this->sm_p); i++) {
                (this -> sm_tids)[i].join();
            }
            for (int i=0; i<(this->cm_p); i++) {
                (this -> cc_tids)[i].join();
            }
            // Delete the stages of the pipe
            delete smoother;
            delete converter;
            delete comparer;
            // Return the total number of frames with movement detected
            return (this->f).get();
        }

};
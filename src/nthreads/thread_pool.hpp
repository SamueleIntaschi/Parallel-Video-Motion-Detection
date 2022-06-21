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
#include "../utils/smoother.hpp"
#include "../nthreads/greyscale_converter.hpp"

using namespace std;
using namespace cv;

/**
 * @brief Class that manages the thread pool
 * 
 */
class ThreadPool {

    private: 
        int sw; // Smoothing workers
        int cw; // Map workers for conversion and background subtraction
        mutex l; // mutex for smoothing tasks
        mutex lr; // mutex for conversion or comparing tasks
        condition_variable cond; // cond variable for smoothing tasks
        condition_variable cond_res; // cond variable for conversion or comparing tasks
        vector<thread> tids;
        thread converter_comparer; // thread that performs conversion or comparing
        deque<function<Mat()>> tasks; // queue for smoothing tasks
        deque<function<float()>> results; // queue for results of smoothing or frames to be converted
        Mat filter;
        Mat background;
        bool stop = false; // flag to stop the thread pool
        bool show = false;
        bool times = false;
        atomic<int> res_number;
        atomic<int> frame_number;
        atomic<int> different_frames;
        float threshold; // threshold to exceed to consider two pixels different
        float percent; // percentage of different pixels between frame and background to detect movement
        promise<int> p; // promise used at the end to get the final result form outside the pool
        future<int> f = (this->p).get_future();

    public:
        ThreadPool(Mat filter, int sw, Mat background, int cw, float threshold, float percent, bool show, bool times):
            cw(cw), sw(sw), filter(filter), background(background), threshold(threshold), percent(percent), 
            show(show), times(times) {
                this -> frame_number = 0;
                this -> res_number = 0;
                this -> different_frames = 0;
            }

        /**
         * @brief Creates a task to convert a frame in black and white and inserts it to the queue
         * 
         * @param m the frame to convert to grayscale
         */
        void submit_conversion_task(Mat m) {
            // Create the task
            auto f = [this] (Mat m) {
                GreyscaleConverter converter(m, this->cw, this->show, this->times);
                m = converter.convert_to_greyscale();
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
         * @brief Creates a task to compare a frame with background and inserts it to the queue
         * 
         * @param m the frame to compare to background
         */
        void submit_result(Mat m) {
            // Create the task
            auto f = [this] (Mat m) {
                Comparer c(this->background, m, this->cw, this->threshold, this->show, this->times);
                return c.different_pixels();
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
         * @brief Create a task to performs smoothing on a matrix and inserts it to the queue
         * 
         * @param m the matrix to smooth
         */
        void submit_task(Mat m) {
            auto f = [this] (Mat m, Mat filter) {
                Smoother s(m, filter, this->show, this->times);
                return s.smoothing();
            };
            auto fb = bind(f, m, this->filter);
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
            auto body_sm = [&] (int i) {
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
                }
            };
            
            // Body of the thread that performs greyscale conversion or background subtraction depending on the task it takes
            auto body_cc = [&] (promise<int> && pp) {
                float res = 0;
                int diff_frames = 0;
                function<float()> t = []() {return -1;};
                // Loop until background subtraction is done for all the frames of the video
                while (this->res_number <= this->frame_number) {
                    {
                        unique_lock<mutex> lock(this -> lr);
                        cond_res.wait(lock, [&](){return(!results.empty() || (this->stop));});
                        if (!results.empty()) {
                            t = results.front();
                            results.pop_front();
                        } 
                    }
                    // Execute task
                    res = t();
                    if (res == 2) { // Case greyscale conversion
                        this -> frame_number++;
                    }
                    else if (res == -1) { // Case end
                        break;
                    }
                    else { // Case background subtraction
                        if (res > this->percent) diff_frames++;
                        this -> res_number++;
                        cout << "Frames with movement detected until now: " << diff_frames << " over " << res_number << " analyzed" << endl;
                    }
                    if (res_number == frame_number) break;
                }
                // Stop the pool when all the frame have been analysed
                stop_pool();
                // Set the value of the promise to communicate it to the main program
                pp.set_value(diff_frames);
            };
            // Starts the thread
            for (int i=0; i<(this->sw); i++) {
                (this -> tids).push_back(thread(body_sm, i));
            }
            this -> converter_comparer = thread(body_cc, move(this->p));
        }

        /**
         * @brief Stop the threads of the pool
         * 
         */
        void stop_pool() {
            this -> stop = true;
            cond.notify_all();
            cond_res.notify_all();
            for (int i=0; i<(this->sw); i++) {
                (this -> tids)[i].join();
            }
            return;
        }

        /**
         * @brief Get the final result from outside the pool
         * 
         * @return the number of frames with movement detected
         */
        int get_final_result() {
            (this->converter_comparer).join();
            return (this->f).get();
        }

};
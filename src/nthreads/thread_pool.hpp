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

class ThreadPool {

    private: 
        int sw;
        int cw;
        mutex l;
        mutex lr;
        condition_variable cond;
        condition_variable cond_res;
        vector<thread> tids;
        thread cc;
        deque<function<Mat()>> tasks;
        deque<function<float()>> results;
        Mat filter;
        Mat background;
        bool stop = false;
        bool show = false;
        bool times = false;
        atomic<int> res_number;
        atomic<int> frame_number;
        atomic<int> different_frames;
        float threshold;
        promise<int> p;
        future<int> f = (this->p).get_future();
        float percent;

        

    public:
        ThreadPool(Mat filter, int sw, Mat background, int cw, float threshold, float percent, bool show, bool times):
            cw(cw), sw(sw), filter(filter), background(background), threshold(threshold), percent(percent), 
            show(show), times(times) {
                this -> frame_number = 0;
                this -> res_number = 0;
                this -> different_frames = 0;
            }

        void submit_conversion_task(Mat m) {
            auto f = [this] (Mat m) {
                GreyscaleConverter converter(m, this->cw, this->show, this->times);
                m = converter.convert_to_greyscale();
                submit_task(m);
                return (float)2;
            };
            auto fb = bind(f, m);
            {
                unique_lock<mutex> lock(this -> lr);
                results.push_back(fb);
            }
            cond_res.notify_one();
        }

        void submit_result(Mat m) {
            auto f = [this] (Mat m) {
                Comparer c(this->background, m, this->cw, this->threshold, this->show, this->times);
                return c.different_pixels();
            };
            auto fb = (bind(f, m));
            {
                unique_lock<mutex> lock(this -> lr);
                results.push_back(fb);
            }
            cond_res.notify_one();
        }

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

        void start_pool() {
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
            auto body_cc = [&] (promise<int> && pp) {
                float res = 0;
                int diff_frames = 0;
                function<float()> t = []() {return -1;};
                while (this->res_number <= this->frame_number) {
                    {
                        unique_lock<mutex> lock(this -> lr);
                        cond_res.wait(lock, [&](){return(!results.empty() || (this->stop));});
                        if (!results.empty()) {
                            t = results.front();
                            results.pop_front();
                        } 
                    }
                    res = t();
                    if (res == 2) { // Case conversion
                        this -> frame_number++;
                    }
                    else if (res == -1) { // Case end
                        break;
                    }
                    else { // Case comparing
                        if (res > this->percent) diff_frames++;
                        this -> res_number++;
                        cout << "Frames with movement detected until now: " << diff_frames << " over " << res_number << " analyzed" << endl;
                    }
                    if (res_number == frame_number) break;
                }
                stop_pool();
                pp.set_value(diff_frames);
            };
            for (int i=0; i<(this->sw); i++) {
                (this -> tids).push_back(thread(body_sm, i));
            }
            this -> cc = thread(body_cc, move(this->p));
        }

        void stop_pool() {
            this -> stop = true;
            cond.notify_all();
            cond_res.notify_all();
            for (int i=0; i<(this->sw); i++) {
                (this -> tids)[i].join();
            }
            return;
        }

        int get_final_result() {
            (this->cc).join();
            return (this->f).get();
        }

};
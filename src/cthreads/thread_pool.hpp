#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>
#include "../common_classes/smoother.hpp"
#include "comparer.hpp"

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
        deque<function<Mat()>> tasks;
        deque<function<float()>> results;
        Mat filter;
        Mat background;
        bool stop = false;
        bool show = false;
        bool times = false;
        float threshold;

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

    public:
        ThreadPool(Mat filter, int sw, Mat b, int cw, float threshold, bool show, bool times) {
            this -> cw = cw;
            this -> sw = sw;
            this -> filter = filter;
            this -> background = b;
            this -> show = show;
            this -> times = times;
            this -> threshold = threshold;
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
            auto body = [&] (int i) {
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
            for (int i=0; i<(this->sw); i++) {
                (this -> tids).push_back(thread(body, i));
            }
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

        function<float()> get_result() {
            function<float()> t = []() {return -1;};
            {
                unique_lock<mutex> lock(this -> lr);
                cond_res.wait(lock, [&](){return(!results.empty() || (this->stop));});
                if (!results.empty()) {
                    t = results.front();
                    results.pop_front();
                }
                else if (this->stop){
                    return t;
                } 
            }
            return t;
        }

};
#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;
using namespace std;
using namespace cv;

class Comparer {
    private:
        Mat background;
        Mat frame;
        int nw;
        bool show = false;
        bool times = false;
        float threshold;
    
    public:
    
        Comparer(Mat b, Mat f, int nw, float threshold, bool show, bool times) {
            this -> background = b;
            this -> frame = f;
            this -> nw = nw;
            this -> show = show;
            this -> times = times;
            this -> threshold = threshold;
        }

        float different_pixels() {
            Mat res = Mat((this->frame).rows, (this->frame).cols, CV_32F);
            float * pa = (float *) (this->frame).data;
            float * pb = (float *) (this->background).data;
            float * pres = (float *) res.data;
            int different_pixels = 0;
            auto comparing = [&] (int i, int &diff_pixels) {
                for (int j=0; j<(this->frame).cols; j++) {
                    pres[i * (this->frame).cols + j] = abs(pb[i * (this->frame).cols + j] - pa[i * (this->frame).cols + j]);
                    if (pres[i * (this->frame).cols + j] > this->threshold) diff_pixels++;
                }
            };
            auto reduce = [] (int &diff_pixels, int e) {
                diff_pixels += e;
            };
            auto start = std::chrono::high_resolution_clock::now();
            ParallelForReduce<int> pf(this->nw);
            pf.parallel_reduce(different_pixels, 0, 0, (this->frame).rows, 1, comparing, reduce, this->nw);
            float diff_pixels_fraction = (float) different_pixels/(this->frame).total();
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times passed to compare frame with background: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Background Subtraction", res);
                waitKey(25);
            }
            return diff_pixels_fraction;
        }
};
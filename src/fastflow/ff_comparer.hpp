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
    
    public:
        Comparer(Mat b, Mat f, int nw, bool show) {
            this -> background = b;
            this -> frame = f;
            this -> nw = nw;
            this -> show = show;
        }

        float different_pixels() {
            auto start = std::chrono::high_resolution_clock::now();
        
            Mat res = Mat((this->frame).rows, (this->frame).cols, CV_32F);
            float * pa = (float *) (this->frame).data;
            float * pb = (float *) (this->background).data;
            float * pres = (float *) res.data;
            auto comparing = [&res, pa, pb, pres] (int i, int &diff_pixels) {
                for (int j=0; j<res.cols; j++) {
                    pres[i * res.cols + j] = abs(pb[i * res.cols + j] - pa[i * res.cols + j]);
                    if (pres[i * res.cols + j] > 0) diff_pixels++;
                }
            };
            auto reduce = [] (int &s, int e) {
                s += e;
            };

            int different_pixels = 0;
            ParallelForReduce<int> pf(nw, true);
            pf.parallel_reduce(different_pixels, 0, 0, (this->frame).rows, 1, comparing, reduce, nw);

            float diff_pixels_fraction = (float) different_pixels/(this->frame).total();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cout << "Times passed to compare frame with background: " << usec << " usec" << endl;
            if (show) {
                imshow("Background Subtraction", res);
                waitKey(25);
            }
            return diff_pixels_fraction;
        }
};
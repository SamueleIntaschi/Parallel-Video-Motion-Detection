#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace std;
using namespace cv;

class Comparer: public ff_Map<Mat> {
    private: 
        bool show = false;
        bool times = false;
        Mat background;
        float threshold;
        float percent;
        int cw;
        int different_frames = 0;
        int frame_number = 0;
        bool has_finished = false;

    public:
        Comparer(Mat background, int cw, float percent, float threshold, bool show, bool times): 
            background(background), cw(cw), percent(percent), threshold(threshold), show(show), times(times) {}

        Mat * svc(Mat * m) {
            if (m != EOS) {

                Mat r = *m;
                Mat res = Mat(r.rows, r.cols, CV_32F);
                float * pa = (float *) r.data;
                float * pb = (float *) (this->background).data;
                float * pres = (float *) res.data;
                int different_pixels = 0;
                auto comparing = [&] (int i, int &diff_pixels) {
                    for (int j=0; j<r.cols; j++) {
                        pres[i * r.cols + j] = abs(pb[i * r.cols + j] - pa[i * r.cols + j]);
                        if (pres[i * r.cols + j] > this->threshold) diff_pixels++;
                    }
                };
                auto reduce = [] (int &diff_pixels, int e) {
                    diff_pixels += e;
                };
                auto start = std::chrono::high_resolution_clock::now();
                ff_Map<Mat,Mat,int>::parallel_reduce(different_pixels, 0, 0, r.rows, 1, comparing, reduce, this->cw);
                float diff_pixels_fraction = (float) different_pixels/r.total();
                if (times) {
                    auto duration = std::chrono::high_resolution_clock::now() - start;
                    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                    cout << "Times passed to compare frame with background: " << usec << " usec" << endl;
                }
                if (show) {
                    imshow("Background Subtraction", res);
                    waitKey(25);
                }
                if (diff_pixels_fraction > this->percent) (this->different_frames)++;
                delete m;
            }
            (this -> frame_number)++;
            cout << "Frames with movement detected until now: " << different_frames << " over " << frame_number << " analyzed" << endl;
            return GO_ON;
        }

        void svc_end() {
            cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
            this -> has_finished = true;
        }

        int get_different_frames_number() {
            if (this -> has_finished) return this->different_frames;
            else return -1;
        }
};
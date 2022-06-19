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
    
        Comparer(Mat background, Mat frame, int nw, float threshold, bool show, bool times):
            background(background), frame(frame), nw(nw), threshold(threshold), show(show), times(times) {}

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
            ParallelForReduce<int> pf(this->nw, true);
            pf.parallel_reduce(different_pixels, 0, 0, (this->frame).rows, 1, comparing, reduce, this->nw);
            //pf.ffStats(cout);
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

class ComparerCollector: public ff_minode_t<Mat> {
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
        ComparerCollector(Mat background, int cw, float percent, float threshold, bool show, bool times): 
            background(background), cw(cw), percent(percent), threshold(threshold), show(show), times(times) {}

        Mat * svc(Mat * m) {
            if (m != EOS) {
                Comparer c(this->background, *m,this->cw, this->threshold, this->show, this->times);
                float diff = c.different_pixels();
                if (diff > this->percent) (this->different_frames)++;
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

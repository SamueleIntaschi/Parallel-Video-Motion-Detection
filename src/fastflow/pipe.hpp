#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include "ff_greyscale_converter.hpp"
#include "../common_classes/smoother.hpp"
#include "ff_comparer.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>

using namespace ff;
using namespace std;
using namespace cv;

class Converter: public ff_monode_t<Mat> {

    private:
        int cw;
        string filename;
        int frame_number = 0;
        bool show = false;
        bool times = false;
        VideoCapture cap;

    public:

        Converter(VideoCapture cap, int cw, bool show, bool times): cap(cap), cw(cw), show(show), times(times) {}

        Mat * svc (Mat *) {

            while (true) {

                Mat frame; 
                this -> cap >> frame;
                if (frame.empty()) break;
                frame.convertTo(frame, CV_32F, 1.0/255.0);

                // Data parallel conversion
                Mat * m = new Mat(frame.rows, frame.cols, CV_32F);
                GreyscaleConverter converter(frame, cw, this->show, this->times);
                *m = converter.convert_to_greyscale();
                ff_send_out(m);
                frame_number++;

            }
            (this->cap).release();
            return EOS;
        }
};

class SmoothingWorker: public ff_node_t<Mat> {
    private:
        bool show = false;
        bool times = false;
        Mat filter;

    public:
        SmoothingWorker(Mat filter, bool show, bool times): filter(filter), show(show), times(times) {}

        Mat * svc(Mat * m) {
            
            if (m != EOS) {
                Smoother s(*m, this->filter, this->show, this->times);
                *m = s.smoothing();
                return(m);
            }
            
            return m;
        }
};

class Comparing: public ff_minode_t<Mat> {
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
        Comparing(Mat background, int cw, float percent, float threshold, bool show, bool times): 
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

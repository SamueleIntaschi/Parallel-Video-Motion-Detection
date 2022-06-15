#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include "ff_greyscale_converter.hpp"
#include "../cthreads/smoother.hpp"
#include "ff_comparer.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>

class Converter: public ff_monode_t<Mat> {

    private:
        int cw;
        string filename;
        int frame_number = 0;
        bool show;
        VideoCapture cap;

    public:

        Converter(VideoCapture cap, int cw, bool show): cap(cap), cw(cw), show(show) {}

        Mat * svc (Mat *) {

            while (true) {

                Mat frame; 
                this -> cap >> frame;
                if (frame.empty()) break;
                frame.convertTo(frame, CV_32F, 1.0/255.0);

                // Data parallel conversion
                GreyscaleConverter converter(frame, cw, this->show);
                Mat * m = converter.convert_to_greyscale();
                ff_send_out(m);
                frame_number++;

            }
            (this->cap).release();
            return EOS;
        }
};

class SmoothingWorker: public ff_node_t<Mat> {
    private:
        bool show;
        Mat filter;
        //int cw;

    public:
        SmoothingWorker(Mat filter, bool show): filter(filter), show(show) {}

        Mat * svc(Mat * m) {
            
            if (m != EOS) {
                Smoother s(*m, this->filter, this->show);
                *m = s.smoothing();
                return(m);
            }
            
            return m;
        }
};

class Comparing: public ff_minode_t<Mat> {
    private: 
        bool show;
        Mat background;
        float threshold;
        float percent;
        int cw;
        int different_frames = 0;
        int frame_number = 0;
        bool has_finished = false;

    public:
        Comparing(Mat background, int cw, float percent, float threshold, bool show): background(background), cw(cw), percent(percent), threshold(threshold), show(show) {}

        Mat * svc(Mat * m) {
            if (m != EOS) {
                Comparer c(this->background, *m,this->cw, this->threshold, this->show);
                float diff = c.different_pixels();
                cout << diff << " " << this->percent << endl;
                if (diff > this->percent) (this->different_frames)++;
                delete m;
            }
            (this -> frame_number)++;
            cout << "Frames with movement detected until now: " << different_frames << " over " << frame_number << " analyzed" << endl;
            return GO_ON;
        }

        void svc_end() {
            cout << "Number of frames with movement detected: " << different_frames << endl;
            this -> has_finished = true;
        }

        int get_different_frames_number() {
            if (this -> has_finished) return this->different_frames;
            else return -1;
        }
};

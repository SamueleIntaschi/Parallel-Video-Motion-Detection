#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>

using namespace std;
using namespace cv;
using namespace ff;

/**
 * @brief Class representing the emitter of the pipe, that reads frames from the source and send them to the node that 
 *        performs grayscale conversion and smoothing
 * 
 */
class Source: public ff_monode_t<Mat> {

    private:
        bool show = false;
        bool times = false;
        // Background matrix to know the frames size
        Mat background;
        VideoCapture cap;

    public:
        Source(Mat background, VideoCapture cap, bool show, bool times): cap(cap), background(background), show(show), times(times) {}

        /**
         * @brief Main function of the emitter node, it reads frames, converts to grayscale and submits to next node
         * 
         * @return EOS when the frames are finished
         */
        Mat * svc (Mat *) {
            while (true) {
                Mat * frame = new Mat(background.rows, background.cols, CV_32FC3);
                this -> cap >> *frame;
                if (frame->empty()) break;
                frame->convertTo(*frame, CV_32F, 1.0/255.0);
                ff_send_out(frame);
            }
            (this->cap).release();
            return EOS;
        }
};
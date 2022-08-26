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
class Emitter: public ff_monode_t<Task> {

    private:
        bool show = false;
        bool times = false;
        int frame_number = 0;
        // Background matrix to know the frames size
        Mat background;
        VideoCapture cap;

    public:
        Emitter(Mat background, VideoCapture cap, bool show, bool times): cap(cap), background(background), show(show), times(times) {}

        /**
         * @brief Main function of the emitter node, it reads frames, converts to grayscale and submits to next node
         * 
         * @return EOS when the frames are finished
         */
        Task * svc (Task *) {
            while (true) {
                Mat * frame = new Mat(background.rows, background.cols, CV_32FC3);
                this -> cap >> *frame;
                if (frame->empty()) break;
                frame->convertTo(*frame, CV_32F, 1.0/255.0);
                this->frame_number++;
                Task * t = new Task;
                t -> m = frame;
                t -> n = 2;
                ff_send_out(t);
            }
            (this->cap).release();
            Task * t = new Task;
            t -> n = this -> frame_number;
            ff_send_out(t);
            return EOS;
        }
};
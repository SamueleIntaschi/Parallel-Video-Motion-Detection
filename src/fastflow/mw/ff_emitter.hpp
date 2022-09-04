#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>

using namespace std;
using namespace cv;
using namespace ff;

/**
 * @brief Class representing the emitter of the pipe, it reads frames and sends them to the master
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
         * @brief Main function of the emitter node, it reads frames, prepares tasks and submits them to workers
         * 
         * @return EOS when the frames are finished
         */
        Task * svc (Task *) {
            while (true) {
                // Reads frames and generate tasks
                Mat * frame = new Mat(background.rows, background.cols, CV_32FC3);
                this -> cap >> *frame;
                if (frame->empty()) break;
                frame->convertTo(*frame, CV_32F, 1.0/255.0);
                this->frame_number++;
                // Task creation
                Task * t = new Task;
                t -> m = frame;
                t -> n = 2; // Task code for greyscale conversion
                ff_send_out(t);
            }
            (this->cap).release();
            // Sends the total number of frames as task code
            Task * t = new Task;
            t -> n = this -> frame_number;
            ff_send_out(t);
            return EOS;
        }
};
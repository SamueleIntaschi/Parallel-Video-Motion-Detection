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

class Collector: public ff_minode_t<float> {
    private:
        int frames_with_movement = 0;
        int frame_number = 0;
        float percent;
        bool has_finished = false;


    public:

        Collector(float percent): percent(percent) {}

        float * svc(float * diff) {
            float &d = *diff;
            if (d > this->percent) (this->frames_with_movement)++;
            delete diff;
            (this -> frame_number)++;
            cout << "Frames with movement detected until now: " << frames_with_movement << " over " << frame_number << " analyzed" << endl;
            return GO_ON;
        }

        /**
         * @brief function executed when the node has finished its work, it prints a message and update a flag
         * 
         */
        void svc_end() {
            cout << "Number of frames with movement detected: " << frames_with_movement << " on a total of " << frame_number << " frames" << endl;
            this -> has_finished = true;
        }

        /**
         * @brief Get the number of frames with movement detected from outside the node if the stream is finished
         * 
         * @return the number of frames with movement detected
         */
        int get_different_frames_number() {
            if (this -> has_finished) return this->frames_with_movement;
            else return -1;
        }



};

/**
 * @brief Class that represents the collector node for the pipe, it receives the matrix to which a 
 *        smoothing filter has been applied by the workers of the farm and compare it to background,
 *        counting the different pixels, in case this number exceed the percentage specified by the user
 *        it increments the counter of the different frames
 * 
 */
class ComparerWorker: public ff_minode_t<Mat, float> {
    private: 
        bool show = false;
        bool times = false;
        Mat background; // background matrix
        float threshold; // threshold to consider two pixels different
        float percent; // Percentage of different pixels between frame and background to detect movement
        int different_frames = 0; // number of frame with movement
        bool has_finished = false; // flag true if the collector has received an EOS

        float different_pixels_seq(Mat frame) {
            auto start = std::chrono::high_resolution_clock::now();
            int cnt = 0;
            Mat res = Mat(frame.rows, frame.cols, CV_32F);
            float * pa = (float *) frame.data;
            float * pb = (float *) (this->background).data;
            float * pres = (float *) res.data;
            for(int i=0; i<frame.rows; i++) {
                for (int j=0; j<frame.cols; j++) {
                    pres[i * frame.cols + j] = abs(pb[i * frame.cols + j] - pa[i * frame.cols + j]);
                    if (pres[i * frame.cols + j] > threshold) cnt++;
                }
            }
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Time spent for background subtraction: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Background subtraction", res);
                waitKey(25);
            }
            float diff_fraction = (float) cnt / frame.total();
            return diff_fraction;
        }

    public:

        ComparerWorker(Mat background, float percent, float threshold, bool show, bool times): 
            background(background), percent(percent), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Main function of the collector node, it receives prepared frames, compares them with background and
         *        count the number of frames with movement detected
         * 
         * @param m the matrix received from a smoothing worker of the farm
         */
        float * svc(Mat * m) {
            float * diff = new float(this->different_pixels_seq(*m));
            //if (diff > this->percent) (this->different_frames)++;
            delete m;
            return diff;
        }
};

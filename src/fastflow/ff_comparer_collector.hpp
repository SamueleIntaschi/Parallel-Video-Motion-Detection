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

/**
 * @brief Class that performs the background subtraction using map and reduce over the rows of the matrix,
 *        reduce is for compute the fraction of different pixels over the total
 * 
 */
class Comparer {

    private:
        Mat background;
        Mat frame;
        int nw; // number of workers
        bool show = false;
        bool times = false;
        float threshold; // threshold to consider two pixels different
    
    public:
    
        Comparer(Mat background, Mat frame, int nw, float threshold, bool show, bool times):
            background(background), frame(frame), nw(nw), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Performs background subtraction and count different pixels using map and reduce
         * 
         * @return the fraction of different pixels between background and current frame over the total
         */
        float different_pixels() {
            Mat res = Mat((this->frame).rows, (this->frame).cols, CV_32F);
            float * pa = (float *) (this->frame).data;
            float * pb = (float *) (this->background).data;
            float * pres = (float *) res.data;
            int different_pixels = 0; // number of different pixels between background and current frame
            auto comparing = [&] (int i, int &diff_pixels) {
                // For each pixel of the row increment the counter if the difference is bigger than the threshold
                for (int j=0; j<(this->frame).cols; j++) {
                    pres[i * (this->frame).cols + j] = abs(pb[i * (this->frame).cols + j] - pa[i * (this->frame).cols + j]);
                    if (pres[i * (this->frame).cols + j] > this->threshold) diff_pixels++;
                }
            };
            // lambda to aggregate the number of different pixels in the
            auto reduce = [] (int &diff_pixels, int e) {
                diff_pixels += e;
            };
            auto start = std::chrono::high_resolution_clock::now();
            ParallelForReduce<int> pf(this->nw, true);
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

/**
 * @brief Class that represents the collector node for the pipe, it receives the matrix to which a 
 *        smoothing filter has been applied by the workers of the farm and compare it to background,
 *        counting the different pixels, in case this number exceed the percentage specified by the user
 *        it increments the counter of the different frames
 * 
 */
class ComparerCollector: public ff_minode_t<Mat> {
    private: 
        bool show = false;
        bool times = false;
        Mat background; // background matrix
        float threshold; // threshold to consider two pixels different
        float percent; // Percentage of different pixels between frame and background to detect movement
        int cw; // number of workers for the map reduce
        int different_frames = 0; // number of frame with movement
        int frame_number = 0; // actual frame
        bool has_finished = false; // flag true if the collector has received an EOS

    public:

        ComparerCollector(Mat background, int cw, float percent, float threshold, bool show, bool times): 
            background(background), cw(cw), percent(percent), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Main function of the collector node, it receives prepared frames, compares them with background and
         *        count the number of frames with movement detected
         * 
         * @param m the matrix received from a smoothing worker of the farm
         */
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

        /**
         * @brief function executed when the node has finished its work, it prints a message and update a flag
         * 
         */
        void svc_end() {
            cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
            this -> has_finished = true;
        }

        /**
         * @brief Get the number of frames with movement detected from outside the node if the stream is finished
         * 
         * @return the number of frames with movement detected
         */
        int get_different_frames_number() {
            if (this -> has_finished) return this->different_frames;
            else return -1;
        }
};

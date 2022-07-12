#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;
using namespace std;
using namespace cv;

/**
 * @brief Class representing the collector node for the background subtraction farm, it receives the matrix to which a 
 *        smoothing filter has been applied and compare it to background counting the different pixels. 
 * 
 */
class ComparerWorker: public ff_node_t<Mat, float> {
    private: 
        bool show = false;
        bool times = false;
        Mat background; // background matrix
        float threshold; // threshold to consider two pixels different
        float percent; // Percentage of different pixels between frame and background to detect movement
        int different_frames = 0; // number of frame with movement
        bool has_finished = false; // flag true if the collector has received an EOS

        /**
         * @brief Counts the number of frames between the frame and the background out of the total 
         * 
         * @param frame the frame to compare with background
         * @return float percentage of different pixels out of the total
         */
        float different_pixels(Mat frame) {
            auto start = std::chrono::high_resolution_clock::now();
            int cnt = 0;
            float * pa = (float *) frame.data;
            float * pb = (float *) (this->background).data;
            for(int i=0; i<frame.rows; i++) {
                for (int j=0; j<frame.cols; j++) {
                    float difference = (float) abs(pb[i * frame.cols + j] - pa[i * frame.cols + j]);
                    pa[i * frame.cols + j] = difference;
                    if (difference > threshold) cnt++;
                }
            }
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Time spent for background subtraction: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Background subtraction", frame);
                waitKey(25);
            }
            float diff_fraction = (float) cnt / frame.total();
            return diff_fraction;
        }

    public:

        ComparerWorker(Mat background, float percent, float threshold, bool show, bool times): 
            background(background), percent(percent), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Main function of the comparer node, it receives prepared frames, compares them with background and
         *        count the number of different pixels out of the total
         * 
         * @param m the matrix received from a smoothing worker of the farm
         */
        float * svc(Mat * m) {
            float * diff = new float(this->different_pixels(*m));
            // Delete the matrix when the count is computed
            delete m;
            return diff;
        }
};

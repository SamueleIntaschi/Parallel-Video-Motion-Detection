#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <atomic>

using namespace std;
using namespace cv;

/**
 * @brief Class that performs background subtraction and computes the fraction of
 *        different pixels out of the total
 * 
 */
class Comparer {
    
    private:
        Mat background;
        bool show = false;
        float threshold;
        bool times = false;
    
    public:

        Comparer(Mat background, float threshold, bool show, bool times):
            background(background), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Performs background subtraction
         * 
         * @return the fraction of different pixels between the background and the actual frame over the total
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
};
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
 * @brief Class that performs greyscale conversion
 * 
 */
class GreyscaleConverter {

    private:
        bool show = false;
        bool times = false;

    public:

        GreyscaleConverter(bool show, bool times): show(show), times(times) {}

        /**
         * @brief Get the avg intensity of pixel the black and white matrix
         * 
         * @param bn the matrix of which we want to compute the pixel avg intensity
         * @return float 
         */
        float get_avg_intensity(Mat bn) {
            int channels = bn.channels();
            if (channels > 1) return -1;
            float * p = (float *) bn.data;
            float sum = 0;
            for(int i=0; i<bn.rows; i++) {
                for (int j=0; j<bn.cols; j++) {
                    sum = sum + p[i * bn.cols + j];
                }
            }
            float avg = (float) sum / bn.total();
            avg = round( avg * 100.0 ) / 100.0;
            return avg;
        }

        
        /**
         * @brief Converts a frames in black and white
         * 
         * @return the matrix that represents the frame in greyscale
         */
        Mat convert_to_greyscale(Mat frame) {
            auto start = std::chrono::high_resolution_clock::now();
            Mat gr = Mat(frame.rows, frame.cols, CV_32F);
            float * p = (float *) frame.data;
            float r, g, b;
            float * gp = (float *) gr.data;
            int channels = frame.channels();
            for(int i=0; i<frame.rows; i++) {
                for (int j=0; j<frame.cols; j++) {
                    r = (float) p[i * frame.cols * channels + j * channels];
                    g = (float) p[i * frame.cols * channels + j * channels + 1];
                    b = (float) p[i * frame.cols * channels + j * channels + 2];
                    gp[i* frame.cols + j ] = (float) (r + g + b) / channels;
                }
            }
            if (this->times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times spent to convert to greyscale: " << usec << " usec" << endl;
            }
            if (this->show) {
                imshow("Frame", gr);
                waitKey(25);
            }
            return gr;
        }

};
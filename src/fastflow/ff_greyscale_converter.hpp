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

using namespace std;
using namespace cv;
using namespace ff;

/**
 * @brief Class used to convert a frame to greyscale using parallel_for, it is used
 *        only for the background because in the pipe it is better to use a Map node
 * 
 */
class GreyscaleConverter {

    private:
        Mat m;
        int nw;
        bool show = false;
        bool times = false;

    public:

        GreyscaleConverter(Mat m, int nw, bool show, bool times):
            m(m), nw(nw), show(show), times(times) {}

        float get_avg_intensity(Mat bn) {
            int channels = bn.channels();
            if (channels > 1) return -1;
            float * p = (float *) bn.data;
            float sum = 0;
            auto f = [&] (const int i, float &s) {
                for (int j=0; j<bn.cols; j++) {
                    s = (float) s + p[i * bn.cols + j];
                }
            }; 
            auto reduce = [] (float &s, float e) {
                s += e;
            };
            ParallelForReduce<float> pf(this->nw, true);
            pf.parallel_reduce(sum, 0, 0, bn.rows, 1, f, reduce, nw);
            float avg = (float) sum / bn.total();
            avg = round( avg * 100.0 ) / 100.0;
            return avg;
        }

        Mat convert_to_greyscale() {
            Mat gr = Mat((this -> m).rows, (this -> m).cols, CV_32F);
            int channels = (this -> m).channels();
            float * pl = (float *) (gr).data;
            float * pm = (float *) (this -> m).data;

            auto greyscale_conversion = [&gr, this, channels, pl, pm] (const long i) {
                float r = 0;
                float b = 0;
                float g = 0;
                for (int j=0; j<gr.cols; j++) {
                    r = (float) pm[i * gr.cols * channels + j * channels];
                    g = (float) pm[i * gr.cols * channels + j * channels + 1];
                    b = (float) pm[i * gr.cols * channels + j * channels + 2];
                    pl[i * gr.cols + j] = (float) (r + g + b) / channels;
                }
            };
            auto start = std::chrono::high_resolution_clock::now();
            ParallelFor pf(nw, true);
            pf.parallel_for(0,(this->m).rows, 1, 0, greyscale_conversion, nw);
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Frame", gr);
                waitKey(25);
            }
            return gr;
        }
};
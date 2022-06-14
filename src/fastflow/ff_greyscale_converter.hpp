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

class GreyscaleConverter {

    private:
        Mat m;
        int nw;
        bool show = false;

    public:

        GreyscaleConverter(Mat m, int nw, bool show) {
            this -> m = m;
            this -> nw = nw;
            this -> show = show;
        }

        float get_avg_intensity(Mat bn) {
            int channels = bn.channels();
            if (channels > 1) return -1;
            float * p = (float *) bn.data;
            float sum = {0.0};
            auto f = [&] (const int i, float &s) {
                for (int j=0; j<(this->m).cols; j++) {
                    s = s + p[i * m.cols + j];
                }
            }; 
            auto reduce = [] (float &s, float e) {
                s += e;
            };
            ParallelForReduce<float> pf(this->nw, true);
            pf.parallel_reduce(sum, 0, 0, bn.rows, 1, f, reduce, nw);
            float avg = sum / bn.total();
            return avg;
        }

        Mat * convert_to_greyscale() {
            auto start = std::chrono::high_resolution_clock::now();

            Mat * gr = new Mat((this -> m).rows, (this -> m).cols, CV_32F);
            int channels = (this -> m).channels();
            float * pl = (float *) (*gr).data;
            float * pm = (float *) (this -> m).data;

            auto greyscale_conversion = [&gr, this, channels, pl, pm] (const int i) {
                float r = 0;
                float b = 0;
                float g = 0;
                for (int j=0; j<(this->m).cols; j++) {
                    r = (float) pm[i * m.cols * channels + j * channels];
                    g = (float) pm[i * m.cols * channels + j * channels + 1];
                    b = (float) pm[i * m.cols * channels + j * channels + 2];
                    pl[i * m.cols + j] = (float) (r + g + b) / channels;
                }
                return;
            };
            
            ParallelFor pf(nw,true);
            pf.parallel_for(1,(this->m).rows, 1, 0, greyscale_conversion, nw);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
            if (show) {
                imshow("Frame", *gr);
                waitKey(25);
            }
            return gr;
        }
};
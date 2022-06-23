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

class Smoother {
    private:
        Mat m;
        Mat filter;
        vector<thread> tids;
        bool show = false;
        bool times = false;

        /**
         * @brief Multiply two matrixes pixel per pixel
         *
         * @param a Mat to multiply
         * @param b Mat to multiply
         * @return A matrix where the pixel i,j is the multiplication of pixel i,j of a and pixel i,j of b.
         */
        Mat pixel_mul(Mat a, Mat b) {
            Mat res = Mat(a.rows, a.cols, CV_32F);
            float * pa = (float *) a.data;
            float * pb = (float *) b.data;
            float * pres = (float *) res.data;
            for(int i=0; i<a.rows; i++) {
                for (int j=0; j<a.cols; j++) {
                    pres[i * a.cols + j] = pa[i * a.cols + j] * pb[i * a.cols + j];
                }
            }
            return res;
        }

        /**
         * @brief Computes a pixel of the matrix with filter applied.
         * 
         * @param sub matrix to apply the filter to
         * @param filter filter matrix
         * @return The central pixel of the matrix with filter applied.
         */
        float smoothing_px(Mat sub, Mat filter) {
            Mat mul = pixel_mul(sub, filter);
            float * p = (float *) mul.data;
            float res = 0;
            for(int i=0; i<mul.rows; i++) {
                for (int j=0; j<mul.cols; j++) {
                    res += p[i * mul.cols + j];
                }
            }
            return res;
        }

    public:
        Smoother(Mat m, Mat filter, bool show, bool times): m(m), filter(filter), show(show), times(times) {}

        /**
         * @brief Performs smoothing of a matrix given a filter.
         * 
         * @return The matrix with smoothing filter applied
         */
        Mat smoothing() {
            auto start = std::chrono::high_resolution_clock::now();
            float * sp = (float *) (this -> m).data;
            for(int i=0; i<(this->m).rows; i++) {
                for (int j=0; j<(this->m).cols; j++) { 
                    cv::Rect r(j-1, i-1, 3, 3);
                    if (r.x >= 0 && r.y >= 0 && r.x + r.width <= (this->m).cols && r.y + r.height <= (this->m).rows) {
                        Mat submatrix = (this->m)(r).clone();
                        float val = sp[i*m.cols + j];
                        sp[i * m.cols + j] = smoothing_px(submatrix, (this->filter));
                    }
                }
            }
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times passed for smoothing: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Smoothing", (this -> m));
                waitKey(25);
            }
            return (this -> m);
        }
};
#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>

#include "../utils/smoother.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace std;
using namespace cv;

/**
 * @brief Class that represents a worker that performs smoothing on the given matrix
 * 
 */
class SmoothingWorker: public ff_monode_t<Mat> {
    private:
        bool show = false;
        bool times = false;
        Mat filter; // filter matrix
        int nw = 0;

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
        SmoothingWorker(int nw, Mat filter, bool show, bool times): filter(filter), show(show), times(times), nw(nw) {}

        Mat smoothing(Mat m) {
            auto start = std::chrono::high_resolution_clock::now();
            vector<pair<int,int>> chunks;
            float * sp = (float *) m.data;
            Mat res = Mat(m.rows, m.cols, CV_32F);
            float * mp = (float *) res.data;
            auto smoothing = [&] (const long i) {
                for (int j=0; j<res.cols; j++) {
                    cv::Rect r(j-1, i-1, 3, 3);
                    if (r.x >= 0 && r.y >= 0 && r.x + r.width < res.cols && r.y + r.height < res.rows) {
                        Mat submatrix = m(r);
                        mp[i * res.cols + j] = smoothing_px(submatrix, (this->filter));
                    }
                    else {                            
                        mp[i * res.cols + j] = sp[i * res.cols + j];
                    }
                }
            };
            ff::parallel_for(0,m.rows,1,0,smoothing,nw);
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times spent for smoothing: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Smoothing", res);
                waitKey(25);
            }
            return res;
        }

        /**
         * @brief Main function of the node, it performs smoothing on the given matrix and submits the result 
         *        to the next node
         * 
         * @param m matrix on which perform smoothing
         * @return matrix on which the filter is applied
         */
        Mat * svc(Mat * m) {
            
            if (m != EOS) {
                /*
                // Use the smoother object to perform smoothing
                Smoother s(*m, this->filter, this->show, this->times);
                *m = s.smoothing();
                return(m);
                */
                *m = this->smoothing(*m);
                
            }
            
            return m;
        }
};
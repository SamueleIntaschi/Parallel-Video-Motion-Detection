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
        Mat filter;
        bool show = false;
        bool times = false;
        int nw = 0;
        vector<chrono::microseconds> usecs; 

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
        Smoother(Mat filter, int nw, bool show, bool times): filter(filter), show(show), times(times), nw(nw) {}

        /**
         * @brief Performs smoothing of a matrix given a filter.
         * 
         * @param m the matrix on which applying the filter
         * @return The matrix with smoothing filter applied
         */
        Mat smoothing(Mat m) {
            auto start = std::chrono::high_resolution_clock::now();
            vector<pair<int,int>> chunks;
            float * sp = (float *) m.data;
            Mat res = Mat(m.rows, m.cols, CV_32F);
            float * mp = (float *) res.data;
            int chunk_rows = m.rows / nw;
            for (int i=0; i<nw; i++) {
                auto start = i*chunk_rows;
                auto stop = (i==(nw-1) ? m.rows : start + chunk_rows);
                chunks.push_back(make_pair(start,stop));
            }
            vector<thread> tids(this->nw);
            auto f = [&] (int ti) {
                int first = chunks[ti].first;
                int final = chunks[ti].second;
                for(int i=first; i<final; i++) {
                    for (int j=0; j<m.cols; j++) {
                        cv::Rect r(j-1, i-1, 3, 3);
                        if (r.x >= 0 && r.y >= 0 && r.x + r.width < m.cols && r.y + r.height < m.rows) {
                            Mat submatrix = m(r);
                            mp[i * res.cols + j] = smoothing_px(submatrix, (this->filter));
                        }
                        // Do not change the values at first and last column or row
                        else {                            
                            mp[i * res.cols + j] = sp[i * res.cols + j];
                        }
                    }
                }
            };
            // Start the thread
            for (int i=0; i<(this->nw); i++) {
                tids[i] = thread(f, i);
            }
            // Wait the threads
            for (int i=0; i<(this->nw); i++) {
                tids[i].join();
            }
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            this->usecs.push_back(chrono::microseconds(usec));
            if (times) cout << "Times spent for smoothing: " << usec << " usec" << endl;
            if (show) {
                imshow("Smoothing", res);
                waitKey(25);
            }
            return res;
        }

        auto get_avg_time() {
            int size = (this->usecs).size();
            chrono::microseconds sum = 0ms;
            for (int i=0; i<size; i++) {
                sum += this->usecs[i];
            }
            return (chrono::microseconds) sum/size;
        }
};
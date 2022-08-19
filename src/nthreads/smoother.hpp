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
        bool show = false;
        bool times = false;
        vector<chrono::microseconds> usecs; 

    public:
        Smoother(bool show, bool times): show(show), times(times) {}

        /**
         * @brief Performs smoothing of a matrix given a filter.
         * 
         * @param m the matrix on which applying the filter
         * @return The matrix with smoothing filter applied
         */
        Mat * smoothing(Mat * m) {
            auto start = std::chrono::high_resolution_clock::now();
            Mat * res = new Mat(m->rows, m->cols, CV_32F, 0.0);
            float * sp = (float *) m->data;
            float * mp = (float *) res->data;
            for(int i=i; i<m->rows; i++) {
                for (int j=0; j<m->cols; j++) {
                    int x = j-1;
                    int y = i-1;
                    int width = 3;
                    int height = 3;
                    // Check that the bounds of the 3x3 kernel do not exceed the frame size
                    if (x >= 0 && y >= 0 && x + width < m->cols && y + height < m->rows) {
                        for(int z=y; z<y+height; z++) {
                            for (int k=x; k<x+width; k++) {
                                mp[i * res->cols + j] = mp[i * res->cols + j] + (float) (sp[z * m->cols + k] / 9);
                            }
                        }
                    }
                    // Do not change the values at first and last column or row
                    else {                            
                        mp[i * res->cols + j] = sp[i * res->cols + j];
                    } 
                }
            }
            delete m;
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times spent for smoothing: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Smoothing", *res);
                waitKey(25);
            }
            return res;
        }
};
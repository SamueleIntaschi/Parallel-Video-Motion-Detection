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
        int nw = 0;
        vector<chrono::microseconds> usecs; 

    public:
        Smoother(int nw, bool show, bool times): show(show), times(times), nw(nw) {}

        /**
         * @brief Performs smoothing of a matrix given a filter.
         * 
         * @param m the matrix on which applying the filter
         * @return The matrix with smoothing filter applied
         */
        Mat smoothing(Mat m) {
            auto start = std::chrono::high_resolution_clock::now();
            vector<pair<int,int>> chunks;
            Mat res = Mat(m.rows, m.cols, CV_32F, 0.0);
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
                float * sp = (float *) m.data;
                float * mp = (float *) res.data;
                for(int i=first; i<final; i++) {
                    for (int j=0; j<m.cols; j++) {
                        int x = j-1;
                        int y = i-1;
                        int width = 3;
                        int height = 3;
                        // Check that the bounds of the 3x3 kernel do not exceed the frame size
                        if (x >= 0 && y >= 0 && x + width < m.cols && y + height < m.rows) {
                            for(int z=y; z<y+height; z++) {
                                for (int k=x; k<x+width; k++) {
                                    mp[i * res.cols + j] = mp[i * res.cols + j] + (float) (sp[z * m.cols + k] / 9);
                                }
                            }
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
};
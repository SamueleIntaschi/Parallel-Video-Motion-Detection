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

//TODO rivedere sottrazione matrici

class Comparer {
    private:
        Mat background;
        Mat frame;
        int nw;
        vector<pair<int,int>> chunks;
        bool show = false;
    
    public:
        Comparer(Mat b, Mat f, int nw, bool show) {
            this -> background = b;
            this -> frame = f;
            this -> nw = nw;
            this -> show = show;
            int chunk_rows = (this->frame).rows/nw;
            for (int i=0; i<nw; i++) {
                auto start = i * chunk_rows;
                auto stop = (i==(nw-1) ? (this->frame).rows : start - chunk_rows);
                chunks.push_back(make_pair(start, stop));
            }
        }

        float different_pixels() {
            auto start = std::chrono::high_resolution_clock::now();
            vector<thread> tids(nw);
            atomic<int> different_pixels;
            different_pixels = 0;
            Mat res = Mat((this->frame).rows, (this->frame).cols, CV_32F);
            float * pa = (float *) (this->frame).data;
            float * pb = (float *) (this->background).data;
            float * pres = (float *) res.data;
            auto comparing = [&] (int ti) {
                int first = (this->chunks)[ti].first;
                int final = (this->chunks)[ti].second;
                for(int i=first; i<final; i++) {
                    for (int j=0; j<(this->frame).cols; j++) {
                        pres[i * (this->frame).cols + j] = abs(pb[i * (this->frame).cols + j] - pa[i * (this->frame).cols + j]);
                        if (pres[i * (this->frame).cols + j] > 0) different_pixels++;
                    }
                }

            };
            for (int i=0; i<(this->nw); i++) {
                tids[i] = thread(comparing, i);
            }
            for (int i=0; i<(this->nw); i++) {
                tids[i].join();
            }
            if (show) {
                imshow("Background Subtraction", res);
                waitKey(25);
            }
            float diff_pixels_fraction = (float) different_pixels/(this->frame).total();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cout << "Times passed to compare frame with background: " << usec << " usec" << endl;
            return diff_pixels_fraction;
        }
};
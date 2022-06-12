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

class GreyscaleConverter {

    private:
        Mat m;
        int nw;
        vector<pair<int,int>> chunks;
        int chunk_rows;
        bool show = false;

    public:

        GreyscaleConverter(Mat m, int nw, bool show) {
            this -> m = m;
            this -> nw = nw;
            this -> show = show;
            this -> chunk_rows = m.rows / nw;            
            for (int i=0; i<nw; i++) {
                auto start = i*chunk_rows;
                auto stop = (i==(nw-1) ? m.rows : start + chunk_rows);
                chunks.push_back(make_pair(start,stop));
            }
        }

        Mat convert_to_greyscale() {
            Mat gr = Mat((this -> m).rows, (this -> m).cols, CV_32F);
            vector<thread> tids(this -> nw);
            auto greyscale_conversion = [&gr, this] (int ti) {
                int first = (this -> chunks[ti]).first;
                int final = (this -> chunks[ti]).second;
                //cout << "Thread " << ti << " takes rows from " << first << " to " << final << endl;
                float * pl = (float *) gr.data;
                float * pm = (float *) (this -> m).data;
                float r = 0;
                float b = 0;
                float g = 0;
                int channels = (this -> m).channels();
                for(int i=first; i<final; i++) {
                    for (int j=0; j<gr.cols; j++) {
                        r = (float) pm[i * gr.cols * channels + j * channels];
                        g = (float) pm[i * gr.cols * channels + j * channels + 1];
                        b = (float) pm[i * gr.cols * channels + j * channels + 2];
                        pl[i * gr.cols + j] = (float) (r + g + b) / channels;
                    }
                }
                return;
            };
            auto start = std::chrono::high_resolution_clock::now();
            for (int i=0; i<(this->nw); i++) {
                tids[i] = thread(greyscale_conversion, i);
            }
            for (int i=0; i<(this->nw); i++) {
                tids[i].join();
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
            if (show) {
                imshow("Frame", gr);
                waitKey(25);
            }
            return gr;
        }
};
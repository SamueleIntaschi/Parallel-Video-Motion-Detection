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
        Mat m;
        int nw; // number of workers
        vector<pair<int,int>> chunks; // vector of pair <first_row, last_row> with the range of rows each thread has to analyse
        bool show = false;
        bool times = false;

    public:

        GreyscaleConverter(Mat m, int nw, bool show, bool times): m(m), nw(nw), show(show), times(times) {
            int chunk_rows = m.rows / nw;    
            // Compute the range of rows each thread has to analyze        
            for (int i=0; i<nw; i++) {
                auto start = i*chunk_rows;
                auto stop = (i==(nw-1) ? m.rows : start + chunk_rows);
                chunks.push_back(make_pair(start,stop));
            }
        }

        /**
         * @brief Get the avg intensity of pixel the black and white matrix
         * 
         * @param bn the matrix of which we want to compute the pixel avg intensity
         * @return float 
         */
        float get_avg_intensity(Mat bn) {
            int channels = bn.channels();
            if (channels > 1) return -1;
            mutex l;
            float * p = (float *) bn.data;
            float sum = 0;
            vector<thread> tids(this -> nw);
            auto f = [&] (int ti) {
                int first = (this -> chunks[ti]).first;
                int final = (this -> chunks[ti]).second;
                for(int i=first; i<final; i++) {
                    for (int j=0; j<bn.cols; j++) {
                        {
                            unique_lock<mutex> lock(l);
                            sum = sum + p[i * bn.cols + j];
                        }
                    }
                }
            };
            for (int i=0; i<(this->nw); i++) {
                tids[i] = thread(f, i);
            }
            for (int i=0; i<(this->nw); i++) {
                tids[i].join();
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
        Mat convert_to_greyscale() {
            Mat gr = Mat((this -> m).rows, (this -> m).cols, CV_32F);
            vector<thread> tids(this -> nw);
            auto greyscale_conversion = [&gr, this] (int ti) {
                // Get the first and final row to analyze for this thread
                int first = (this -> chunks[ti]).first;
                int final = (this -> chunks[ti]).second;
                float * pl = (float *) gr.data;
                float * pm = (float *) (this -> m).data;
                float r = 0;
                float b = 0;
                float g = 0;
                int channels = (this -> m).channels();
                for(int i=first; i<final; i++) {
                    for (int j=0; j<gr.cols; j++) {
                        // Compute the average value of the three colors
                        r = (float) pm[i * gr.cols * channels + j * channels];
                        g = (float) pm[i * gr.cols * channels + j * channels + 1];
                        b = (float) pm[i * gr.cols * channels + j * channels + 2];
                        pl[i * gr.cols + j] = (float) (r + g + b) / channels;
                    }
                }
                return;
            };
            auto start = std::chrono::high_resolution_clock::now();
            // Start the thread
            for (int i=0; i<(this->nw); i++) {
                tids[i] = thread(greyscale_conversion, i);
            }
            // Wait the threads
            for (int i=0; i<(this->nw); i++) {
                tids[i].join();
            }
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
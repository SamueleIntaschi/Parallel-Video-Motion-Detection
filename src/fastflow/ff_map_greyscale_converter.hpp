#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

class Converter: public ff_Map<Mat> {

    private:
        int cw;
        string filename;
        int frame_number = 0;
        bool show = false;
        bool times = false;
        VideoCapture cap;

    public:

        Converter(VideoCapture cap, int cw, bool show, bool times): cap(cap), cw(cw), show(show), times(times) {}

        Mat * svc (Mat *) {

            while (true) {

                Mat frame; 
                this -> cap >> frame;
                if (frame.empty()) break;
                frame.convertTo(frame, CV_32F, 1.0/255.0);

                Mat * m = new Mat(frame.rows, frame.cols, CV_32F);

                Mat gr = Mat(frame.rows, frame.cols, CV_32F);
                int channels = frame.channels();
                float * pl = (float *) (gr).data;
                float * pm = (float *) frame.data;

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
                ff_Map::parallel_for(0,frame.rows,1,0,greyscale_conversion,cw);
                if (times) {
                    auto duration = std::chrono::high_resolution_clock::now() - start;
                    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                    cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
                }
                if (show) {
                    imshow("Frame", gr);
                    waitKey(25);
                }

                *m = gr;
                ff_send_out(m);
                frame_number++;

            }
            (this->cap).release();
            return EOS;
        }
};
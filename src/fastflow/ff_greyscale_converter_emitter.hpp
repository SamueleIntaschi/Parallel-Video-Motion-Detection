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

/**
 * @brief Class that represents the emitter of the pipe, that reads frames from the source,
 *        converts them in grayscale and send it to the next node of the pipe, that performs 
 *        smoothing
 * 
 */
class ConverterEmitter: public ff_monode_t<Mat> {

    private:
        int cw; // number of workers for map
        string filename; // name of the video to analyze
        int frame_number = 0;
        bool show = false;
        bool times = false;
        VideoCapture cap;

    public:

        ConverterEmitter(VideoCapture cap, int cw, bool show, bool times): cap(cap), cw(cw), show(show), times(times) {}

        /**
         * @brief Main function of the emitter node, it reads frames, converts to grayscale and submits to next node
         * 
         * @return EOS when the frames are finished
         */
        Mat * svc (Mat *) {

            while (true) {
                
                auto start = std::chrono::high_resolution_clock::now();

                Mat frame; 
                this -> cap >> frame;
                if (frame.empty()) break;
                frame.convertTo(frame, CV_32F, 1.0/255.0);
                Mat * m = new Mat(frame.rows, frame.cols, CV_32F);
                Mat gr = Mat(frame.rows, frame.cols, CV_32F);
                int channels = frame.channels();
                float * pl = (float *) (gr).data;
                float * pm = (float *) frame.data;

                // Map function for rows
                auto greyscale_conversion = [&gr, this, channels, pl, pm] (const long i) {
                    float r = 0;
                    float b = 0;
                    float g = 0;
                    // For each pixel of the row compute the average value of the channels
                    for (int j=0; j<gr.cols; j++) {
                        r = (float) pm[i * gr.cols * channels + j * channels];
                        g = (float) pm[i * gr.cols * channels + j * channels + 1];
                        b = (float) pm[i * gr.cols * channels + j * channels + 2];
                        pl[i * gr.cols + j] = (float) (r + g + b) / channels;
                    }
                };
                // Compute the parallel_for
                ff::parallel_for(0,frame.rows,1,0,greyscale_conversion,cw);
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
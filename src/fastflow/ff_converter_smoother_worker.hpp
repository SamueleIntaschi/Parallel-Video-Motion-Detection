#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace std;
using namespace cv;

/**
 * @brief Class representing a worker that performs grayscale conversion and smoothing on a given frame
 * 
 */
class SmootherConverterWorker: public ff_Map<Mat,Mat> {
    private:
        bool show = false;
        bool times = false;
        int nw = 0;

    public:
        SmootherConverterWorker(int nw, bool show, bool times): show(show), times(times), nw(nw) {}

        /**
         * @brief Converts a 3-channel frame in grayscale
         * 
         * @param frame The frame to convert
         * @return Mat* The pointer to the frame converted in grayscale
         */
        Mat * convert_to_greyscale(Mat * frame) {
            auto start = std::chrono::high_resolution_clock::now();
            Mat * gr = new Mat(frame->rows, frame->cols, CV_32F);
            // Map function for rows
            auto greyscale_conversion = [&] (const long i) {
                float r = 0;
                float b = 0;
                float g = 0;
                float * pl = (float *) gr->data;
                float * pm = (float *) frame->data;
                int channels = frame->channels();
                // For each pixel of the row compute the average value of the channels
                for (int j=0; j<(*gr).cols; j++) {
                    r = (float) pm[i * gr->cols * channels + j * channels];
                    g = (float) pm[i * gr->cols * channels + j * channels + 1];
                    b = (float) pm[i * gr->cols * channels + j * channels + 2];
                    pl[i * gr->cols + j] = (float) (r + g + b) / channels;
                }
            };
            // Compute the parallel_for
            ff_Map::parallel_for(0, frame->rows, 1, 0, greyscale_conversion, nw);
            if (this->times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times passed to convert to greyscale: " << usec << " usec" << endl;
            }
            if (this->show) {
                imshow("Frame", *gr);
                waitKey(25);
            }
            delete frame;
            return gr;
        }

        /**
         * @brief Applies an average 3x3 filter on a grayscale frame
         * 
         * @param m The frame on which apply the filter
         * @return Mat* The pointer to the frame with filter applied
         */
        Mat * smoothing(Mat * m) {
            auto start = std::chrono::high_resolution_clock::now();
            Mat * res = new Mat(m->rows, m->cols, CV_32F, 0.0);
            // Performs the smoothing on the rows of the matrix
            auto smoothing = [&] (const long i) {
                float * sp = (float *) m->data;
                float * mp = (float *) res->data;
                for (int j=0; j<res->cols; j++) {
                    int x = j-1;
                    int y = i-1;
                    int width = 3;
                    int height = 3;
                    // Check that the bounds of the 3x3 kernel do not exceed the frame size
                    if (x >= 0 && y >= 0 && x + width < m->cols && y + height < m->rows) {
                        for(int z=y; z<y+height; z++) {
                            for (int k=x; k<x+width; k++) {
                                // Compute the average value of the 3x3 neighborhood of the pixel
                                mp[i * res->cols + j] = mp[i * res->cols + j] + (float) (sp[z * m->cols + k] / 9);
                            }
                        }
                    }
                    else {                            
                        mp[i * res->cols + j] = sp[i * res->cols + j];
                    }
                }
            };
            ff_Map::parallel_for(0, m->rows, 1, 0, smoothing, nw);
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times spent for smoothing: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Smoothing", *res);
                waitKey(25);
            }
            delete m;
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
                
                // Grayscale conversion
                Mat * gr = this->convert_to_greyscale(m);
                // Smoothing
                m = this->smoothing(gr);
            }
            
            return m;
        }
};
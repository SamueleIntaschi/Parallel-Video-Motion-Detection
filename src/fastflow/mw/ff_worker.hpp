#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>

using namespace ff;
using namespace std;
using namespace cv;

struct Task {  
    Mat * m;
    float n;
};

/**
 * @brief Class that represents a worker node, it receives task from the master, compute them and
 *        sends back the results
 * 
 */
class Worker: public ff_node_t<Task> {

    private:
        bool show = false;
        bool times = false;
        Mat background; // background matrix
        float threshold; // threshold to consider two pixels different

        /**
         * @brief Converts a frames in black and white
         * 
         * @return a pointer to the matrix that represents the frame in greyscale
         */
        Mat * convert_to_greyscale(Mat * m) {
            auto start = std::chrono::high_resolution_clock::now();
            Mat * gr = new Mat(m->rows, m->cols, CV_32F);
            int channels = m->channels();
            float * pl = (float *) gr->data;
            float * pm = (float *) m->data;
            float r = 0;
            float b = 0;
            float g = 0;
            for (int i=0; i<gr->rows; i++) {
                for (int j=0; j<gr->cols; j++) {
                    r = (float) pm[i * gr->cols * channels + j * channels];
                    g = (float) pm[i * gr->cols * channels + j * channels + 1];
                    b = (float) pm[i * gr->cols * channels + j * channels + 2];
                    pl[i * gr->cols + j] = (float) (r + g + b) / channels;
                }
            }
            delete m;
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Times spent on grayscale conversion: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Frame", *gr);
                waitKey(25);
            }
            return gr;
        }

        /**
         * @brief Performs smoothing of a matrix given a filter.
         * 
         * @param m the matrix on which applying the filter
         * @return a pointer to the matrix with smoothing filter applied
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

        /**
         * @brief Counts the number of frames between the frame and the background out of the total 
         * 
         * @param frame the frame to compare with background
         * @return float percentage of different pixels out of the total
         */
        float different_pixels(Mat * frame) {
            auto start = std::chrono::high_resolution_clock::now();
            int cnt = 0;
            float * pa = (float *) frame->data;
            float * pb = (float *) (this->background).data;
            for(int i=0; i<frame->rows; i++) {
                for (int j=0; j<frame->cols; j++) {
                    float difference = (float) abs(pb[i * frame->cols + j] - pa[i * frame->cols + j]);
                    pa[i * frame->cols + j] = difference;
                    if (difference > threshold) cnt++;
                }
            }
            if (times) {
                auto duration = std::chrono::high_resolution_clock::now() - start;
                auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
                cout << "Time spent for background subtraction: " << usec << " usec" << endl;
            }
            if (show) {
                imshow("Background subtraction", *frame);
                waitKey(25);
            }
            float diff_fraction = (float) cnt / frame->total();
            delete frame;
            return diff_fraction;
        }

    public:
        Worker(Mat background, float threshold, bool show, bool times): background(background), threshold(threshold), show(show), times(times) {}

        /**
         * @brief Main function of the node, it performs smoothing on the given matrix and submits the result 
         *        to the next node
         * 
         * @param t task to execute
         * @return next task to be computed
         */
        Task * svc(Task * t) {
            // Selects what to do depending on the code received
            if (t -> n == 2) { // Case grayscale conversion
                t->m = this->convert_to_greyscale(t->m);
                // Updates task code
                t->n = 3;
            }
            else if (t -> n == 3) { // Case smoothing
                t->m = this->smoothing(t->m);
                // Updates task code
                t->n = 4;
            }
            else if (t -> n == 4) { // Case background subtraction
                // Uses task code to communicate the result
                t->n = this->different_pixels(t->m);
            }
            return t;
        }

};
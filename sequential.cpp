#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 

using namespace std;
using namespace cv;

// compile with (maybe some flags are missing): g++ program.cpp -o program `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

int evaluate_different_pixels(Mat m, Mat t) {
    int cnt = 0;
    float * pt = (float *) t.data;
    float * pm = (float *) m.data;
    for(int i=0; i<t.rows; i++) {
        for (int j=0; j<t.cols; j++) {
            if (pm[i * t.cols + j] > pt[i * t.cols + j]) cnt++;
        }
    }
    return cnt;
}

int non_zero_pixels(Mat a) {
    int cnt = 0;
    float * p = (float *) a.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            if (p[i * a.cols + j] > 0) cnt++;
        }
    }
    return cnt;
}

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

float smoothing_px(Mat sub, Mat h1) {
    //Mat mul = sub.mul(h1);
    Mat mul = pixel_mul(sub, h1);
    float * p = (float *) mul.data;
    float res = 0;
    for(int i=0; i<mul.rows; i++) {
        for (int j=0; j<mul.cols; j++) {
            res += p[i * mul.cols + j];
        }
    }
    return res;
}

int subtract(Mat a, Mat b) {
    int cnt = 0;
    Mat res = Mat(a.rows, a.cols, CV_32F);
    float * pa = (float *) a.data;
    float * pb = (float *) b.data;
    float * pres = (float *) res.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            //if (pa[i * a.cols + j] < pb[i * a.cols + j]) {
            //    pres[i * a.cols + j] = 0;
            //}
            //else {
            //cout << pa[i * a.cols + j] << " " << pb[i * a.cols + j] << endl;
            pres[i * a.cols + j] = abs(pa[i * a.cols + j] - pb[i * a.cols + j]);
            if (pres[i * a.cols + j] > 0) cnt++;
            //}
        }
    }
    //cout << res << endl;
    return cnt;
}

// Sequential pattern
int main(int argc, char * argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename threshold" << endl;
    }

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // Initialization
    VideoCapture cap(argv[1]);
    int k = atoi(argv[2]); // k for accuracy then
    float threshold = (float) k / 100;
    // Matrix for smoothing
    Mat h1 = Mat::ones(3, 3, CV_32F);
    h1 = (Mat_<float>) (1/9 * h1);
    float * h1p = (float *) h1.data;
    for (int i=0; i<h1.rows; i++) {
        for (int j=0; j<h1.rows; j++) {
            *h1p++ = (float) 1/9;
        }
    }
    int frame_number = 0;
    int different_frames = 0;
    // Matrixes for make comparison between previous and current frame
    Mat background; // The first frame is used as background image
    while(true) {
        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        // Greyscale conversion
        int channels = frame.channels();
        int rows = frame.rows;
        int columns = frame.cols;
        Mat greyscale = Mat(rows, columns, CV_32F);
        float * p = (float *) frame.data;
        float r, g, b;
        float * gp = (float *) greyscale.data;
        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<rows; i++) {
            for (int j=0; j<columns; j++) {
                r = (float) p[i * columns * channels + j * channels];
                g = (float) p[i * columns * channels + j * channels + 1];
                b = (float) p[i * columns * channels + j * channels + 2];
                *gp++ = (float) (r + g + b) / channels;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed in seconds to convert to greyscale: " << usec << endl;

        // Smoothing
        float * sp = (float *) greyscale.data;
        start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<greyscale.rows; i++) {
            for (int j=0; j<greyscale.cols; j++) { 
                cv::Rect r(j-1, i-1, 3, 3);
                if (r.x >= 0 && r.y >= 0 && r.x + r.width <= greyscale.cols && r.y + r.height <= greyscale.rows) {
                    Mat submatrix = greyscale(r).clone();
                    //*sp++ = smoothing_px(submatrix, h1);
                    sp[i * columns + j] = smoothing_px(submatrix, h1);
                }
                /*
                else {

                    sp[i * columns + j] = greyscale.at<float>(i,j);
                    //*sp++ = greyscale.at<float>(i,j);
                }*/
            }
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::high_resolution_clock::now() - start;
        usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        cout << "Times passed in seconds for smoothing: " << usec << endl;
        //imshow("Smoothing", greyscale);
        //waitKey(25);

        // Movement detection
        if (frame_number == 0) { // Case first frame taken as background
            background = greyscale;
        }
        else { // Case movement detection
            int different_pixels = 0;
            //t = (double)getTickCount();
            start = std::chrono::high_resolution_clock::now();
            //if (frame_number > 0) {
            different_pixels = subtract(greyscale, background);
            //imshow("Smoothed", subtraction);
            //waitKey(25);
            //float total_pixels = greyscale.total();
            //float different_pixels_fraction  = (float) non_zero_pixels(subtraction) / total_pixels;
            //if (different_pixels_fraction > accuracy) different_frames++;
            //}
            float different_pixels_fraction = (float)different_pixels/greyscale.total();
            cout << different_pixels_fraction << " " << threshold << endl;
            if (different_pixels_fraction > threshold) different_frames++;
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::high_resolution_clock::now() - start;
            usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            cout << "Times passed in seconds to compare frame with background: " << usec << endl;
            cout << "Frame number " << frame_number << " -> number of frames with movement detected until now: " << different_frames << endl;
        }
        frame_number++;
    }
    cout << "Number of frames with movement detected: " << different_frames << endl;
    cap.release();
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;
    return(0);
}
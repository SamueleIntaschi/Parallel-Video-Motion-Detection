#include <iostream>
#include <fstream>
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



Mat smoothing(Mat m, Mat filter, bool show) {
    cout << "Smoothing" << endl;
    float * gp = (float *) m.data;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<m.rows; i++) {
        for (int j=0; j<m.cols; j++) { 
            cv::Rect r(j-1, i-1, 3, 3);
            if (r.x >= 0 && r.y >= 0 && r.x + r.width <= m.cols && r.y + r.height <= m.rows) {
                Mat submatrix = m(r).clone();
                //*sp++ = smoothing_px(submatrix, h1);
                gp[i * m.cols + j] = smoothing_px(submatrix, filter);
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    cout << "Times passed in seconds for smoothing: " << usec << endl;
    if (show) {
        imshow("Smoothing", m);
        waitKey(25);
    }
    return m;
}

Mat greyscale_conversion(Mat frame, bool show) {
    //TODO there is a segmentation fault here
    cout << "Conversion" << endl;
    Mat gr = Mat(frame.rows, frame.cols, CV_32F);
    float * p = (float *) frame.data;
    float r, g, b;
    float * gp = (float *) gr.data;
    int channels = frame.channels();
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<frame.rows; i++) {
        for (int j=0; j<frame.cols; j++) {
            r = (float) p[i * frame.cols * channels + j * channels];
            g = (float) p[i * frame.cols * channels + j * channels + 1];
            b = (float) p[i * frame.cols * channels + j * channels + 2];
            gp[i* frame.cols * channels + j * channels] = (float) (r + g + b) / channels;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    cout << "Times passed in seconds to convert to greyscale: " << usec << endl;
    if (show) {
        imshow("Frame", gr);
        waitKey(25);
    }
    return gr;
}

int different_pixels(Mat a, Mat b, bool show) {
    cout << "Comapring" << endl;
    int cnt = 0;
    Mat res = Mat(a.rows, a.cols, CV_32F);
    float * pa = (float *) a.data;
    float * pb = (float *) b.data;
    float * pres = (float *) res.data;
    for(int i=0; i<a.rows; i++) {
        for (int j=0; j<a.cols; j++) {
            pres[i * a.cols + j] = abs(pb[i * a.cols + j] - pa[i * a.cols + j]);
            if (pres[i * a.cols + j] > 0) cnt++;
        }
    }
    if (show) {
        imshow("Background subtraction", res);
        waitKey(25);
    }
    return cnt;
}

// Sequential pattern
int main(int argc, char * argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename k show" << endl;
        return 0;
    }
    bool show = false;
    if (argc == 5) {
        if (strcmp(argv[4], "show") == 0) show = true;
    }

    // Initialization
    string filename = argv[1];
    VideoCapture cap(filename);
    int k = atoi(argv[2]); // k for accuracy then
    float percent = (float) k / 100;

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
    auto complessive_time_start = std::chrono::high_resolution_clock::now();
    Mat background; // The first frame is used as background image
    float threshold = 0;

    while(true) {
        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        // Greyscale conversion
        frame = greyscale_conversion(frame, show);
        
        // Smoothing
        frame = smoothing(frame, h1, show);

        // Movement detection
        if (frame_number == 0) { // Case first frame taken as background
            background = frame;
            // Get the average pixel intensity of the background to create a threshold
            float sum = 0;
            float * gp = (float *) frame.data;
            for(int i=0; i<frame.rows; i++) {
                for (int j=0; j<frame.cols; j++) {
                    sum = sum + gp[i* frame.cols + j];
                }
            }
            float avg_intensity = (float) sum / frame.total();
            threshold = avg_intensity / 3;
            cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
            cout << "Background average intensity: " << avg_intensity << endl;
        }
        else { // Case movement detection
            int diff_pixels = 0;
            auto start = std::chrono::high_resolution_clock::now();
            diff_pixels = different_pixels(frame, background, show);
            float different_pixels_fraction = (float)diff_pixels/frame.total();
            cout << different_pixels_fraction << endl;
            if (different_pixels_fraction > threshold) different_frames++;
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start;
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
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

    ofstream file;
    time_t now = time(0);
    char* date = (char *) ctime(&now);
    date[strlen(date) - 1] = '\0';
    file.open("results.txt", std::ios_base::app);
    file << date << " - " << filename << ",sequential," << k << "," << show << "," << complessive_usec << "," << different_frames << endl;
    file.close();

    return(0);
}
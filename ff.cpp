#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include "src/fastflow/ff_greyscale_converter.hpp"
#include "src/cthreads/thread_pool.hpp"


using namespace std;
using namespace cv;


int main(int argc, char * argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename accuracy nw" << endl;
    }
    bool show = false;
    if (argc == 5) {
        if (strcmp(argv[4], "show") == 0) show = true;
    }

    // Initialization
    string filename = argv[1];
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    //int gw = (nw - uw)/3;
    int cw = nw/3;
    int sw = nw - cw;
    if (cw == 0) cw = 1;
    if (sw == 0) sw = 1;
    cout << "Thread use: " << cw << " for conversion to greyscale and comparing, " << sw << " for smoothing" << endl;
    // Start time measurement
    auto complessive_time_start = std::chrono::high_resolution_clock::now();
    float threshold = (float) k / 100;
    VideoCapture cap(filename); 
    Mat h1 = Mat::ones(3, 3, CV_32F); // Filter matrix for smoothing
    h1 = (Mat_<float>) (1/9 * h1);
    float * h1p = (float *) h1.data;
    for (int i=0; i<h1.rows; i++) {
        for (int j=0; j<h1.rows; j++) {
            *h1p++ = (float) 1/9;
        }
    }
    atomic<int> frame_number;
    atomic<int> different_frames;
    frame_number = 0;
    different_frames = 0;

    // Take first frame as background
    Mat background; // The first frame is used as background image
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    GreyscaleConverter converter(background, cw, show);
    background = converter.convert_to_greyscale();
    deque<function<float()>> results;

    ThreadPool sm_pool(h1, sw, background, cw, show);
    sm_pool.start_pool();

    while (true) {

        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);

        GreyscaleConverter converter(frame, nw, show);
        frame = converter.convert_to_greyscale();

        sm_pool.submit_task(frame);

        frame_number++;

    }

    int res_number = 0;
    float res = 0;
    while (res_number <= frame_number) {
        res = (sm_pool.get_result())();
        if (res == -1) {
            cout << "fine dei giochi" << endl;
            break;
        }
        if (res > threshold) different_frames++;
        res_number++;
        cout << "Frames with movement detected until now: " << different_frames << " over " << res_number << " analyzed on a total of " << frame_number << endl;
        if (res_number == frame_number) break;
    }

    sm_pool.stop_pool();

    cout << "Number of frames with movement detected: " << different_frames << endl;
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;
    return 0;
};
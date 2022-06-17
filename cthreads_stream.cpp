#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <ctime>
#include "src/cthreads_stream/thread_pool_stream.hpp"


using namespace std;
using namespace cv;



int main(int argc, char * argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename accuracy nw" << endl;
    }
    bool show = false;
    bool times = false;
    if (argc == 5) {
        if (strcmp(argv[4], "show") == 0) show = true;
        else if (strcmp(argv[4], "times") == 0) times = true;
    }

    // Initialization
    string filename = argv[1];
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    int cw = nw/3;
    int sw = nw - cw;
    if (cw == 0) cw = 1;
    if (sw == 0) sw = 1;
    cout << "Thread use: " << cw << " for conversion to greyscale and comparing, " << sw << " for smoothing" << endl;
    float percent = (float) k / 100;
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

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // Take first frame as background
    Mat background; // The first frame is used as background image
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    GreyscaleConverter converter(background, cw, show, times);
    background = converter.convert_to_greyscale();
    Smoother s(background, h1, show, times);
    background = s.smoothing();
    float avg_intensity = converter.get_avg_intensity(background);
    float threshold = (float) avg_intensity / 3;
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;

    ThreadPool pool(h1, sw, background, cw, threshold, percent, show, times);
    pool.start_pool();

    while (true) {

        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);
        
        pool.submit_conversion_task(frame);

        frame_number++;

    }

    different_frames = pool.get_final_result();

    cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;

    ofstream file;
    time_t now = time(0);
    char* date = (char *) ctime(&now);
    date[strlen(date) - 1] = '\0';
    file.open("results.txt", std::ios_base::app);
    file << date << " - " << filename << ",ntstream," << k << "," << nw << "," << show << "," << complessive_usec << "," << different_frames << endl;
    file.close();
    
    return 0;
};
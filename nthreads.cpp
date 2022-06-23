#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include <ctime>

#include "src/nthreads/thread_pool.hpp"
#include "src/utils/file_writer.hpp"

using namespace std;
using namespace cv;

// Native C++ threads implementation
int main(int argc, char * argv[]) {

    // Parsing of the program arguments
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename k nw" << endl;
        return 0;
    }
    bool show = false;
    bool times = false;
    string program_name = argv[0];
    string filename = argv[1];
    program_name = program_name.substr(2, program_name.length()-1);
    string output_file = "results/" + filename.substr(filename.find('/')+1, filename.length()-filename.find('/')-5) + ".txt";
    cout << output_file << endl;
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i],"-output_file") == 0) output_file = argv[i+1];
        if (strcmp(argv[i], "-show") == 0) show = true;
        if (strcmp(argv[i], "-info") == 0) times = true;
    }
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    if (nw < 2) {
        cout << "Specify at least 2 threads" << endl;
        return 0;
    }
    // Split the number of threads for the three phases
    int cw = nw/3; // Threads for map
    if (cw == 0) cw = 1;
    int sw = nw - cw; // Workers for smoothing
    cout << "Native C++ threads implementation" << endl;
    cout << "Thread use: " << cw << " for conversion to greyscale and comparing, " << sw << " for smoothing" << endl;
    // Percentage of different pixels between frame and background to detect movement
    float percent = (float) k / 100;
    
    VideoCapture cap(filename);

    // Filter matrix for smoothing
    Mat h1 = Mat::ones(3, 3, CV_32F); 
    // For smoothing it is used average filtering
    h1 = (Mat_<float>) (1/9 * h1);
    float * h1p = (float *) h1.data;
    for (int i=0; i<h1.rows; i++) {
        for (int j=0; j<h1.rows; j++) {
            *h1p++ = (float) 1/9;
        }
    }

    int frame_number = 0;
    int different_frames = 0;

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // The first frame is taken as background image
    Mat background;
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion and smoothing
    GreyscaleConverter converter(background, cw, show, times);
    background = converter.convert_to_greyscale();
    Smoother s(background, h1, show, times);
    background = s.smoothing();
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter.get_avg_intensity(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 3;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;

    // Create and start the thread_pool
    ThreadPool pool(h1, sw, background, cw, threshold, percent, show, times);
    pool.start_pool();

    // Loop that performs the actions until the last frame of the video
    while (true) {

        // Get the frame
        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);
        
        // Submit the frame to be converted to greyscale
        pool.submit_conversion_task(frame);

        frame_number++;

    }

    // Wait that all the threads of the pool exited and get the number of frames with movement detected
    different_frames = pool.get_final_result();

    cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;
    
    // Write the results on a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, show, time, different_frames);
    
    return 0;
};
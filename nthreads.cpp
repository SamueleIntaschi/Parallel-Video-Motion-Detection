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

/**
 * @brief Print how to use the program
 * 
 * @param prog the name of the program
 */
void print_usage(string prog) {
    cout << "Basic usage is " << prog << " filename k -nw number_of_threads" << endl;
    cout << "Options are: \n" <<
    "-info: shows times information \n" <<
    "-show: shows results frames for each stage \n" <<
    "-specific_stage_nw data_smoothing_workers stream_smoothing_workers stream_converter_comparer_workers: specifies the number of threads to use for each phase\n"
    << endl;
}

// Native C++ threads implementation
int main(int argc, char * argv[]) {

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // number of stream parallel workers used for smoothing
    int stream_smoothing_workers = 1; 
    // number of stream parallel workers used for grayscale conversion and background subtraction
    int stream_cc_workers = 1;
    // number of data parallel workers used for smoothing
    int data_smoothing_workers = 1;
    // total number of threads to use
    int nw = 0;
    // flag to show result frames for each phase
    bool show = false;
    // flag to show the time for each phase
    bool times = false;

    // Options parsing
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-show") == 0) show = true;
        if (strcmp(argv[i], "-info") == 0) times = true;
        if (strcmp(argv[i], "-specific_stage_nw") == 0) {
            data_smoothing_workers = atoi(argv[i+1]);
            stream_smoothing_workers = atoi(argv[i+2]);
            stream_cc_workers = atoi(argv[i+3]);
        }
        if (strcmp(argv[i], "-help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-nw") == 0) {
            nw = atoi(argv[i + 1]);
        }
    }

    // Creation of the name of the file to write the results to
    string program_name = argv[0];
    program_name = program_name.substr(2, program_name.length()-1);
    // Name of the video
    string filename = argv[1];
    string output_file = "results/" + filename.substr(filename.find('/')+1, filename.length() - filename.find('/')-(filename.length() - filename.find('.')) - 1) + ".txt";
    // Percent of different pixels needed to detect a movement in a frame
    int k = atoi(argv[2]); 
    float percent = (float) k / 100;
    // Number of video frames
    int frame_number = 0;

    // Split the workers
    if (nw > 0) {
        if (nw > 3 && nw <= 8) {
            stream_cc_workers = 2;
            data_smoothing_workers = 1;
            stream_smoothing_workers = nw-2;
        }
        else if (nw > 8 && nw <= 16) {
            stream_cc_workers = 4;
            data_smoothing_workers = 2;
            int r = nw - stream_cc_workers;
            stream_smoothing_workers =  r / 2;
        }
        else if (nw > 16 && nw <= 24) {
            data_smoothing_workers = 3;
            stream_cc_workers = 3;      
            int r = nw - stream_cc_workers;  
            stream_smoothing_workers = r / 3;
        }
        else if (nw > 24) {
            data_smoothing_workers = 4;
            stream_cc_workers = (nw - 4)/4; 
            int r = nw - stream_cc_workers;  
            stream_smoothing_workers = r / 4;
        }
        int rest = nw - (data_smoothing_workers*stream_smoothing_workers) - stream_cc_workers;
        if (rest > 0) stream_cc_workers += rest;
    }
    else nw = (data_smoothing_workers * stream_smoothing_workers) + stream_cc_workers;

    cout << "Native C++ threads implementation" << endl;
    cout << "Total threads used: " << (data_smoothing_workers * stream_smoothing_workers) << " for smoothing and " << stream_cc_workers << " for background subtraction and grayscale conversion" << endl; 

    VideoCapture cap(filename);

    // The first frame is taken as background image
    Mat background;
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion and smoothing
    GreyscaleConverter * converter = new GreyscaleConverter(show, times);
    background = converter->convert_to_greyscale(background);
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter->get_avg_intensity(background);
    Smoother * smoother = new Smoother(data_smoothing_workers, show, times);
    background = smoother->smoothing(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 10;
    
    cout << "Frames resolution: " << background.rows << "x" << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    cout << "Threshold is: " << threshold << endl;

    // Creation of the comparer
    Comparer * comparer = new Comparer(background, threshold, show, times);
    // Create and start the thread_pool
    ThreadPool pool(smoother, converter, comparer, stream_cc_workers, stream_smoothing_workers, background, threshold, percent, show, times);
    pool.start_pool();

    // Loop that reads frames of video
    while (true) {

        // Get the frame
        Mat frame; 
        cap >> frame;
        if (frame.empty()) break;
        frame.convertTo(frame, CV_32F, 1.0/255.0);
        
        // Submit the frame to be converted to greyscale
        pool.submit_conversion_task(frame);

        // Increment the number of total frames
        frame_number++;

    }

    cap.release();
    // Communicate the frames number to the thread pool
    pool.communicate_frames_number(frame_number);

    // Wait that all the threads of the pool exited and get the number of frames with movement detected
    int different_frames = pool.get_final_result();

    cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time spent: " << complessive_usec << endl;
    
    // Write the results on a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, show, time, different_frames);
    
    return 0;
};
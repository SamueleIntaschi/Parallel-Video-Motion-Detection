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
#include "src/utils/seq_smoother.hpp" // sequential implementation used for background preparation
#include "src/utils/seq_greyscale_converter.hpp"

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
    "-nw: specifies the number of workers to use\n"<<
    "-mapping: threads will be mapped on cores"
    << endl;
}

// Native C++ threads implementation
int main(int argc, char * argv[]) {

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // 8 workers if the user does not specify a value
    int nw = 8;
    // flag to show result frames for each phase
    bool show = false;
    // flag to show the time for each phase
    bool times = false;
    // flag to indicate if each thread must be assigned to a specific core
    bool mapping = false;

    // Options parsing
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-show") == 0) show = true;
        if (strcmp(argv[i], "-info") == 0) times = true;
        if (strcmp(argv[i], "-mapping") == 0) mapping = true;
        if (strcmp(argv[i], "-help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-nw") == 0) {
            nw = atoi(argv[i + 1]);
            if (nw <= 0) nw = 1;
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

    cout << "Native C++ threads implementation" << endl;
    cout << "Total threads used: " << nw << endl; 

    VideoCapture cap(filename);

    // The first frame is taken as background image
    Mat background;
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion and smoothing
    GreyscaleConverterSeq converterseq(show, times);
    background = converterseq.convert_to_greyscale(background);
    // Computes the average intensity to establish a threshold for background subtraction
    float avg_intensity = converterseq.get_avg_intensity(background);
    SmootherSeq s(background, show, times);
    background = s.smoothing();
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 10;
    
    cout << "Frames resolution: " << background.rows << "x" << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    cout << "Threshold is: " << threshold << endl;

    // Creation of the classes to analyze frames
    GreyscaleConverter * converter = new GreyscaleConverter(show, times);
    Smoother * smoother = new Smoother(show, times);
    Comparer * comparer = new Comparer(background, threshold, show, times);
    // Creates and starts the thread_pool
    ThreadPool pool(smoother, converter, comparer, nw, background, threshold, percent, show, times, mapping);
    pool.start_pool();

    // Loop that reads frames of video
    while (true) {
        // Task generation
        Mat * frame = new Mat(background.rows, background.cols, CV_32FC3);
        cap >> *frame;
        if (frame->empty()) break;
        frame->convertTo(*frame, CV_32F, 1.0/255.0);
        // Increments the number of total frames
        frame_number++;   
        // Submits the frame to be converted to greyscale
        pool.submit_conversion_task(frame, frame_number);
    }

    cap.release();
    // Communicates the total frames number to the thread pool
    pool.communicate_frames_number(frame_number);

    // Waits that all the threads of the pool exited and gets the number of frames with movement detected
    int different_frames = pool.get_final_result();

    cout << "Number of frames with movement detected: " << different_frames << " on a total of " << frame_number << " frames" << endl;
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time spent: " << complessive_usec << endl;
    
    // Writes the results on a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, mapping, time, different_frames);
    
    return 0;
};
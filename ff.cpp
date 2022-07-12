#include <iostream>
#include "opencv2/opencv.hpp"
#include "src/fastflow/ff_comparer_worker.hpp"
#include "src/fastflow/ff_emitter.hpp"
#include "src/fastflow/ff_converter_smoother_worker.hpp"
#include "src/utils/file_writer.hpp"
#include "src/utils/seq_smoother.hpp" // sequential implementation used for background preparation
#include "src/nthreads/greyscale_converter.hpp" // sequential implementation used for background preparation
#include "src/fastflow/ff_collector.hpp"

using namespace ff;
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
    "-specific_stage_nw data_smoothing_converter_workers stream_smoothing_converter_workers stream_comparer_workers: specifies the number of threads to use for each phase\n"
    << endl;
}


// FastFlow implementation
int main(int argc, char * argv[]) {
    
    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // number of stream parallel workers used for grayscale conversion and smoothing
    int stream_smoothing_workers = 1;
    // number of stream parallel workers used for background subtraction
    int stream_comparer_workers = 1;
    // number of data parallel workers used for grascale conversion and smoothing
    int data_smoothing_workers = 1;
    // total number of workers used
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
            stream_comparer_workers = atoi(argv[i+3]);
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
    int k = atoi(argv[2]); // k for accuracy then
    float percent = (float) k / 100;

    // Split the workers
    if (nw > 0) {
        int r = 0;
        if (nw > 4 && nw <= 8) {
            stream_comparer_workers = 1;
            r = nw - stream_comparer_workers;
            data_smoothing_workers = r/2;
            stream_smoothing_workers = r/data_smoothing_workers;
        }
        else if (nw > 8 && nw <= 16) {
            stream_comparer_workers = 2;
            r = nw - stream_comparer_workers;
            data_smoothing_workers = r/2;
            stream_smoothing_workers = r/data_smoothing_workers;
        }
        else if (nw > 16 && nw <= 32) {
            stream_comparer_workers = 3;
            r = nw - stream_comparer_workers;
            data_smoothing_workers = r/2;
            stream_smoothing_workers = r/data_smoothing_workers;
        }
        else if (nw > 32) {
            stream_comparer_workers = nw/8;
            r = nw - stream_comparer_workers;
            data_smoothing_workers = r/2;
            stream_smoothing_workers = r/data_smoothing_workers;
        }
        int rest = nw - (data_smoothing_workers*stream_smoothing_workers) - stream_comparer_workers;
        if (rest > 0) stream_comparer_workers += rest;
    }
    else nw = (data_smoothing_workers * stream_smoothing_workers) + stream_comparer_workers;

    cout << "FastFlow implementation" << endl;
    cout << "Total threads used: " << (data_smoothing_workers * stream_smoothing_workers) << 
    " for grayscale conversion and smoothing, " << stream_comparer_workers << " for background subtraction" << endl; 

    VideoCapture cap(filename);

    // Take first frame as background
    Mat background; 
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion for the background
    GreyscaleConverter converter(show, times);
    background = converter.convert_to_greyscale(background);
    // Smoothing of the background
    Smoother s(background, show, times);
    background = s.smoothing();
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter.get_avg_intensity(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 10;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    cout << "Threshold is: " << threshold << endl;

    // Creation of the workers for the farms
    std::vector<std::unique_ptr<ff_node>> converter_smoother_workers;
    std::vector<std::unique_ptr<ff_node>> comparer_workers;
    for(int i=0;i<stream_smoothing_workers;++i) converter_smoother_workers.push_back(make_unique<SmootherConverterWorker>(data_smoothing_workers, show, times));
    for(int i=0;i<stream_comparer_workers;++i) comparer_workers.push_back(make_unique<ComparerWorker>(background, percent, threshold, show, times));
    // Creation of the emitter
    Emitter * emitter = new Emitter(background, cap, show, times);
    // Creation of the collector
    Collector * collector = new Collector(percent);
    // Creation the farm for the threads that performs grayscale conversion and smoothing
    ff_Farm<Mat> farm(move(converter_smoother_workers));
    farm.add_emitter(*emitter);
    farm.remove_collector();
    // Creation of the farm of the background subtraction workers
    ff_Farm<Mat, float> comparer_farm(move(comparer_workers));
    comparer_farm.remove_collector();
    // Create the final pipe
    ff_Pipe<Mat,float> pipe(farm, comparer_farm, collector);
    optimize_static(pipe);

    // Start the pipe and wait it to finish
    if (pipe.run_and_wait_end() < 0) cout << "fastflow error" << endl;

    // When the pipe has finished get the final result from the collector
    int different_frames = collector->get_different_frames_number();

    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Total time passed: " << complessive_usec << endl;

    // Print the stats if the program is compiled with -DTRACE_FASTFLOW and the flag -info is specified
    if (times) {
        pipe.ffStats(cout);
    }

    // Write the output in a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, show, time, different_frames);

    return 0;
};
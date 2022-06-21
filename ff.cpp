#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>

#include "src/fastflow/ff_comparer_collector.hpp"
#include "src/fastflow/ff_greyscale_converter_emitter.hpp"
#include "src/fastflow/ff_smoother_worker.hpp"
#include "src/fastflow/ff_greyscale_converter.hpp"
#include "src/utils/file_writer.hpp"

using namespace ff;
using namespace std;
using namespace cv;

// FastFlow implementation
int main(int argc, char * argv[]) {

    // Parsing of the program arguments
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename accuracy nw" << endl;
    }
    bool show = false;
    bool times = false;
    if (argc == 5) {
        if (strcmp(argv[4], "-show") == 0) show = true;
        else if (strcmp(argv[4], "-info") == 0) times = true;
    }
    string output_file = "results/results.txt";
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i],"-output_file") == 0) {
            output_file = argv[i+1];
            break;
        }
    }
    string program_name = argv[0];
    program_name = program_name.substr(2, program_name.length()-1);
    string filename = argv[1];
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    if (k < 4) {
        cout << "Specify at least 4 threads" << endl;
    }
    int cw = nw/4;
    int sw = nw - (2 * cw);
    if (cw == 0) cw = 1;
    if (sw == 0) sw = 1;
    cout << "FastFlow implementation" << endl;
    cout << "Thread use: " << cw << " for conversion to greyscale, " << cw <<  " for comparing, " << sw << " for smoothing" << endl;
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

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // Take first frame as background
    Mat background; 
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion using parallel_for
    GreyscaleConverter converter(background, cw, show, times);
    background = converter.convert_to_greyscale();
    // Smoothing of the background
    Smoother s(background, h1, show, times);
    background = s.smoothing();
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter.get_avg_intensity(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 3;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    
    // Nodes creation
    std::vector<std::unique_ptr<ff_node>> w;
    // Create the collector that performs background subtraction
    ComparerCollector * cmp = new ComparerCollector(background, cw, percent, threshold, show, times);
    // Create the emitter that also converts the frame to greyscale
    ConverterEmitter * cnv = new ConverterEmitter(cap, cw, show, times);
    // Create the worker that performs smoothing
    for(int i=0;i<sw;++i) w.push_back(make_unique<SmoothingWorker>(h1,show, times));
    // Create the farm for the threads that performs smoothing and remove the collector
    ff_Farm<Mat> farm(std::move(w));
    farm.remove_collector();
     
    // Create the pipe
    ff_Pipe<Mat> pipe(cnv, farm, cmp);

    OptLevel opt;
    // When the number of threads exceed the real cores set the blocking mode
    opt.max_nb_threads = ff_realNumCores();
    opt.blocking_mode = true; // enable blocking mode if #threads > max_nb_threads
    optimize_static(pipe, opt);

    // Start the pipe and wait it to finish
    if (pipe.run_and_wait_end() < 0) cout << "fastflow error" << endl;

    // When the pipe has finished get the final result from the collector
    int different_frames = cmp->get_different_frames_number();

    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Total time passed: " << complessive_usec << endl;

    if (times) {
        pipe.ffStats(cout);
    }

    // Write the output in a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, show, time, different_frames);

    return 0;
};
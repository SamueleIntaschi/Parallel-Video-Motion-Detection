#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>

#include "src/fastflow/ff_comparer_collector.hpp"
#include "src/fastflow/ff_comparer_worker.hpp"
#include "src/fastflow/ff_greyscale_converter_emitter.hpp"
#include "src/fastflow/ff_smoother_worker.hpp"
#include "src/fastflow/ff_greyscale_converter.hpp"
#include "src/utils/file_writer.hpp"

using namespace ff;
using namespace std;
using namespace cv;



// FastFlow implementation
int main(int argc, char * argv[]) {
    
    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // Parsing of the program arguments
    if (argc == 1) {
        cout << "Usage is " << argv[0] << " filename k nw" << endl;
        return 0;
    }
    bool show = false;
    bool times = false;
    string program_name = argv[0];
    program_name = program_name.substr(2, program_name.length()-1);
    string filename = argv[1];
    string output_file = "results/" + filename.substr(filename.find('/')+1, filename.length() - filename.find('/')-(filename.length() - filename.find('.')) - 1) + ".txt";
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i],"-output_file") == 0) output_file = argv[i+1];
        if (strcmp(argv[i], "-show") == 0) show = true;
        if (strcmp(argv[i], "-info") == 0) times = true;
    }
    
    int k = atoi(argv[2]); // k for accuracy then
    int nw = atoi(argv[3]); // number of workers
    //int np = atoi(argv[4]);; // number 

    int stream_smoothing_workers = 1;
    int stream_comparer_workers = 1;
    int data_converter_workers = 1;
    int data_smoothing_workers = 1;

    // Split the workers
    if (nw > 4 && nw <= 8) {
        data_converter_workers = 1;
        stream_comparer_workers = 1;
        data_smoothing_workers = 1;
        stream_smoothing_workers = nw-2;
    }
    else if (nw > 8 && nw <= 16) {
        data_converter_workers = 2;
        stream_comparer_workers = 2;
        data_smoothing_workers = 2;
        int r = nw - data_converter_workers - stream_comparer_workers;
        stream_smoothing_workers =  r / 2;
    }
    else if (nw > 16 && nw <= 32) {
        data_converter_workers = 2;
        data_smoothing_workers = 3;
        stream_comparer_workers = 4;      
        int r = nw - data_converter_workers - stream_comparer_workers;  
        stream_smoothing_workers = r / 3;
    }
    else if (nw > 32) {
        data_converter_workers = 4;
        data_smoothing_workers = 4;
        stream_comparer_workers = (nw - 8)/4;      
        int r = nw - data_converter_workers - stream_comparer_workers;  
        stream_smoothing_workers = r / 4;
    }
    int rest = nw - data_converter_workers - (data_smoothing_workers*stream_smoothing_workers) - stream_comparer_workers;
    if (rest > 0) stream_comparer_workers += rest;

    cout << "FastFlow implementation" << endl;
    cout << "Total threads used: " << data_converter_workers << " for grayscale conversion, " << (data_smoothing_workers * stream_smoothing_workers) << " for smoothing and " << stream_comparer_workers << " for background subtraction" << endl; 

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

    // Take first frame as background
    Mat background; 
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion using parallel_for
    GreyscaleConverter converter(background, data_converter_workers, show, times);
    background = converter.convert_to_greyscale();
    //for(int i=0;i<sm_p;++i) w.push_back(new SmoothingWorker(sm_w, h1,show, times));
    // Smoothing of the background
    Smoother s(background, h1, show, times);
    background = s.smoothing();
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter.get_avg_intensity(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 10;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    cout << "Threshold is: " << threshold << endl;
    
    // Create the collector that performs background subtraction
    //ComparerCollector * cmp = new ComparerCollector(background, cm_w, percent, threshold, show, times);
    // Create the emitter that also converts the frame to greyscale
    ConverterEmitter * cnv = new ConverterEmitter(cap, data_converter_workers, show, times);
    
    //std::vector<std::unique_ptr<ff_node>> we;
    // Create the worker that performs smoothing
    //for(int i=0;i<cm_p;++i) we.push_back(make_unique<ConverterEmitter>(cap, 1,show, times));
    //std::vector<std::unique_ptr<ff_node>> wc;
    // Create the worker that performs smoothing
    //for(int i=0;i<cm_p;++i) wc.push_back(make_unique<ComparerCollector>(background, 1, percent, threshold, show, times));
    //std::vector<std::unique_ptr<ff_node>> smoothing_workers;
    // Create the worker that performs smoothing
    //for(int i=0;i<sm_p;++i) smoothing_workers.push_back(make_unique<SmoothingWorker>(sm_w, h1,show, times));
    // Create the farm for the threads that performs smoothing and remove the collector
    //ff_Farm<Mat> smoothing_farm(std::move(smoothing_workers));
    //smoothing_farm.add_emitter(*cnv);
    //farm.add_workers(w);
    //farm.add_collector(*cmp);
    //smoothing_farm.remove_collector();

    //std::vector<std::unique_ptr<ff_node>> comparer_workers;
    vector<ff_node*> smoothing_workers;
    vector<ff_node*> comparer_workers;
    for (int i=0; i<stream_smoothing_workers; i++) smoothing_workers.push_back(new SmoothingWorker(data_smoothing_workers, h1, show, times));
    for (int i=0; i<stream_comparer_workers; i++) comparer_workers.push_back(new ComparerWorker(background, percent, threshold, show, times));
    Collector * collector = new Collector(percent);
    // Create the worker that performs smoothing
    //for(int i=0;i<sm_p;++i) wc.push_back(make_unique<ComparerWorker>(background, cm_w, percent, threshold, show, times));
    // Create the farm for the threads that performs smoothing and remove the collector
    //ff_Farm<Mat> comparer_farm(std::move(wc));
    //comparer_farm.add_collector(*collector);

    ff_a2a a2a;
    a2a.add_firstset(smoothing_workers);
    a2a.add_secondset(comparer_workers);
     
    // Create the pipe
    //ff_Pipe<Mat,float> pipe(smoothing_farm, comparer_farm);
    ff_Pipe<Mat, float> pipe(cnv, a2a, collector);

    //OptLevel opt;
    // When the number of threads exceed the real cores set the blocking mode
    //opt.max_nb_threads = ff_realNumCores();
    //opt.blocking_mode = true; // enable blocking mode if #threads > max_nb_threads
    optimize_static(pipe);//, opt);

    // Start the pipe and wait it to finish
    if (pipe.run_and_wait_end() < 0) cout << "fastflow error" << endl;

    // When the pipe has finished get the final result from the collector
    int different_frames = collector->get_different_frames_number();

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
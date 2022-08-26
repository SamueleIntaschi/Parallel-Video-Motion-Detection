#include <iostream>
#include "opencv2/opencv.hpp"
#include "src/utils/file_writer.hpp"
#include "src/utils/seq_smoother.hpp" // sequential implementation used for background preparation
#include "src/utils/seq_greyscale_converter.hpp" // sequential implementation used for background preparation
#include "src/fastflow/fffarm/ff_source.hpp"
#include "src/fastflow/fffarm/ff_sink.hpp"
#include "src/fastflow/fffarm/ff_farm_worker.hpp"

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
    "-nw: specifies the number of workers to use\n" <<
    "-mapping: threads will be mapped on cores"
    << endl;
}


// FastFlow implementation
int main(int argc, char * argv[]) {
    
    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // total number of workers used
    int nw = 1;
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
    int k = atoi(argv[2]); // k for accuracy then
    float percent = (float) k / 100;

    cout << "FastFlow Farm implementation" << endl;
    cout << "Total threads used: " << nw << endl; 

    VideoCapture cap(filename);

    // Take first frame as background
    Mat background; 
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    // Greyscale conversion for the background
    GreyscaleConverterSeq converter(show, times);
    background = converter.convert_to_greyscale(background);
    // Smoothing of the background
    SmootherSeq s(background, show, times);
    background = s.smoothing();
    // Compute the average intensity to establish a threshold for background subtraction
    float avg_intensity = converter.get_avg_intensity(background);
    // Threshold to exceed to consider two pixels different
    float threshold = (float) avg_intensity / 10;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    cout << "Threshold is: " << threshold << endl;

    // Creation of the emitter
    Source * emitter = new Source(background, cap, show, times);
    // Creation of the collector
    Sink * collector = new Sink(percent, times);
    vector<std::unique_ptr<ff_node>> farm_workers;
    for(int i=0;i<nw;++i){
        farm_workers.push_back(make_unique<FarmWorker>(background, threshold, show, times));
    }
    ff_Farm<Mat, float> farm(move(farm_workers));
    farm.add_emitter(*emitter);
    farm.add_collector(*collector);
    farm.set_scheduling_ondemand();
    if (mapping == false) {
        OptLevel opt;
        opt.max_mapped_threads = 0;
        opt.no_default_mapping = true;
        opt.max_nb_threads = 0;
        opt.blocking_mode = true;
        optimize_static(farm, opt);
    }
    if (farm.run_and_wait_end() < 0) cout << "fastflow error" << endl;
        
    // When the pipe has finished get the final result from the collector
    int different_frames = collector->get_different_frames_number();

    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Total time passed: " << complessive_usec << endl;

    // Print the stats if the program is compiled with -DTRACE_FASTFLOW and the flag -info is specified
    if (times) {
        farm.ffStats(cout);
    }

    delete collector;
    delete emitter;

    // Write the output in a file
    FileWriter fw(output_file);
    string time = to_string(complessive_usec);
    fw.print_results(filename, program_name, k, nw, show, time, different_frames);

    return 0;
};
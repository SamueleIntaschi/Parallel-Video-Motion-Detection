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
    if (k < 4) {
        cout << "Specify at least 4 threads" << endl;
    }
    int cw = nw/4;
    int sw = nw - (2 * cw);
    if (cw == 0) cw = 1;
    if (sw == 0) sw = 1;
    cout << "Thread use: " << cw << " for conversion to greyscale, " << cw <<  " for comparing, " << sw << " for smoothing" << endl;
    // Start time measurement
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

    auto complessive_time_start = std::chrono::high_resolution_clock::now();

    // Take first frame as background
    Mat background; // The first frame is used as background image
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    GreyscaleConverter converter(background, cw, show, times);
    background = converter.convert_to_greyscale();
    //background = (converter.convert_to_greyscale());
    Smoother s(background, h1, show, times);
    background = s.smoothing();
    float avg_intensity = converter.get_avg_intensity(background);
    float threshold = (float) avg_intensity / 3;
    
    cout << "Frames resolution: " << background.rows << " x " << background.cols << endl;
    cout << "Background average intensity: " << avg_intensity << endl;
    
    std::vector<std::unique_ptr<ff_node>> w;
    ComparerCollector * cmp = new ComparerCollector(background, cw, percent, threshold, show, times);
    ConverterEmitter * cnv = new ConverterEmitter(cap, cw, show, times);
    for(int i=0;i<sw;++i) w.push_back(make_unique<SmoothingWorker>(h1,show, times));

    ff_Farm<Mat> farm(std::move(w));
    farm.remove_collector();
     
    ff_Pipe<Mat> pipe(cnv, farm, cmp);

    OptLevel opt;
    opt.max_mapped_threads = ff_realNumCores();
    opt.no_default_mapping = true; // disable mapping if #threads > max_mapped_threads
    opt.max_nb_threads = ff_realNumCores();
    opt.blocking_mode = true; // enable blocking mode if #threads > max_nb_threads
    optimize_static(pipe, opt);

    if (pipe.run_and_wait_end() < 0) cout << "fastflow error" << endl;

    int different_frames = cmp->get_different_frames_number();

    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();

    cout << "Total time passed: " << complessive_usec << endl;

    //pipe.ffStats(cout);

    FileWriter fw("results.txt");
    string time = to_string(complessive_usec);
    fw.print_results(filename,"ff",k,nw,show,time,different_frames);


    return 0;
};
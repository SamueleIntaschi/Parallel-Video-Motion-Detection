#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include "src/fastflow/pipe.hpp"

using namespace ff;
using namespace std;
using namespace cv;

//TODO try with flag NO_DEFAULT_MAPPING

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

    // Take first frame as background
    Mat background; // The first frame is used as background image
    cap >> background;
    if (background.empty()) return 0;
    background.convertTo(background, CV_32F, 1.0/255.0);
    GreyscaleConverter converter(background, cw, show);
    background = *(converter.convert_to_greyscale());
    float avg_intensity = converter.get_avg_intensity(background);
    float threshold = (float) avg_intensity / 3;
    
    cout << "Backgorund average intensity: " << avg_intensity << endl;
    
    std::vector<std::unique_ptr<ff_node>> w;
    Converter * cnv = new Converter(cap, cw, show);
    Comparing * cmp = new Comparing(background, cw, percent, threshold, show);
    for(int i=0;i<nw;++i) w.push_back(make_unique<SmoothingWorker>(h1,show));
    ff_Farm<Mat> farm(std::move(w));
    ff_Pipe<Mat> pipe(cnv, farm, cmp);

    std::cout << "Starting application ..." << std::endl;
    cout << pipe.run_and_wait_end() << endl;;

    auto complessive_time_end = std::chrono::high_resolution_clock::now();
    auto complessive_duration = std::chrono::high_resolution_clock::now() - complessive_time_start;
    auto complessive_usec = std::chrono::duration_cast<std::chrono::microseconds>(complessive_duration).count();
    cout << "Total time passed: " << complessive_usec << endl;
    return 0;
};
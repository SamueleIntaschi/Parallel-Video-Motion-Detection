#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>
#include "../common_classes/smoother.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace std;
using namespace cv;

class SmoothingWorker: public ff_node_t<Mat> {
    private:
        bool show = false;
        bool times = false;
        Mat filter;

    public:
        SmoothingWorker(Mat filter, bool show, bool times): filter(filter), show(show), times(times) {}

        Mat * svc(Mat * m) {
            
            if (m != EOS) {
                Smoother s(*m, this->filter, this->show, this->times);
                *m = s.smoothing();
                return(m);
            }
            
            return m;
        }
};
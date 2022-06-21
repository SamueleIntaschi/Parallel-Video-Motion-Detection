#include <iostream>
#include "opencv2/opencv.hpp"
#include <thread>
#include <chrono> 
#include <atomic>
#include <queue>
#include <mutex>
#include <vector>

#include "../utils/smoother.hpp"
#include <ff/ff.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <ff/map.hpp>

using namespace ff;
using namespace std;
using namespace cv;

/**
 * @brief Class that represents a worker that performs smoothing on the given matrix
 * 
 */
class SmoothingWorker: public ff_node_t<Mat> {
    private:
        bool show = false;
        bool times = false;
        Mat filter; // filter matrix

    public:
        SmoothingWorker(Mat filter, bool show, bool times): filter(filter), show(show), times(times) {}

        /**
         * @brief Main function of the node, it performs smoothing on the given matrix and submits the result 
         *        to the next node
         * 
         * @param m matrix on which perform smoothing
         * @return matrix on which the filter is applied
         */
        Mat * svc(Mat * m) {
            
            if (m != EOS) {
                // Use the smoother object to perform smoothing
                Smoother s(*m, this->filter, this->show, this->times);
                *m = s.smoothing();
                return(m);
            }
            
            return m;
        }
};
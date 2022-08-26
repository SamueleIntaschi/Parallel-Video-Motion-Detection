#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>

using namespace ff;
using namespace std;
using namespace cv;

/**
 * @brief Class representing the collector of the pipe, it receives the percentage of pixels that differ more than the
 *        threshold from the background pixels and compares them with the parameter specified by the user to decide if there
 *        is a movement in the frame
 * 
 */
class Sink: public ff_minode_t<float> {
    private:
        int frames_with_movement = 0;
        int frame_number = 0;
        float percent;
        bool has_finished = false;
        bool times;


    public:

        Sink(float percent, bool times): percent(percent), times(times) {}

        /**
         * @brief Main function of the node
         * 
         * @param diff the percentage of different pixels out of the total
         * @return float* 
         */
        float * svc(float * diff) {
            float &d = *diff;
            if (d > this->percent) (this->frames_with_movement)++;
            // Delete the number when it has been analyzed
            delete diff;
            (this -> frame_number)++;
            if (times) cout << "Frames with movement detected until now: " << frames_with_movement << " over " << frame_number << " analyzed" << endl;
            return GO_ON;
        }

        /**
         * @brief function executed when the node has finished its work, it prints a message and update a flag
         * 
         */
        void svc_end() {
            cout << "Number of frames with movement detected: " << frames_with_movement << " on a total of " << frame_number << " frames" << endl;
            this -> has_finished = true;
        }

        /**
         * @brief Get the number of frames with movement detected from outside the node if the stream is finished
         * 
         * @return the number of frames with movement detected
         */
        int get_different_frames_number() {
            if (this -> has_finished) return this->frames_with_movement;
            else return -1;
        }

};
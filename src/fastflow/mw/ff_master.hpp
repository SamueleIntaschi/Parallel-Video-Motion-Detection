#include <iostream>
#include "opencv2/opencv.hpp"
#include <ff/ff.hpp>
#include "../mw//ff_worker.hpp"

using namespace ff;
using namespace std;
using namespace cv;


class Master: public ff_monode_t<Task> {

    private:
        int frames_with_movement = 0;
        float percent;
        int total_frames = -1;
        int frame_number = 0;
        bool has_finished = false;
        bool times = false;
        bool eos_received = false;

    public:
        Master(float percent, bool times): percent(percent), times(times) {}

        Task * svc(Task * t) {
            if (t -> n >= 0 && t -> n <= 1) {
                // Result of background subtraction
                this->frame_number++;
                if (t->n >= this->percent) this->frames_with_movement++;
                delete t;
                if (times) cout << "Frames with movement detected until now: " << frames_with_movement << " over " << frame_number << " analyzed" << endl;
                if (this->eos_received && this->total_frames != -1 && this->total_frames == this->frame_number) {
                    broadcast_task(EOS);
                }
                return GO_ON;
            }
            else if (t->n > 4) {
                this->total_frames = t->n;
                if (eos_received && this->total_frames == this->frame_number) {
                    broadcast_task(EOS);
                }
                delete t;
                return GO_ON;
            }
            return t;
        }

        /**
         * @brief function executed when EOS is received, it save the information and notify all the workers
         *        when it is needed
         * 
         */
        void eosnotify(ssize_t id) {
            if (!eos_received) {
                eos_received = true;
                if (this->total_frames != -1 && this->total_frames == this->frame_number) {
                    broadcast_task(EOS);
                }
            }
        }

        /**
         * @brief function executed when the node has finished its work, it prints a message and update a flag
         * 
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
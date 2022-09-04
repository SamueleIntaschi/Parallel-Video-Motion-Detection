#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

/**
 * @brief Class used to print results on a file
 * 
 */
class FileWriter {
    private:
        string filename;

    public:
        FileWriter(string filename): filename(filename) {}

        /**
         * @brief print results on a file
         * 
         * @param video the name of the video
         * @param type ff, nt or seq
         * @param k k parameter specified by the user
         * @param nw number of workers for each stage used for data parallelism
         * @param mapping flag that indicates if mapping is used or not
         * @param complessive_usec execution time 
         * @param different_frames number of frames with movement found
         * @param np number of replication of each stage
         */
        void print_results(string video, string type, int k, int nw, bool mapping, string complessive_usec, int different_frames) {
            ofstream file;
            time_t now = time(0);
            char* date = (char *) ctime(&now);
            date[strlen(date) - 1] = '\0';
            file.open(this->filename, std::ios_base::app);
            //file << date << " - " << video << "," << type << "," << k << "," << nw << "," << np << "," << show << "," << complessive_usec << "," << different_frames << endl;
            file << date << " - " << video << "," << type << "," << k << "," << nw << "," << mapping << "," << complessive_usec << "," << different_frames << endl;
            file.close();
        }

};
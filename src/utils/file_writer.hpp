#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

class FileWriter {
    private:
        string filename;

    public:
        FileWriter(string filename): filename(filename) {}

        void print_results(string video, string type, int k, int nw, bool show, string complessive_usec, int different_frames) {
            ofstream file;
            time_t now = time(0);
            char* date = (char *) ctime(&now);
            date[strlen(date) - 1] = '\0';
            file.open(this->filename, std::ios_base::app);
            file << date << " - " << video << "," << type << "," << k << "," << nw << "," << show << "," << complessive_usec << "," << different_frames << endl;
            file.close();
        }

};
#include "string"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char * argv[]) {

    if (argc == 1) {
        cout << "Usage is one of the following command: \n" <<
                                argv[0] << " 1 from_nw to_nw \n" << 
                                argv[0] << " 2 from_nw to_nw percent video \n" <<
                                argv[0] << " 3 nw percent video tries" <<
                                endl;
        return 0;
    }
    bool seq = false;
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-seq") == 0) seq = true;
    }
    int type = atoi(argv[1]);

    if (type == 1) {

        int nw0 = atoi(argv[2]); 
        int nw1 = atoi(argv[3]);

        if (seq) {
            string command0 = "./seq Videos/people1.mp4 2";
            system(command0.c_str());
        }

        for (int i=nw0; i<=nw1; i++) {
            string command1 = "./nt Videos/people1.mp4 12 -nw " + to_string(i);
            string command2 = "./ff Videos/people1.mp4 12 -nw " + to_string(i);
            system(command1.c_str());
            system(command2.c_str());
        }

    }
    else if (type == 2) {

        int nw0 = atoi(argv[2]); 
        int nw1 = atoi(argv[3]);
        string filename = argv[5];
        int percent = atoi(argv[4]);

        if (seq) {
            string command0 = "./seq " +  filename + " " + to_string(percent);
            system(command0.c_str());
        }

        for (int i=nw0; i<=nw1; i++) {
            string command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(i);
            string command2 = "./ff " +  filename + " " + to_string(percent) + " -nw " + to_string(i);
            system(command1.c_str());
            system(command2.c_str());
        }
    }

    else if (type == 3) {

        int nw0 = atoi(argv[2]);
        int tries = atoi(argv[5]);
        string filename = argv[4];
        int percent = atoi(argv[3]);

        if (seq) {
            string command0 = "./seq " +  filename + " " + to_string(percent);
            system(command0.c_str());
        }

        for (int i=0; i<tries; i++) {
            string command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0);
            string command2 = "./ff " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0);
            system(command1.c_str());
            system(command2.c_str());
        }
    } 

    return 0;
}
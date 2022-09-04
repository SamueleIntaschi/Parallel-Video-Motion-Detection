#include "string"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char * argv[]) {

    if (argc == 1) {
        cout << "Usage is one of the following command: \n" <<
                                argv[0] << " 1 from_nw to_nw percent video \n" <<
                                argv[0] << " 2 nw percent video tries" <<
                                endl;
        return 0;
    }
    // Options parsing
    bool mapping = false;
    bool seq = false;
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-mapping") == 0) mapping = true;
        if (strcmp(argv[i], "-seq") == 0) seq = true;
    }
    int type = atoi(argv[1]);

    // Tries all the implementations with a range of thread numbers specified by the user
    if (type == 1) {

        int nw0 = atoi(argv[2]); 
        int nw1 = atoi(argv[3]);
        string filename = argv[5];
        int percent = atoi(argv[4]);

        if (seq) {
            string command0 = "./seq " +  filename + " " + to_string(percent);
            system(command0.c_str());
        }

        for (int i=nw0; i<=nw1; i++) {
            string command1;
            string command2;
            string command3;
            if (mapping == false) {
                command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(i);
                command2 = "./ffmw " +  filename + " " + to_string(percent) + " -nw " + to_string(i);
                command3 = "./fffarm " +  filename + " " + to_string(percent) + " -nw " + to_string(i);
            }
            else {
                command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(i) + " -mapping";
                command2 = "./ffmw " +  filename + " " + to_string(percent) + " -nw " + to_string(i) + " -mapping";
                command3 = "./fffarm " +  filename + " " + to_string(percent) + " -nw " + to_string(i)  + " -mapping";
            }
            system(command1.c_str());
            system(command2.c_str());
            system(command3.c_str());
        }
    }

    // Tries all the implementations more time with a specific number of threads
    else if (type == 2) {

        int nw0 = atoi(argv[2]);
        int tries = atoi(argv[5]);
        string filename = argv[4];
        int percent = atoi(argv[3]);

        if (seq) {
            string command0 = "./seq " +  filename + " " + to_string(percent);
            system(command0.c_str());
        }

        for (int i=0; i<tries; i++) {
            string command1;
            string command2;
            string command3;
            if (mapping == false) {
                command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0);
                command2 = "./ffmw " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0);
                command3 = "./fffarm " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0);
            }
            else {
                command1 = "./nt " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0) + " -mapping";
                command2 = "./ffmw " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0) + " -mapping";
                command3 = "./fffarm " +  filename + " " + to_string(percent) + " -nw " + to_string(nw0)  + " -mapping";
            }
            system(command1.c_str());
            system(command2.c_str());
            system(command3.c_str());
        }
    } 

    return 0;
}
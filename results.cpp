#include "string"
#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char * argv[]) {

    if (argc == 1) {
        cout << "Usage is one of the following command: \n" << 
                                argv[0] << " 1 nw \n" << 
                                argv[0] << " 2 from_nw to_nw \n" << 
                                argv[0] << " 3 from_nw to_nw percent video  " <<
                                endl;
    }
    bool seq = false;
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-seq") == 0) seq = true;
    }
    int type = atoi(argv[1]);
    
    if (type == 1) {

        int nw = atoi(argv[2]);
        
        string command1 = "./seq Videos/people_low1.mp4 5 -output_file results/people_low1.txt";
        string command2 = "./nt Videos/people_low1.mp4 5 " + to_string(nw) + " -output_file results/people_low1.txt";  
        string command3 = "./ff Videos/people_low1.mp4 5 " + to_string(nw) + " -output_file results/people_low1.txt";
        if (seq) system(command1.c_str());
        system(command2.c_str());
        system(command3.c_str());

        command1 = "./seq Videos/people_low2.mp4 13 -output_file results/people_low2.txt";
        command2 = "./nt Videos/people_low2.mp4 13 " + to_string(nw) + " -output_file results/people_low2.txt";  
        command3 = "./ff Videos/people_low2.mp4 13 " + to_string(nw) + " -output_file results/people_low2.txt";
        if (seq) system(command1.c_str());
        system(command2.c_str());
        system(command3.c_str());

        command1 = "./seq Videos/people_low3.mp4 35 -output_file results/people_low3.txt";
        command2 = "./nt Videos/people_low3.mp4 35 " + to_string(nw) + " -output_file results/people_low3.txt";  
        command3 = "./ff Videos/people_low3.mp4 35 " + to_string(nw) + " -output_file results/people_low3.txt";
        if (seq) system(command1.c_str());
        system(command2.c_str());
        system(command3.c_str());

    }
    else if (type == 2) {

        int nw0 = atoi(argv[2]); 
        int nw1 = atoi(argv[3]);

        if (seq) {
            string command0 = "./seq Videos/people_low1.mp4 5 -output_file results/people_low1.txt";
            system(command0.c_str());
        }

        for (int i=nw0; i<nw1; i++) {
            string command1 = "./nt Videos/people_low1.mp4 5 " + to_string(i) + " -output_file results/people_low1.txt";
            string command2 = "./ff Videos/people_low1.mp4 5 " + to_string(i) + " -output_file results/people_low1.txt";
            system(command1.c_str());
            system(command2.c_str());
        }

    }
    else if (type == 3) {

        int nw0 = atoi(argv[2]); 
        int nw1 = atoi(argv[3]);

        string filename = argv[5];
        int percent = atoi(argv[4]);
        int pos1 = filename.find("/");
        int pos2 = filename.find(".");
        string o_file = filename.substr(pos1+1, pos2-pos1) + "txt";
        o_file = "results/" +  o_file;

        if (seq) {
            string command0 = "./seq " +  filename + " " + to_string(percent) + " -output_file " + o_file;
            system(command0.c_str());
        }

        for (int i=nw0; i<nw1; i++) {
            string command1 = "./nt " +  filename + " " + to_string(percent) + " " + to_string(i) + " -output_file " + o_file;
            string command2 = "./ff " +  filename + " " + to_string(percent) + " " + to_string(i) + " -output_file " + o_file;
            system(command1.c_str());
            system(command2.c_str());
        }
    }

    return 0;
}
//#include <stdio.h>
#include <unistd.h>


int main(int argc, char **argv) {

    int opt;
    while ((opt = getopt(argc, argv, "abc")) != -1) {
        switch (opt) {
            case 'a':
                /* code */
                break;
            case 'b':
                /* code */
                break;
            case 'c':
                /* code */
                break;
            default:
                /* code */
                break;
        }
    }
    
    return 0;
}
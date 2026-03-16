#include "../inc/structs.h"
#include "../inc/functions.h"
#include <string.h>

int main(int argc, char **argv) {
    t_args args; // burda sıfırla memset' gerek kalmayabilir

    memset(&args, 0, sizeof(t_args)); // args yapısını sıfırla
    parse_arguments(argc, argv, &args);

    
}
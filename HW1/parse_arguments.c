#include "./includes/struct.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void print_error(t_error error, char flag, char *arg){
    switch (error){
        case ERR_NO_VALUE:
            write(STDERR_FILENO, "ERROR!: No value was entered for parameter -", 44);
            write(STDERR_FILENO, &flag, 1);
            write(STDERR_FILENO, ".\n", 2);
            break;
        
        case ERR_INVALID_PARAM:
            write(STDERR_FILENO, "ERROR! : Invalid parameter. -> ", 31);
            write(STDERR_FILENO, "-", 1);
            write(STDERR_FILENO, &flag, 1);
            write(STDERR_FILENO, "\n", 1);
            break;

        case ERR_BAD_FORMAT:
            write(STDERR_FILENO, "ERROR! : Argument in unexpected format. -> ", 43);
            write(STDERR_FILENO, arg, strlen(arg)); 
            write(STDERR_FILENO, "\n", 1);
            break;

        case ERR_NO_TARGET_DIR:
            write(STDERR_FILENO, "ERROR! : Parameter -w is mandatory.\n", 36);
            break;

        case ERR_NO_CRITERIA:
            write(STDERR_FILENO, "ERROR! : At least one search criterion (-f, -b, -t, -p, -l) must be specified.\n", 79);
            break;
        default:
            write(STDERR_FILENO, "Unknown error\n", 14);
            break;
    }
}
// w -> return 2, others -> return 1, invalid -> return 0
int is_correct_flags(int i, char flag, char **argv, t_file *criteria) {
    switch (flag) {
        case 'w':
            criteria->target_dir = argv[i + 1];
            break;
        case 'f':
            criteria->filename = argv[i + 1];
            return 1;
            break;
        case 'b': // byte olmalı ya o durumda bir error bastırmalı mısın?
            criteria->file_size = atoi(argv[i + 1]);
            return 1;
            break;
        case 't':
            criteria->file_type = argv[i + 1];
            return 1;
            break;
        case 'p':
            criteria->permissions = argv[i + 1];
            return 1;
            break;
        case 'l':
            criteria->link_count = atoi(argv[i + 1]);
            return 1;
            break;
        default:
            print_error(ERR_INVALID_PARAM, flag, NULL);
            return 0;
    }
    return 2;
}

int parse_arguments(int argc, char **argv, t_file *criteria) {
    int i = 1;
    int has_criteria = 0; 

    while (i < argc) {
        if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][2] == '\0') {
            char flag = argv[i][1];
            if (argv[i + 1] == NULL) {
                print_error(ERR_NO_VALUE, flag, NULL);
                return 0; 
            }
            int res = is_correct_flags(i, flag, argv, criteria); // w -> return 2, others -> return 1, invalid -> return 0
            if (res == 1) has_criteria = 1;
            else if (res == 0) return 0;
            //i++; 
        } else {
            print_error(ERR_BAD_FORMAT, 0, argv[i]);
            return 0; 
        }
        i+=2; // çalışmaza burayı i++ yapmayı unutma.
    }

    if (criteria->target_dir == NULL) {
        print_error(ERR_NO_TARGET_DIR, 0, NULL);
        return 0;
    }
    if (!has_criteria) {
        print_error(ERR_NO_CRITERIA, 0, NULL);
        return 0;
    }
    return 1; 
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "ptree.h"

int main(int argc, char **argv) {
    if(argc != 2 && (argc != 4 || (argc == 4 && strcmp(argv[1], "-d") != 0 ))){ //CASE1: Too few/many arguments or wrong flag.
        fprintf(stderr, "Usage:\n\tptree [-d N] PID\n");
        return 1;
    }
    struct TreeNode *root = NULL;
    int pid = strtol(argv[argc - 1], NULL, 10);
    int generate_test = generate_ptree(&root, pid);
    if (argc == 2){ //CASE2: No flag specified. Print the whole tree.
        print_ptree(root, 0);
	if (generate_test == 1){
	    return 2;
	}
	return 0;
    }

    int depth = strtol(argv[2], NULL, 10); //Set specified depth.
    if (depth < 0){
        fprintf(stderr, "Usage:\n\tptree [-d N] PID\n");
	return 1;
    }
    print_ptree(root, depth);
    if (generate_test == 1){ //CASE3: Flag specified but library call failed.
        return 2;
    }
    return 0;

}


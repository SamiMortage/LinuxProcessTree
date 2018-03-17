#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ptree.h"

// Defining the constants described in ptree.h
const unsigned int MAX_PATH_LENGTH = 1024;


//Defining a tuple in order to keep track of the child array
//and it's size
struct Tuple{
    pid_t *child_array;
    int size;
};
// If TEST is defined (see the Makefile), will look in the tests 
// directory for PIDs, instead of /proc.
#ifdef TEST
    const char *PROC_ROOT = "tests";
#else
    const char *PROC_ROOT = "/proc";
#endif
/*malloc wrapper function that checks for errors.
 *Return a pointer to the allocated memory if 
 *malloc succeeds, otherwise exit with error message.
 */
void *Malloc(int size){
    void *pt = malloc(size);
    if(pt == NULL){
	perror("malloc");
	exit(1);
    }
    return pt;
}


/*Return 0 if pid and it's exe link exists, return -1 otherwise.
 */
int validate_process(pid_t pid){
    char path_to_pid[MAX_PATH_LENGTH + 1];
    char path_to_exe[MAX_PATH_LENGTH + 1];
    if (sprintf(path_to_exe, "%s/%d/exe", PROC_ROOT, pid) <= 0){
	fprintf(stderr, "Sprintf failed\n");
	exit(1);//exit because lstat will fail
    }
    if (sprintf(path_to_pid, "%s/%d", PROC_ROOT, pid) <= 0){
        fprintf(stderr, "Sprintf failed\n");
        exit(1);//exit because stat will fail
    }
    struct stat stat_buf;
    if (stat(path_to_pid, &stat_buf) == -1 || lstat(path_to_exe, &stat_buf) == -1){
        return -1;
    }
    return 0;
}

/*Populate tuple with an array containg the PID's of the children in
 *the cmdline file found in path and the size of the array.
 *If the child's process is not executing or the PID is not found, it's not added to the array.
 *Return 1 if any of the libray calls fail and 0 otherwise.
 */
int parse_valid_children(struct Tuple **tuple , pid_t parent_pid){
    int ret = 0; //Keeps track of error code.
    char path_to_children[MAX_PATH_LENGTH + 1];
    if (sprintf(path_to_children, "%s/%d/task/%d/children", PROC_ROOT, parent_pid, parent_pid) <= 0){
	fprintf(stderr, "Sprintf failed\n");
	free(*tuple);//Free allocated space before returning
	(*tuple) = NULL;
        return 1;
    }
    FILE *file = fopen(path_to_children ,"r");
    if (file == NULL){
        fprintf(stderr, "Cannot open file: %s\n", path_to_children);
        free(*tuple);
        (*tuple) = NULL; 
 	return 1;
    }
    int num_of_children = 0;
    //Count the number of children to know the size
    //of the array.
    while(fscanf(file, "%*d") != EOF){
        num_of_children++;
    }
    if (num_of_children == 0){
	free(*tuple);
	*tuple =  NULL;
	return 0;
    }
    rewind(file);
    pid_t *child_array = Malloc(sizeof(int) * num_of_children);
    char pid_str[5];
    int num_invalid = 0;//Keeps track of the number of invalid PID's.
    int j = 0;
    while(fscanf(file, "%s", pid_str) != EOF){
        child_array[j] = strtol(pid_str, NULL, 10);
	if (validate_process(child_array[j]) == -1){
	    child_array[j] = -1;
            num_invalid += 1;
	}
        strcpy(pid_str, "");//Reset pid_str.
        j++;
    }
    if (fclose(file) == EOF){
	fprintf(stderr, "Cannot close file\n");
	ret = 1;
	};
    if (num_invalid != 0){
	if (num_invalid == num_of_children){//No valid children, set pointer to NULL
	    free(*tuple);
	    *tuple = NULL;
	    return 0;
	}
	pid_t *valid_children = Malloc(sizeof(int) * (num_of_children - num_invalid));
	int j = 0;
	for (int i = 0; i < num_of_children; i++){
	    //Only copy the valid children into the new array.
	    if (child_array[i] != -1){
		valid_children[j] = child_array[i];
	    	j++;
	    }
	}
	free(child_array);//Free old array.
        (*tuple) -> child_array = valid_children;
        (*tuple) -> size = num_of_children - num_invalid;
	return ret;
    }
    (*tuple) -> child_array = child_array;
    (*tuple) -> size = num_of_children;
    return ret;
}

/*Sets name pointer to point to the name of the 
 *executable if it exists.
 *Return 1 if a call failed else return 0.
 */
int extract_name(char **name_pointer, pid_t pid){
    char path_to_cmdline[MAX_PATH_LENGTH + 1];
    if(sprintf(path_to_cmdline, "%s/%d/cmdline", PROC_ROOT, pid) <= 0){
    	fprintf(stderr, "Sprintf failed\n");
	free(*name_pointer);//Free's the allocated memory.
        (*name_pointer) = NULL;
        return 1;
    }
    FILE *file = fopen(path_to_cmdline, "r");
    if (file == NULL){
        fprintf(stderr, "Cannot open file: %s\n", path_to_cmdline);
	free(*name_pointer);
        (*name_pointer) = NULL;
        return 1;
    }
    int num_read = fscanf(file, "%s", *name_pointer);
    if (num_read <= 0){//cmdline is empty.
	if (fclose(file) == EOF){
	    fprintf(stderr,"Cannot close file\n");
            free(*name_pointer);
            *name_pointer = NULL;
            return 1;
	}
	free(*name_pointer);
	*name_pointer = NULL;
	return 0;
    }
    if (fclose(file) == EOF){//In this case don't set name_pointer to null.
        fprintf(stderr,"Cannot close file\n");
        return 1;
    }
    return 0;
}

/*Recursively builds tree rooted at root. If an error is encountered, it
 *returns 1. Otherwise 0 is returned.
 */
int build_node(struct TreeNode *root,  pid_t *siblings, int num_of_siblings){
    int static ret = 0;//Keeps track of error codes.
    char *name_pointer = Malloc((sizeof(char) * MAX_PATH_LENGTH) + 1);
    if(extract_name(&name_pointer, root -> pid) == 1){
	ret = 1;//extract_name failed.
    }
    (root) -> name = name_pointer;
    if(num_of_siblings == 0){
        (root) -> sibling = NULL;
    }
    else{
        struct TreeNode *sibling_pointer = Malloc(sizeof(struct TreeNode));
        sibling_pointer -> pid = siblings[0];
        if(num_of_siblings == 1){//Then its sibling will have no siblings.
	    build_node(sibling_pointer, NULL, 0);
            (root) -> sibling = sibling_pointer;
        }
        else{
	    //In this case, root has 2 or more siblings, so it's sibling
	    //will have at least one sibling. So pass in pointer of root's
	    //second sibling as the sibling parameter.
	    build_node(sibling_pointer, &siblings[1], num_of_siblings - 1);
            (root) -> sibling = sibling_pointer;
        }
    }
    struct Tuple *child_tuple = Malloc(sizeof(struct Tuple));
    if (parse_valid_children(&child_tuple, root -> pid) == 1){
	ret = 1;//parse_valid_children failed.
    }
    if (child_tuple == NULL){
        (root) -> child = NULL;
    }
    else{
	struct TreeNode *child_pointer = Malloc(sizeof(struct TreeNode));
        child_pointer -> pid = (child_tuple -> child_array[0]);
	if ( (child_tuple -> size) == 1){//It's the only child, so it has no siblings
	    build_node(child_pointer, NULL, 0);
            (root) -> child = child_pointer;
	}
	else{
            //In this case, root has 2 or more children, so it's first child
            //will have at least one sibling. So pass in pointer of root's
            //second child as the sibling parameter.
            build_node(child_pointer , &(child_tuple -> child_array[1]), (child_tuple -> size) - 1);
	    (root) -> child = child_pointer;
	}
    }
    return ret;
}

/*
 * Creates a PTree rooted at the process pid.
 * The function returns 0 if the tree was created successfully 
 * and 1 if the tree could not be created or if at least
 * one PID was encountered that could not be found or was not an 
 * executing process.
 */
int generate_ptree(struct TreeNode **root, pid_t pid) {
    int ret = 0;
    if (validate_process(pid) == -1){
	return 1;
    }
    *root = Malloc(sizeof(struct TreeNode));
    (*root) -> pid = pid;
    char *name_pointer = Malloc((sizeof(char) * MAX_PATH_LENGTH) + 1);;
    if (extract_name(&name_pointer,  pid) == 1){
	ret = 1;//extract_name failed.
    }
    (*root) -> name = name_pointer;
    (*root) -> sibling = NULL;//The root doesn't have siblings.
    struct Tuple *child_tuple = Malloc(sizeof(struct Tuple));
    if (parse_valid_children(&child_tuple , pid) == 1){
	ret = 1;//parse_valid_children failed.
    }
    if (child_tuple == NULL){
        (*root) -> child = NULL;
        return ret;
    }
    struct TreeNode *child_pointer = Malloc(sizeof(struct TreeNode));
    child_pointer -> pid = (child_tuple -> child_array[0]);
    if ((child_tuple -> size) == 1){//Only one child, so its first child will have 0 siblings.
	if (build_node(child_pointer, NULL, 0) == 1){
	    ret = 1;//build_node failed.
	}
	(*root) -> child = child_pointer;
        return ret;
    }
    //In this case, root's child has more than one sibling, so pass in the pointer to root's second 
    //child as the sibling parameter.
    if (build_node(child_pointer, &(child_tuple -> child_array[1]), (child_tuple -> size) - 1) == 1){
	ret = 1;
    }
    (*root) -> child = child_pointer;
    return ret;
}
/*
 * Prints the TreeNodes encountered on a preorder traversal of an PTree
 * to a specified maximum depth. If the maximum depth is 0, then the 
 * entire tree is printed.
 */
void print_ptree(struct TreeNode *root, int max_depth) {
    static int depth = 0;
    int temp = depth;//Keeps track of current depth.
    static int print_all = 0; // If max_depth is 0, this is set to 1 (true in boolean terms).
    if (max_depth == 0){
        print_all = 1;
    }
    //Print each node differently depending on whether the name
    //of the executable exists or not.
    if (root != NULL){
        if(root->name != NULL){
            printf("%*s%d: %s\n",  depth * 2, "", root->pid, root->name);
        }
        else{
            printf("%*s%d\n", depth * 2, "", root->pid);
        }

        //Print the child if it exists and the max_depth threshold
        //hasn't been reached yet or if the user indicated that
        //the entire tree should be printed.
        if (root->child != NULL && ((depth < max_depth) || print_all)){
            depth++; //Keeps track of depth.
            print_ptree(root->child, max_depth);
        }
        //Print the sibling if it exists.
        if (root->sibling != NULL){
	    depth = temp;
            print_ptree(root->sibling, max_depth);
        }
    }
}

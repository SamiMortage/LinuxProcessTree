#include <stdio.h>
#include <stdlib.h>

#include "ptree.h"

void print_ptree(struct TreeNode *root, int max_depth) {
    static int depth = 0;
    static int print_all = 0;
    if (max_depth == 0){
        print_all = 1;
    }
    if (root != NULL){
        if(root->name != NULL){
            printf("%*s%d: %s\n",  depth * 2, "", root->pid, root->name);
        }
        else{
            printf("%*s%d\n", depth * 2, "", root->pid);
        }
    }
    if (root->child != NULL && ((depth < max_depth) || print_all)){
        depth++;
	print_ptree(root->child, max_depth);
    }
    if (root->sibling != NULL){
        print_ptree(root->sibling, max_depth);
    }


}

int main(int argc, char *argv[]) {
    // Creates a ptree to test printing
    // Notice that in this tree the names are string literals. This is fine for
    // testing but is not what the assignment asks you to do in generate_ptree.
    // Read the handout carefully. 
    struct TreeNode root, child_one, child_two, grandchild;
    root.pid = 4511;
    root.name = "root process";
    root.child = &child_one;
    root.sibling = NULL;

    child_one.pid = 4523;
    child_one.name = "first child";
    child_one.child = NULL;
    child_one.sibling = &child_two;

    child_two.pid = 4524; 
    child_two.name = "second child";
    child_two.child = &grandchild;
    child_two.sibling = NULL;

    grandchild.pid = 4609;
    grandchild.name = "grandchild";
    grandchild.child = NULL;
    grandchild.sibling = NULL;

    print_ptree(&root, 1);

    return 0;
}


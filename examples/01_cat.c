#include <stdio.h>

#define EXTLIB_IMPL
#include "../extlib.h"

int main(int argc, char** argv) {
    if(argc <= 1) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return 1;
    }
    char* filename = argv[1];
    StringBuffer sb = {0};
    if(!read_entire_file(filename, &sb)) return 1;

    // Could just print the whole file at once, but this is useful to demonstrate how to split a
    // string buffer
    StringSlice ss = sb_to_ss(sb);
    while(ss.size) {
        StringSlice line = ss_split_once(&ss, '\n');
        printf(SS_Fmt "\n", SS_Arg(line));
    }

    sb_free(&sb);
}

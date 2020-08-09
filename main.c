#include "preprocess.h"
/*#include "compile.h"*/
#include "container.h"

#include "utilities.h"


#define SOMETHING_INTERESTING is about>to< <<happen>>here

int main(int argc, char **argv) {
    list *lines;
    preprocessor_state *state = preprocess_init();
    if (preprocess(state, argv[1])) {
        return -1;
    }

    print_lines(state->lines);
    /* print_macros(state->macros); */

    return 0;
}


#include "errors.h"

char *error_strings[MAX_ERROR_NUMBER] = {
    "No error.",
    "Generic error.",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Division by zero in preprocessor expression.",
    "Exhausted RPN stack during preprocessor expression.",
    "Invalid operator in preprocessor expression.",
    "Unexpected number in preprocessor expression.",
    "Mismatched parenthesis in preprocessor expression.",
    "",
    "",
    "Floating constant in preprocessor expression",
    "Invalid digits in octal integer constant in preprocessor expression.",
    "Invalid integer suffix in preprocessor expression."
};

char *get_error_string(int8_t error_number) {
    return error_strings[-error_number];
}

cmake_minimum_required(VERSION 3.10)

project(ccnp VERSION 0.1)

set(CMAKE_C_FLAGS "-pedantic -Werror")

add_executable(ccnp main.c preprocess.c compile.c container.c message.c prep_eval.c utilities.c)

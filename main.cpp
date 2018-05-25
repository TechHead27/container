extern "C" {
#include <argp.h>
}

#include <cstdio>
#include <cstdlib>
#include <vector>

//#include "UserMount.hpp"

struct Options {
    unsigned int time;
    bool net;
    char *program;
    std::vector<char*> args;
};

error_t parse_arg(int key, char *arg, struct argp_state *state) {
    struct Options *output = (struct Options*)state->input;

    switch (key) {
        case 't':
            output->time = atoi(arg);
            break;
        case 'n':
            output->net = true;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0)
                output->program = arg;
            else
                output->args.push_back(arg);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

struct Options processOptions(int argc, char **argv) {
    struct argp parser;
    struct argp_option options[3];
    struct Options result = { 0 };

    options[0] = {"time", 't', "limit", 0, "Execution time allowed"};
    options[1] = {"network", 'n', NULL, 0, "Allow network access"};
    options[2] = { 0 };
    
    parser = {options, parse_arg, "command args..."};
    argp_parse(&parser, argc, argv, 0, NULL, &result);
    return result;
}

void test_args(struct Options *opts) {
    printf("Run %s for %u seconds with %i network acess with arguments ",
        opts->program, opts->time, opts->net);
}

int main(int argc, char **argv) {
    struct Options opts = processOptions(argc, argv);
    test_args(&opts);
    //UserMount um(opts);

    //um.start();
    //um.wait();

    return 0;
}

#include "args.h"
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
ARGUMENT        MEANING
-i              initialize a playground
-r              run a playground
-k <value>      size of the square playground (used when -w and -h are not specified)
-w <value>      width of the playground (when used -k is not specified)
-h <value>      height of the playground (when used -k is not specified)
-e [0|1|2|3]    evolution type; 0="ordered", 1="static", 2="wave", 3="white-black"
-f <string>     the name of the file to be either read or written
-n <value>      number of steps to be calculated
-s <value>      every how many steps a dump of the system is saved on a file (0 meaning only at the end)
-g <p_l>        enable FoG, setting p_L (probability of setting the selected cell alive) and p_D = 1 - p_L
-b              enable benchmark mode to time execution
*/

#define DEFAULT_PLAYGROUND_SIZE 100
#define DEFAULT_NUMBER_OF_STEPS 10000
#define DEFAULT_DUMP_FREQUENCY 1
#define OUTPUT_DIRECTORY "patterns/output/"
#define SNAPSHOT_SUFFIX "_"

/// converts a string to an integer and checks for invalid input
static int parse_integer(const char *value, int *result)
{
    char *end;
    errno = 0;
    long number = strtol(value, &end, 10);

    if (errno != 0 || end == value || *end != '\0' ||
        number < INT_MIN || number > INT_MAX)
        return -1;

    *result = (int)number;
    return 0;
}

/// converts a string to a double and checks for invalid input
static int parse_double(const char *value, double *result)
{
    char *end;
    errno = 0;
    double number = strtod(value, &end);

    if (errno != 0 || end == value || *end != '\0')
        return -1;

    *result = number;
    return 0;
}

/// builds the filename of a pattern snapshot
char *build_snapshot_filename(const char *pattern_name, int step)
{
    size_t length = strlen(OUTPUT_DIRECTORY) + strlen(pattern_name) +
                    strlen(SNAPSHOT_SUFFIX) + 5 + strlen(".pgm") + 1;
    char *filename = malloc(length);

    if (filename == NULL)
        return NULL;

    snprintf(filename, length, "%s%s%s%05d.pgm",
             OUTPUT_DIRECTORY, pattern_name, SNAPSHOT_SUFFIX, step);

    return filename;
}

/// prints an argument error, frees allocated memory and returns failure
static int argument_error(arguments_t *args, const char *message)
{
    fprintf(stderr, "error: %s\n", message);
    free(args->pattern_name);
    args->pattern_name = NULL;
    return -1;
}

int parse_arguments(int argc, char **argv, arguments_t *args)
{
    int c, width_set = 0, height_set = 0, size_set = 0;

    *args = (arguments_t){
        .action = 0,
        .width = DEFAULT_PLAYGROUND_SIZE,
        .height = DEFAULT_PLAYGROUND_SIZE,
        .evolution = ORDERED,
        .steps = DEFAULT_NUMBER_OF_STEPS,
        .dump_frequency = DEFAULT_DUMP_FREQUENCY,
        .pattern_name = NULL,
        .benchmark = 0,
        .fog_enabled = 0,
        .p_l = 0.0
    };

    optind = 1; // global variable for getopt() to keep track of which argv is currently parsed

    while ((c = getopt(argc, argv, "irk:w:h:e:f:n:s:bg:")) != -1) {
        switch (c) {
        case 'i':
            if (args->action) return argument_error(args, "-i and -r are mutually exclusive");
            args->action = INIT;
            break;
        case 'r':
            if (args->action) return argument_error(args, "-i and -r are mutually exclusive");
            args->action = RUN;
            break;
        case 'k':
            if (size_set || width_set || height_set)
                return argument_error(args, "-k cannot be combined with -w or -h");
            if (parse_integer(optarg, &args->width) != 0 || args->width < 100)
                return argument_error(args, "playground size must be at least 100");
            args->height = args->width;
            size_set = 1;
            break;
        case 'w':
            if (size_set)
                return argument_error(args, "-w cannot be combined with -k");
            if (parse_integer(optarg, &args->width) != 0 || args->width < 100)
                return argument_error(args, "width must be at least 100");
            width_set = 1;
            break;
        case 'h':
            if (size_set)
                return argument_error(args, "-h cannot be combined with -k");
            if (parse_integer(optarg, &args->height) != 0 || args->height < 100)
                return argument_error(args, "height must be at least 100");
            height_set = 1;
            break;
        case 'e':
            if (parse_integer(optarg, &args->evolution) != 0 ||
                (args->evolution != ORDERED && args->evolution != STATIC &&
                 args->evolution != WAVE && args->evolution != WHITE_BLACK))
                return argument_error(args, "evolution must be 0, 1, 2 or 3");
            break;
        case 'f':
            free(args->pattern_name);
            args->pattern_name = malloc(strlen(optarg) + 1);
            if (args->pattern_name == NULL)
                return argument_error(args, "memory allocation failed");
            strcpy(args->pattern_name, optarg);
            break;
        case 'n':
            if (parse_integer(optarg, &args->steps) != 0 || args->steps < 0)
                return argument_error(args, "invalid number of steps");
            break;
        case 's':
            if (parse_integer(optarg, &args->dump_frequency) != 0 || args->dump_frequency < 0)
                return argument_error(args, "invalid dump frequency");
            break;
        case 'b':
            args->benchmark = 1;
            break;
        case 'g':
            if (parse_double(optarg, &args->p_l) != 0 ||
                args->p_l < 0.0 || args->p_l > 1.0)
                return argument_error(args, "p_L must be between 0 and 1");

            args->fog_enabled = 1;
            break;
        default:
            return argument_error(args, "unknown or incomplete option");
        }
    }

    if (optind < argc) return argument_error(args, "unexpected argument");
    if (args->action == 0) return argument_error(args, "specify either -i or -r");
    if (width_set != height_set) return argument_error(args, "-w and -h must be specified together");
    if (args->pattern_name == NULL) return argument_error(args, "pattern name is required (-f)");
    if (args->action == RUN && (size_set || width_set || height_set))
        return argument_error(args, "-k, -w and -h are only valid with -i");

    return 0;
}

void free_arguments(arguments_t *args)
{
    free(args->pattern_name);
    args->pattern_name = NULL;
}
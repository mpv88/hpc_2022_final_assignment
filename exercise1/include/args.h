/// \file args.h
/// \author mpv
/// \brief configuration options for the GoL parsed from command-line arguments.

#ifndef GOL_ARGS_H
#define GOL_ARGS_H

/// \brief Identifies the operation requested by the user.
enum action {
    INIT = 1,
    RUN = 2
};

/// \brief Identifies the evolution algorithm.
enum evolution_type {
    ORDERED = 0,
    STATIC = 1,
    WAVE = 2,
    WHITE_BLACK = 3
};

/// \brief Stores the command-line arguments used by the program.
typedef struct {
    enum action action;
    int width;
    int height;
    int evolution;
    int steps;
    int dump_frequency;
    char *pattern_name;
    int benchmark;
} arguments_t;

/// \brief Parses and validates the command-line arguments.
/// \param argc number of command-line arguments.
/// \param argv array of command-line argument strings.
/// \param args structure in which the parsed arguments are stored.
/// \return 0 on success, -1 if the arguments are invalid.
int parse_arguments(int argc, char **argv, arguments_t *args);

/// \brief Builds the filename of a pattern snapshot.
/// \param pattern_name name of the pattern.
/// \param step evolution step of the snapshot.
/// \return allocated string containing the snapshot filename, or NULL on error.
char *build_snapshot_filename(const char *pattern_name, int step);

/// \brief Frees memory allocated for command-line arguments.
/// \param args structure containing the arguments to free.
void free_arguments(arguments_t *args);

#endif
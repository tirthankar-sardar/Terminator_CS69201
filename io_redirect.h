
#ifndef IO_REDIRECT_H
#define IO_REDIRECT_H

typedef struct
{
    char *input_file;
    char *output_file;
    int append_output;
} IORedirection;

int parse_redirections(char *argv[], IORedirection *redirect);

#endif

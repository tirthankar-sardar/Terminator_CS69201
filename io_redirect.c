
#include <string.h>
#include "io_redirect.h"

int parse_redirections(char *argv[], IORedirection *redirect)
{
    int read_pos = 0;
    int write_pos = 0;

    redirect->input_file = NULL;
    redirect->output_file = NULL;
    redirect->append_output = 0;

    while (argv[read_pos] != NULL)
    {
        if (strcmp(argv[read_pos], "<") == 0 ||
            strcmp(argv[read_pos], ">") == 0 ||
            strcmp(argv[read_pos], ">>") == 0)
        {
            int is_input = strcmp(argv[read_pos], "<") == 0;
            int is_append = strcmp(argv[read_pos], ">>") == 0;

            if (argv[read_pos + 1] == NULL ||
                argv[read_pos + 1][0] == '\0' ||
                strcmp(argv[read_pos + 1], "<") == 0 ||
                strcmp(argv[read_pos + 1], ">") == 0 ||
                strcmp(argv[read_pos + 1], ">>") == 0)
                return -1;

            if (is_input)
            {
                if (redirect->input_file != NULL)
                    return -1;

                redirect->input_file = argv[read_pos + 1];
            }
            else
            {
                if (redirect->output_file != NULL)
                    return -1;

                redirect->output_file = argv[read_pos + 1];
                redirect->append_output = is_append;
            }

            read_pos += 2;
            continue;
        }

        argv[write_pos++] = argv[read_pos++];
    }

    argv[write_pos] = NULL;
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "recorders.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

struct options {
        options() {
                filename[0] = 0;
                comment[0] = 0;
        }
        char filename[FILENAME_MAXLEN];
        char comment[COMMENT_MAXLEN];
};

void parseArgs(int argc, char *argv[], options *opts)
{
        po::options_description description("Usage: lcg-annotate [options].\nAllowed options");
        po::variables_map options;
        std::string filename, message;

        description.add_options()("help,h", "print help message")
        ("file,f", po::value<std::string>(&filename), "annotate filename")
        ("message,m", po::value<std::string>(&message), "comment");

        try {
                po::store(po::parse_command_line(argc, argv, description), options);
                po::notify(options);    
                if (options.count("help")) {
                        std::cout << description << "\n";
                        exit(0);
                }
                if (options.count("file"))
                        strncpy(opts->filename, filename.c_str(), FILENAME_MAXLEN);
                if (options.count("message"))
                        strncpy(opts->comment, message.c_str(), COMMENT_MAXLEN);
        } catch(std::exception e) {
                fprintf(stderr, "Missing argument or unknown option.\n");
                exit(1);
        }
}

int writeComment(const char *filename, const char *comment)
{
        return 0;
}

int main(int argc, char *argv[])
{
        char c;
        options opts;
        parseArgs(argc, argv, &opts);

        if (!strlen(opts.filename)) {
                fprintf(stdout, "Enter the name of the file you want to annotate: ");
                fscanf(stdin, "%s", opts.filename);
                fflush(stdin);
        }

        if (!strlen(opts.comment)) {
                fprintf(stdout, "Enter the comment: ");
                fscanf(stdin, "%s", opts.comment);
                fflush(stdin);
        }

        return writeComment(opts.filename, opts.comment);
}


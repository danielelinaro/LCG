#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"
#include "recorders.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace lcg;

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

herr_t counter(hid_t location_id /*in*/, const char *attr_name /*in*/,
                const H5A_info_t *ainfo /*in*/, void *op_data /*in,out*/) {
        int next = *((int*)op_data);
        int current = atoi(attr_name);
        if (current >= next)
                next = current+1;
        *((int*)op_data) = next;
        return 0;
}

int writeComment(const char *filename, const char *comment)
{
        struct stat buf;

        if (stat(filename, &buf) < 0) {
                Logger(Critical, "%s: no such file.\n", filename);
                return -1;
        }

        hid_t fid, grp, dspace, atype, attr;
        hsize_t n = 0;
        herr_t status;
        int next = -1;
        char tag[4];

        // open the file
        fid = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
        if (fid < 0) {
                Logger(Critical, "%s: unable to open file.\n", filename);
                return -1;
        }
        Logger(Debug, "Successfully opened HDF5 file %s.\n", filename);

        // open the comments group
        grp = H5Gopen(fid, COMMENTS_GROUP, H5P_DEFAULT);
        if (grp < 0) {
                Logger(Critical, "%s: unable to open group.\n", COMMENTS_GROUP);
                H5Fclose(fid);
                return -1;
        }
        Logger(Debug, "Successfully opened comments group.\n");

        // find out how many comments are already present
        status = H5Aiterate2(grp, H5_INDEX_NAME, H5_ITER_INC, &n, counter, (void *) &next);
        if (status < 0) {
                Logger(Critical, "Unable to iterate over all attributes.\n");
                H5Gclose(grp);
                H5Fclose(fid);
        }
        sprintf(tag, "%03d", next);
        Logger(Debug, "The next attribute will have tag [%s]\n", tag);

        // create a dataspace for the comment
        dspace = H5Screate(H5S_SCALAR);
        if (dspace < 0) {
                Logger(Critical, "Error in H5Screate.\n");
                H5Gclose(grp);
                H5Fclose(fid);
                return -1;
        }

        atype = H5Tcopy(H5T_C_S1);
        if (atype < 0) {
                Logger(Critical, "Error in H5Tcopy.\n");
                H5Sclose(dspace);
                H5Gclose(grp);
                H5Fclose(fid);
                return false;
        }
        Logger(Debug, "Successfully copied type.\n");

        status = H5Tset_size(atype, strlen(comment) + 1);
        if (status < 0) {
                Logger(Critical, "Error in H5Tset_size.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                H5Gclose(grp);
                H5Fclose(fid);
                return false;
        }
        Logger(Debug, "Successfully set type size.\n");

        status = H5Tset_strpad(atype, H5T_STR_NULLTERM);
        if (status < 0) {
                Logger(Critical, "Error in H5Tset_strpad.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                H5Gclose(grp);
                H5Fclose(fid);
                return false;
        }
        Logger(Debug, "Successfully set type padding string.\n");

        // create an attribute to the comments group
        attr = H5Acreate2(grp, tag, atype, dspace, H5P_DEFAULT, H5P_DEFAULT);
        if (attr < 0) {
                Logger(Critical, "Error in H5Acreate2.\n");
                H5Tclose(atype);
                H5Sclose(dspace);
                H5Gclose(grp);
                H5Fclose(fid);
                return false;
        }
        Logger(Debug, "Successfully created attribute.\n");

        // write the attribute
        status = H5Awrite(attr, atype, comment);
        if (status < 0)
                Logger(Critical, "Error in H5Awrite.\n");
        else
                Logger(Debug, "Successfully written string attribute.\n");

        H5Aclose(attr);
        H5Tclose(atype);
        H5Sclose(dspace);
        H5Gclose(grp);
        H5Fclose(fid);
        
        return (int) status;
}

int main(int argc, char *argv[])
{
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

        // add a timestamp to the comment
        recorders::Comment comment(opts.comment);

        return writeComment(opts.filename, comment.message());
}


/**
 * File:   make_package.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Build package
 *
 * Created on August 24, 2015, 2:22 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../src/config/conf.h"
#include "../src/config/config.h"

#define TBP_VERSION "0.0.2"

#ifndef LIBRARY_MAJOR_VERSION
#define LIBRARY_MAJOR_VERSION 0
#endif

/**
 * Build type
 */
enum B_TYPE {

    DEB = 1,    ///< Debian
    RPM,        ///< REHL/Centos/Fedora/Suse RPM created under Ubuntu
    YUM,
    ZYP
};

/**
 * Show application usage
 *
 * @param appname
 */
void show_usage(const char* appname) {

    printf( "\n"
        "Usage: %s LINUX [ARCH]\n"
        "\n"
        "Where LINUX: deb - DEBIAN, "
                     "rpm - RPM for Ubuntu, "
                     "yum - REHL/Centos/Fedore, zyp - Opensuse\n"
        "      ARCH: architecture (default: amd64)\n"
        "\n"
        , appname);

}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {

    printf("Teonet build package ver. %s, %s\n", TBP_VERSION, COPYRIGHT);

    int rv = EXIT_FAILURE; // Return value
    int b_type = 0; // Build type

    // Show CI_BUILD_REF
    {
        char *CI_BUILD_REF = getenv("CI_BUILD_REF");
        if(CI_BUILD_REF != NULL)
            printf("CI_BUILD_REF=%s\n", CI_BUILD_REF);
    }

    // Check for required arguments
    if(argc < 2) {
        show_usage(argv[0]);
        return (EXIT_FAILURE);
    }

    // Make DEBIAN repository
    if(!strcmp(argv[1], "deb")) b_type = DEB;
    else if(!strcmp(argv[1], "rpm")) b_type = RPM;
    else if(!strcmp(argv[1], "yum")) b_type = YUM;
    else if(!strcmp(argv[1], "zyp")) b_type = ZYP;

    // Check for build type
    if(!b_type) {
        show_usage(argv[0]);
        return (EXIT_FAILURE);
    }

    // DEB specific
    if(b_type == 1) {

        // Import repository keys
        if(system("sh/make_deb_keys_add.sh")) return (EXIT_FAILURE);
    }

    // Get build ID from GitLab CI environment variable
    char version[KSN_BUFFER_SM_SIZE];
    char *CI_BUILD_ID = getenv("CI_BUILD_ID");
    char *CIRCLE_BUILD_NUM = getenv("CIRCLE_BUILD_NUM");

    // Get version from Teonet configuration header file
    snprintf(version, KSN_BUFFER_SM_SIZE, "%s", VERSION);

    // Execute build packet script
    char cmd[KSN_BUFFER_SM_SIZE];
    snprintf(cmd, KSN_BUFFER_SM_SIZE, "sh/make_%s.sh %s %d %d.0.0 %s %s %s",
            b_type == DEB ? argv[1] : "rpm",  // 0
            // Script parameters
            version,                // $1 Version
            LIBRARY_MAJOR_VERSION,  // $2 Library major version
            LIBRARY_MAJOR_VERSION,  // $3 Library version
            CI_BUILD_ID != NULL ? CI_BUILD_ID : (CIRCLE_BUILD_NUM != NULL ? CIRCLE_BUILD_NUM : "1"), // $4 Build
            argc >= 3 ? argv[2] : b_type == DEB ? "amd64" : "x86_64", // $5 Architecture
            b_type > DEB ? argv[1] : "deb" // $6 RPM subtype
            //"", // $7 Package name (default: libteonet)
            //""  // $8 Package description (default: ...)
    ) < 0 ? abort() : (void)0;

    rv = system(cmd);

    return rv != 0;
}

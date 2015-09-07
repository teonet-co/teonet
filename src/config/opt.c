/**
 * Read command line options
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#ifndef HAVE_MINGW
#include <syslog.h>
#endif
#include <libgen.h>
#include <sys/stat.h>
#ifdef HAVE_MINGW
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include "config/config.h"
#include "utils/utils.h"
#include "config/conf.h"
#include "daemon.h"
#include "ev_mgr.h"

void opt_usage(char *app_name, int app_argc, char** app_argv);

/**
 * Read configuration and command line options
 *
 * This function should be called to read configuration and command line 
 * options
 *
 * @param argc Number of command line arguments
 * @param argv String array with command line arguments
 * @param conf Pointer to ksnet_cfg structure to read configuration and
 *             command line parameters to
 * @param app_argc Number of application arguments
 * @param app_argv String array with application argument names
 * @param show_arg Show arguments 
 *
 * @return Entered application arguments
 */
char ** ksnet_optRead(int argc, char **argv, ksnet_cfg *conf,
        int app_argc, char **app_argv, int show_arg) {

    int option_index = 0, opt;
    struct option loptions[] = {
        { "help",           no_argument,       0, 'h' },
        { "version",        no_argument,       0, 'v' },
        { "app_name",       no_argument,       0,  0  },
        { "app_description",no_argument,       0,  0  },
        { "uuid",           no_argument,       0,  0  },
        { "port",           required_argument, 0, 'p' },
        { "port_increment", no_argument,       &conf->port_inc_f, 1 },
        { "r_port",         required_argument, 0, 'r' },
        { "r_address",      required_argument, 0, 'a' },
        { "network",        required_argument, 0, 'n' },
        { "hot_keys",       no_argument,       &conf->hot_keys_f, 1 },
        { "show_debug",     no_argument,       &conf->show_debug_f, 1 },
        { "show_debug_vv",  no_argument,       &conf->show_debug_vv_f, 1 },
        { "show_connect",   no_argument,       &conf->show_connect_f, 1 },
        { "show_peers",     no_argument,       &conf->show_peers_f, SHOW_PEER_CONTINUOSLY },
        { "show_tr_udp",    no_argument,       &conf->show_tr_udp_f, SHOW_PEER_CONTINUOSLY },
        #if M_ENAMBE_VPN
        { "vpn_start",      no_argument,       &conf->vpn_connect_f, 1 },
        { "vpn_ip",         required_argument, 0, 'i' },
        { "vpn_mtu",        required_argument, 0, 'm' },
        #endif
        { "daemon",         no_argument,       &conf->dflag, 1 },
        { "kill",           no_argument,       &conf->kflag, 1 },

        { 0, 0, 0, 0 }
    };
    static const char *data_path = NULL;

    // Get data folder path
    if(data_path == NULL) {

        data_path = getDataPath();
        #ifdef DEBUG_KSNET
        printf("current data path: %s\n", data_path);
        #endif
    }

    // Create data folder (if absent)
    #ifdef HAVE_MINGW
    mkdir(data_path);
    #else
    mkdir(data_path, 0755);
    #endif

    // initialize random seed
    srand(time(NULL));

    // Read options

    // Initialize getopt()
    optind = 0;

    // Check online parameters
    for(;;) {

      opt = getopt_long (argc, argv, "?hv:p:r:a:dkn:", loptions,
                         &option_index);
      if (opt==-1)
      {
          //usage();
          //exit(EXIT_SUCCESS);
          break;
      }
      switch (opt) {

        case 0:
          if(!strcmp(loptions[option_index].name, "app_name")) {
              printf("Name of this application is: %s\n\n",
                     conf->app_name);
              exit(EXIT_SUCCESS);
          }

          else if(!strcmp(loptions[option_index].name, "app_desc")) {
              printf("Application description:\n%s\n\n",
                     conf->app_description);
              exit(EXIT_SUCCESS);
          }

          else if(!strcmp(loptions[option_index].name, "uuid")) {

              printf("UUID: ");
              #ifndef HAVE_MINGW
              size_t i;
              char pl = 4;
              uuid_t uuid;
              uuid_generate(uuid);
              for(i = 0; i < sizeof uuid; i ++) {
                  if(i && i <= 10 && !(i % pl)) {
                      printf("-");
                      pl = 2;
                  }
                  printf("%02x", uuid[i]);
              }
              #else
              UUID uuid;
              unsigned char *uuid_str;
              UuidCreateNil(&uuid);
              UuidCreate(&uuid);
              UuidToString(&uuid, &uuid_str);
              printf("%s", uuid_str);
              RpcStringFree(&uuid_str);
              #endif
              printf("\n\n");
              exit(EXIT_SUCCESS);
          }

          break;

        case '0': break;

        case '?':
        case 'h':
          opt_usage(argv[0], app_argc, app_argv);
          exit(EXIT_SUCCESS);
          break;

        case 'v':
          printf("The current version of the %s is %s\n\n",
                 PACKAGE_NAME, VERSION);
          exit(EXIT_SUCCESS);
          break;

        case 'p':
          conf->port = atoi(optarg);
          break;

        case 'r':
          conf->r_port = atoi(optarg);
          break;

        case 'a':
          strncpy((char*)conf->r_host_addr, optarg, KSN_BUFFER_SM_SIZE/2);
          break;

        case 'i':
          strncpy((char*)conf->vpn_ip, optarg, KSN_BUFFER_SM_SIZE/2);
          break;

        case 'm':
          conf->vpn_mtu = atoi(optarg);
          break;

        case 'n':
          strncpy((char*)conf->network, optarg, KSN_BUFFER_SM_SIZE/2);
          break;
          
        case 'd':
          // Start this application in Daemon mode
          conf->dflag = 1;
          break;

        case 'k':
          // Kill application started in Daemon mode
          conf->kflag = 1;
          break;
      }
    }

    // Check for arguments
    if((argc - optind) < app_argc) {

        int i;

        fprintf(stderr, "Expected argument%s:",
                ((app_argc - (argc - optind)) > 1 ? "s" : ""));
        // Show expected arguments
        for(i = argc - optind; i < app_argc; i++) {
            printf(" %s", app_argv[i]);
        }
        printf("\n\n");
        opt_usage(argv[0], app_argc, app_argv);
        exit(EXIT_FAILURE);
    }
    else if((argc - optind) > app_argc) {

        int i, extraind = optind + app_argc;
        fprintf(stderr, "Extra argument%s:", (argc - extraind > 1 ? "s" : ""));
        // Show extra arguments
        for(i = extraind; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        printf("\n\n");
        opt_usage(argv[0], app_argc, app_argv);
        exit(EXIT_FAILURE);
    }

    // Set Host name
    strncpy(conf->host_name, argv[optind], KSN_MAX_HOST_NAME);

    // Show arguments
    if(show_arg) {

        int i;
        for(i = 0; i < app_argc; i++) {
            printf("%s: %s\n", app_argv[i], argv[optind+i]);
        }
        printf("\n");
    }      

    #ifndef HAVE_MINGW
    // Start or stop application in daemon mode
    start_stop_daemon(argv, conf);
    #endif

    // Change configuration for daemon mode
    if(conf->dflag) {
        conf->show_connect_f = 0;
        conf->show_debug_f = 0;
        conf->hot_keys_f = 0;
    }    

    return &argv[optind];
}

/**
 * Show online parameters usage help
 */
void opt_usage(char *app_name, int app_argc, char** app_argv) {

    int i;

    // Create arguments string
    char app_argv_str[KSN_BUFFER_SIZE], *app_name_cpy = strdup(app_name);
    app_argv_str[0] = 0;
    for(i = 0; i < app_argc; i++) {
        strncat(app_argv_str, " ", KSN_BUFFER_SIZE - strlen(app_argv_str) - 1);
        strncat(app_argv_str, app_argv[i], KSN_BUFFER_SIZE - strlen(app_argv_str) - 1);
    }

    printf("Usage: %s [OPTIONS]%s\n"
    "\n"
    "Options:\n"
    "\n"
    "  -h, --help               SHow this help message\n"
    "  -v, --version            Show current version\n"
    "      --uuid               Generate new uuid\n"
    "      --app_name           Show this application name\n"
    "      --app_description    Show this application description\n"
    "  -n, --network=value      Set network name to connect to\n"
    "  -p, --port=value         Set port number (default "KSNET_PORT_DEFAULT")\n"
    "      --port_increment     Increment port if busy\n"
    "  -a, --r_address=value    Set remote server address (default localhost)\n"
    "  -r, --r_port=value       Set remote server port number (default "KSNET_PORT_DEFAULT")\n"
    "      --hot_keys           Switch on the hot keys monitor\n"
    #ifdef DEBUG_KSNET
    "      --show_debug         Show debug messages\n"
    "      --show_debug_vv      Show debug additional messages\n"
    #endif
    "      --show_connect       Show connection messages\n"
    "      --show_peers         Show peers screen after connection\n"
    "      --show_tr_udp        Show TR-UDP statistic after connection\n"
    #if M_ENAMBE_VPN
    "      --vpn_start          Start VPN\n"
    "      --vpn_ip             VPN IP\n"
    "      --vpn_mtu            VPN MTU\n"
    #endif
    "\n"
    "  -d, --daemon             Start this application in daemon mode\n"
    "  -k, --kill               Kill the application running in daemon mode\n"
    "\n",
    basename(app_name_cpy), app_argv_str);

    free(app_name_cpy);
}

/**
 * Set application name
 *
 * Set application name, prompt and description (use it before ksnet_optRead)
 */
void ksnet_optSetApp(ksnet_cfg *conf,
                     const char* app_name,
                     const char* app_prompt,
                     const char* app_description) {

    strncpy(conf->app_name, app_name, KSN_BUFFER_SM_SIZE/2);
    strncpy(conf->app_prompt, app_prompt, KSN_BUFFER_SM_SIZE/2);
    strncpy(conf->app_description, app_description, KSN_BUFFER_SM_SIZE/2);
}

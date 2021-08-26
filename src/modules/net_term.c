/**
 * File:   net_term.c
 * Author: Kirill Scherba <kieill@scherba.ru>
 *
 * Created on May 8, 2015, 10:52 PM
 *
 *
 * Teonet net terminal module
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "ev_mgr.h"
#include "net_cli.h"
#include "utils/rlutil.h"

#define MODULE _ANSI_LIGHTBLUE "terminal_server" _ANSI_NONE

#ifdef M_ENAMBE_TERM

// Local functions
struct cli_def *ksnTermCliInit(ksnTermClass *kter);
void ksnet_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *watcher,
                     int revents, int fd);

//#define CLI_PORT                8000

#define kev ((ksnetEvMgrClass *) (kter->ke))

/**
 * Terminal initialize
 *
 * @param ke
 * @return
 */
ksnTermClass *ksnTermInit(void *ke) {

    ksnTermClass *kter = malloc(sizeof(ksnTermClass));
    kter->ke = ke;

    // Set My context
    char *mymessage = "I contain user data!";
    kter->myctx.value = 5;
    kter->myctx.message = mymessage;

    // Check configuration, create TCP server and start Terminal telnet CLI server
    //! \todo: Check configuration
    int fd, port_created, // port = CLI_PORT,
        port = kev->teo_cfg.port;

    // Start TCP server
    if((fd = ksnTcpServerCreate(((ksnetEvMgrClass*)ke)->kt, port, ksnet_accept_cb, kter,
        &port_created)) > 0) {

        ksn_printf(kev, MODULE, MESSAGE,
                "started at port %d, socket fd %d\n", port_created, fd);

    }

    return kter;
}

/**
 * Terminal destroy and free allocated memory
 *
 * @param kte
 * @return
 */
void ksnTermDestroy(ksnTermClass *kte) {

    ksnetEvMgrClass *ke = kte->ke;
    free(kte);
    ke->kter = NULL;
}

/**
 * TCP Server accept callback
 *
 * @param loop
 * @param watcher
 * @param revents
 *
 * @param fd
 */
void ksnet_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *watcher,
                     int revents, int fd) {

    // Start CLI server
    struct cli_def *cli = ksnTermCliInit(watcher->data);
    cli_loop(cli, fd);
}

/******************************************************************************/
/* CLInterface commands callback                                              */
/******************************************************************************/

#define MODE_CONFIG_INT             10

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

int regular_callback(struct cli_def *cli) {

    cli->regular_count++;
    if (cli->debug_regular)
    {
        cli_print(cli, "Regular callback - %u times so far", cli->regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}

int idle_timeout(struct cli_def *cli) {

    cli_print(cli, "Custom idle timeout (press any key to continue)\n");
    cli_reprompt(cli);
    return CLI_OK; // CLI_QUIT;
}

/******************************************************************************/
/* KSNet commands                                                             */
/******************************************************************************/

/**
 * Show ksnet peers
 *
 * @param cli The handle of the cli structure
 * @param command The entire command which was entered. This is after command expansion
 * @param argv The list of arguments entered
 * @param argc The number of arguments entered
 * @return
 */
int cmd_peer(struct cli_def *cli, const char *command, char *argv[], int argc) {

    char *str = ksnetArpShowStr(cli->ke->kc->ka);
    cli_print(cli, "\r\n%s", str);
    free(str);

    return CLI_OK;
}

/**
 * KSNet commands group
 *
 * @param cli
 * @param command
 * @param argv
 * @param argc
 * @return
 */
int cmd_ksnet(struct cli_def *cli, const char *command, char *argv[], int argc) {

    cli_print(cli, "Teonet command usage:");
    //cli_find_command(cli, command)
    //cli_set_configmode(cli, MODE_CONFIG_INT, "ksnet");

    return CLI_OK;
}

/**
 * KSNet show command
 *
 * @param cli
 * @param command
 * @param argv
 * @param argc
 * @return
 */
int cmd_ksnet_show(struct cli_def *cli, const char *command, char *argv[], int argc) {

    cli_print(cli,
            "\nTeonet connection settings:\n"
            "-----------------------------------\n"
            "Host name:       %18s\n"
            "Host port:       %18d\n"
            "Remote address:  %18s\n"
            "Remote port:     %18d\n"
            , cli->ke->teo_cfg.host_name
            , (int) cli->ke->teo_cfg.port
            , cli->ke->teo_cfg.r_host_addr
            , (int) cli->ke->teo_cfg.r_port
    );

    return CLI_OK;
}

int cmd_ksnet_set(struct cli_def *cli, const char *command, char *argv[], int argc) {

    cli_print(cli, "\r\nSet Teonet parameters:");

    return CLI_OK;
}

int cmd_ksnet_set_peers_f(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if (argc < 1 || strcmp(argv[0], "?") == 0) {
        cli_print(cli,
        "Specify a value: 0 - don't show; 1 - show ones; 2 - continuously");
        return CLI_OK;
    }
    else {
        int show_peers_f = atoi(argv[0]);
        cli->ke->teo_cfg.show_peers_f = show_peers_f != 0;
    }

    return CLI_OK;
}

int cmd_ksnet_set_tr_udp_f(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if (argc < 1 || strcmp(argv[0], "?") == 0) {
        cli_print(cli,
        "Specify a value: 0 - don't show; 1 - show ones; 2 - continuously");
        return CLI_OK;
    }
    else {
        int show_tr_udp_f = atoi(argv[0]);
        cli->ke->teo_cfg.show_tr_udp_f = show_tr_udp_f != 0;
    }

    return CLI_OK;
}

/**
 * Set ksnet show debug messages flag
 *
 * @param cli
 * @param command
 * @param argv
 * @param argc
 * @return
 */
int cmd_ksnet_set_debug_f(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if (argc < 1 || strcmp(argv[0], "?") == 0) {
        cli_print(cli,
            "Specify show debug messages value: 0 - don't show; 1 - show\n"
            "(current value is: %d)", cli->ke->teo_cfg.show_debug_f);
        return CLI_OK;
    }
    else {
        int show_debug_f = atoi(argv[0]);
        cli->ke->teo_cfg.show_debug_f = show_debug_f != 0;
        cli_print(cli, "Teonet debug flag was set to: %d",
                  cli->ke->teo_cfg.show_debug_f);
    }

    return CLI_OK;
}

/**
 * Set ksnet extend L0 DEBUG logs flag
 *
 * @param cli
 * @param command
 * @param argv
 * @param argc
 * @return
 */
int cmd_ksnet_extend_l0_log_f(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if (argc < 1 || strcmp(argv[0], "?") == 0) {
        cli_print(cli,
            "Specify extend L0 DEBUG logs flag value: 0 - don't show; 1 - show\n"
            "(current value is: %d)", cli->ke->teo_cfg.extended_l0_log_f);
        return CLI_OK;
    }
    else {
        int extended_l0_log_f = atoi(argv[0]);
        cli->ke->teo_cfg.extended_l0_log_f = extended_l0_log_f != 0;
        cli_print(cli, "Teonet extend L0 DEBUG logs flag was set to: %d",
                  cli->ke->teo_cfg.extended_l0_log_f);
    }

    return CLI_OK;
}

/**
 * Set ksnet show debug messages flag
 *
 * @param cli
 * @param command
 * @param argv
 * @param argc
 * @return
 */
int cmd_ksnet_set_debug_vv_f(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if (argc < 1 || strcmp(argv[0], "?") == 0) {
        cli_print(cli,
            "Specify show debug_vv messages value: 0 - don't show; 1 - show\n"
            "(current value is: %d)", cli->ke->teo_cfg.show_debug_vv_f);
        return CLI_OK;
    }
    else {
        int show_debug_vv_f = atoi(argv[0]);
        cli->ke->teo_cfg.show_debug_vv_f = show_debug_vv_f != 0;
        cli_print(cli, "Teonet debug_vv flag was set to: %d",
                  cli->ke->teo_cfg.show_debug_vv_f);
    }

    return CLI_OK;
}

int cmd_ksnet_send_message(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if(argc < 2 || !strcmp(argv[0], "?") || !strcmp(argv[1], "?")) {
        cli_print(cli, "Specify peer name and message: <peer_name> <message>\n");
        return CLI_OK;
    }
    else {
        ksnCommandSendCmdEcho(cli->ke->kc->kco, argv[0], argv[1], strlen(argv[1]) + 1);
        cli_print(cli, "The message \"%s\" was send to peer %s", argv[1], argv[0]);
    }

    return CLI_OK;
}

int cmd_ksnet_tunnel(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    cli_print(cli,
            "Teonet tunnel command usage:\n"
            "Use ? to get help, or press TAB\n");

    return CLI_OK;
}

int cmd_ksnet_tunnel_create(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if(argc < 3 ||
       !strcmp(argv[0], "?") ||
       !strcmp(argv[1], "?") ||
       !strcmp(argv[2], "?") ||
       (argc >= 4 && !strcmp(argv[3], "?"))
               ) {

        cli_print(cli,
            "\n"
            "Teonet tunnel command command usage: \n"
            "\n"
            "Teonet tunnel create <port> <peer_name> <remote_port> [remote_ip]\n"
            "\n"
            "  port           this host local port\n"
            "  peer_name      remote port peer name\n"
            "  remote_port    remote port\n"
            "  remote_ip      remote server IP address connect to\n"
        );
    }
    else {

        cli_print(cli, "Create tunnel at port %d to peer %s at port %d and IP %s",
            atoi(argv[0]), argv[1], atoi(argv[2]), argc >= 4 ? argv[3] : localhost);

        ksnTunCreate(cli->ke->ktun, atoi(argv[0]), argv[1], atoi(argv[2]),
                     argc >= 4 ? argv[3] : NULL);
    }

    return CLI_OK;
}

int cmd_ksnet_tunnel_list(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    char *list = ksnTunListShow(cli->ke->ktun);
    cli_print(cli, "%s", list);
    free(list);

    return CLI_OK;
}

int cmd_ksnet_tunnel_remove(struct cli_def *cli, const char *command, char *argv[],
                        int argc) {

    if(argc < 1) {
        cli_print(cli, "Enter tunnel FD to remove");
    }
    else {

        // Stop server
        int fd = atoi(argv[0]);
        if(ksnTunListRemove(cli->ke->ktun, fd) != NULL) {
            close(fd);
            cli_print(cli, "The tunnel with FD %d was removed", fd);
        }
        else cli_print(cli, "Invalid server FD %d was entered", fd);
    }

    return CLI_OK;
}

/******************************************************************************/
/* Test commands                                                              */
/******************************************************************************/

//int cmd_test(struct cli_def *cli, const char *command, char *argv[], int argc)
//{
//    int i;
//    cli_print(cli, "called %s with \"%s\"", __FUNCTION__, command);
//    cli_print(cli, "%d arguments:", argc);
//    for (i = 0; i < argc; i++)
//        cli_print(cli, "        %s", argv[i]);
//
//    return CLI_OK;
//}
//
//int cmd_set(struct cli_def *cli, UNUSED(const char *command), char *argv[],
//    int argc)
//{
//    if (argc < 2 || strcmp(argv[0], "?") == 0)
//    {
//        cli_print(cli, "Specify a variable to set");
//        return CLI_OK;
//    }
//
//    if (strcmp(argv[1], "?") == 0)
//    {
//        cli_print(cli, "Specify a value");
//        return CLI_OK;
//    }
//
//    if (strcmp(argv[0], "regular_interval") == 0)
//    {
//        unsigned int sec = 0;
//        if (!argv[1] && !&argv[1])
//        {
//            cli_print(cli, "Specify a regular callback interval in seconds");
//            return CLI_OK;
//        }
//        sscanf(argv[1], "%u", &sec);
//        if (sec < 1)
//        {
//            cli_print(cli, "Specify a regular callback interval in seconds");
//            return CLI_OK;
//        }
//        cli->timeout_tm.tv_sec = sec;
//        cli->timeout_tm.tv_usec = 0;
//        cli_print(cli, "Regular callback interval is now %d seconds", sec);
//        return CLI_OK;
//    }
//
//    cli_print(cli, "Setting \"%s\" to \"%s\"", argv[0], argv[1]);
//    return CLI_OK;
//}
//
//int cmd_show_regular(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
//{
//    cli_print(cli, "cli_regular() has run %u times", cli->regular_count);
//    return CLI_OK;
//}
//
//int cmd_config_int(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
//{
//    if (argc < 1)
//    {
//        cli_print(cli, "Specify an interface to configure");
//        return CLI_OK;
//    }
//
//    if (strcmp(argv[0], "?") == 0)
//        cli_print(cli, "  test0/0");
//
//    else if (strcasecmp(argv[0], "test0/0") == 0)
//        cli_set_configmode(cli, MODE_CONFIG_INT, "test");
//    else
//        cli_print(cli, "Unknown interface %s", argv[0]);
//
//    return CLI_OK;
//}
//
//int cmd_config_int_exit(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
//{
//    cli_set_configmode(cli, MODE_CONFIG, NULL);
//    return CLI_OK;
//}
//
//int cmd_debug_regular(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
//{
//    cli->debug_regular = !cli->debug_regular;
//    cli_print(cli, "cli_regular() debugging is %s", cli->debug_regular ? "enabled" : "disabled");
//    return CLI_OK;
//}
//
//int cmd_context(struct cli_def *cli, UNUSED(const char *command),
//                UNUSED(char *argv[]), UNUSED(int argc)) {
//
//    struct my_context *myctx = (struct my_context *) cli_get_context(cli);
//    cli_print(cli, "User context has a value of %d and message saying \"%s\"",
//              myctx->value, myctx->message);
//
//    return CLI_OK;
//}

/******************************************************************************/
/* Authentication commands                                                    */
/******************************************************************************/

/**
 * Check authentication callback
 *
 * @param username
 * @param password
 * @return
 */
int check_auth(const char *username, const char *password) {

    if (strcasecmp(username, "fred") != 0)
        return CLI_ERROR;
    if (strcasecmp(password, "nerk") != 0)
        return CLI_ERROR;

    return CLI_OK;
}

int check_enable(const char *password)
{
    return !strcasecmp(password, "topsecret");
}

void pc(UNUSED(struct cli_def *cli), const char *string)
{
    printf("%s\n", string);
}

/******************************************************************************/
/* Interface definition                                                       */
/******************************************************************************/

/**
 * Initial CLI Terminal interface
 */
struct cli_def *ksnTermCliInit(ksnTermClass *kter) {

    struct cli_command *c, *cc;
    struct cli_def *cli;

    cli = cli_init(kter->ke);

    cli_set_banner(cli, "Teonet terminal server environment, ver. " VERSION
                        "\n(press Ctrl+D to quit)");
    cli_set_hostname(cli, kev->teo_cfg.host_name);
    cli_telnet_protocol(cli, 1);
    cli_regular(cli, regular_callback);
    cli_regular_interval(cli, 5); // Defaults to 1 second
    cli_set_idle_timeout_callback(cli, 120, idle_timeout); // 120 second idle timeout

    /**************************************************************************/
    /* KSNet commands                                                         */
    /**************************************************************************/

    cli_register_command(cli, NULL, "peers", cmd_peer, PRIVILEGE_UNPRIVILEGED,
        MODE_EXEC, "Show list of teonet peers");

    c = cli_register_command(cli, NULL, "teonet", cmd_ksnet, PRIVILEGE_UNPRIVILEGED,
        MODE_EXEC, "Monitoring and manage Teonet functions");

        cli_register_command(cli, c, "show", cmd_ksnet_show, PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "Show teonet settings");

        cc = cli_register_command(cli, c, "set", cmd_ksnet_set, PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "Set teonet parameters");

            cli_register_command(cli, cc, "peers", cmd_ksnet_set_peers_f, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Set show peers flag");

            cli_register_command(cli, cc, "debug", cmd_ksnet_set_debug_f, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Set show debug messages flag");

            cli_register_command(cli, cc, "extend_l0_log", cmd_ksnet_extend_l0_log_f, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Extend L0 DEBUG logs flag (on/off)");    

            cli_register_command(cli, cc, "debug_vv", cmd_ksnet_set_debug_vv_f, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Set show debug_vv messages flag");

            cli_register_command(cli, cc, "tr_udp", cmd_ksnet_set_tr_udp_f, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Set show TR-UDP flag");

        cli_register_command(cli, c, "send_message", cmd_ksnet_send_message, PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "Send message to peer");

        cc = cli_register_command(cli, c, "tunnel", cmd_ksnet_tunnel, PRIVILEGE_UNPRIVILEGED,
            MODE_EXEC, "Manage teonet tunnels");

            cli_register_command(cli, cc, "create", cmd_ksnet_tunnel_create, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Create teonet tunnel");

            cli_register_command(cli, cc, "list", cmd_ksnet_tunnel_list, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Show list of tunnels");

            cli_register_command(cli, cc, "remove", cmd_ksnet_tunnel_remove, PRIVILEGE_UNPRIVILEGED,
                MODE_EXEC, "Remove tunnel from list");

    /**************************************************************************/
    /* Test Commands                                                          */
    /**************************************************************************/

//    cli_register_command(cli, NULL, "test", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//    cli_register_command(cli, NULL, "simple", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//    cli_register_command(cli, NULL, "simon", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//    cli_register_command(cli, NULL, "set", cmd_set, PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
//
//    c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//        cli_register_command(cli, c, "regular", cmd_show_regular, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
//                             "Show the how many times cli_regular has run");
//
//        cli_register_command(cli, c, "counters", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
//                             "Show the counters that the system uses");
//
//        cli_register_command(cli, c, "junk", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//    cli_register_command(cli, NULL, "interface", cmd_config_int, PRIVILEGE_PRIVILEGED, MODE_CONFIG,
//                         "Configure an interface");
//
//    cli_register_command(cli, NULL, "exit", cmd_config_int_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT,
//                         "Exit from interface configuration");
//
//    cli_register_command(cli, NULL, "address", cmd_test, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT, "Set IP address");
//
//    c = cli_register_command(cli, NULL, "debug", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
//
//        cli_register_command(cli, c, "regular", cmd_debug_regular, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
//                             "Enable cli_regular() callback debugging");
//
//    /* Set user context and its command ***************************************/
//    cli_set_context(cli, (void*) &kter->myctx);
//    cli_register_command(cli, NULL, "context", cmd_context, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
//                         "Test a user-specified context");

    /**************************************************************************/
    /* Authentication                                                         */
    /**************************************************************************/

    cli_set_auth_callback(cli, check_auth);
    cli_set_enable_callback(cli, check_enable);

    if(kev->event_cb != NULL) kev->event_cb(kev, EV_K_TERM_STARTED, NULL, 0, NULL);
    
    /* Test reading from a file ***********************************************/
    {
        FILE *fh;

        if ((fh = fopen("teonet.txt", "r")))
        {
            // This sets a callback which just displays the cli_print() text to stdout
            cli_print_callback(cli, pc);
            cli_file(cli, fh, PRIVILEGE_UNPRIVILEGED, MODE_EXEC);
            cli_print_callback(cli, NULL);
            fclose(fh);
        }
    }

    return cli;
}

#endif

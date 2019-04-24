/**
 * KSnet Network CLI module
 *
 * Modified by Kirill Scherba to use in KSNet on May 07, 2015, 23:43
 * from the libcli library code http://sites.dparrish.com/libcli code
 * Developer's Reference: http://sites.dparrish.com/libcli/developers-reference
 */

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#if !defined(__APPLE__) && !defined(__FreeBSD__)
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#ifndef WIN32
#include <regex.h>
#include <sys/socket.h>
#endif


//#include "ev_mgr.h"
#include "net_cli.h"
#include "utils/rlutil.h"

// vim:sw=4 tw=120 et

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

#define MATCH_REGEX     1
#define MATCH_INVERT    2

#ifdef WIN32
/*
 * Stupid windows has multiple namespaces for filedescriptors, with different
 * read/write functions required for each ..
 */
int read(int fd, void *buf, unsigned int count) {
    return recv(fd, buf, count, 0);
}

int write(int fd, const void *buf, unsigned int count) {
    return send(fd, buf, count, 0);
}

int vasprintf(char **strp, const char *fmt, va_list args) {
    int size;

    size = vsnprintf(NULL, 0, fmt, args);
    if ((*strp = malloc(size + 1)) == NULL) {
        return -1;
    }

    size = vsnprintf(*strp, size + 1, fmt, args);
    return size;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    int size;

    va_start(args, fmt);
    size = vasprintf(strp, fmt, args);

    va_end(args);
    return size;
}

int fprintf(FILE *stream, const char *fmt, ...) {
    va_list args;
    int size;
    char *buf;

    va_start(args, fmt);
    size = vasprintf(&buf, fmt, args);
    if (size < 0) {
        goto out;
    }
    size = write(stream->_file, buf, size);
    free(buf);

out:
    va_end(args);
    return size;
}

/*
 * Dummy definitions to allow compilation on Windows
 */
int regex_dummy() {return 0;};
#define regfree(...) regex_dummy()
#define regexec(...) regex_dummy()
#define regcomp(...) regex_dummy()
#define regex_t int
#define REG_NOSUB       0
#define REG_EXTENDED    0
#define REG_ICASE       0
#endif

enum cli_states {
    STATE_LOGIN,
    STATE_PASSWORD,
    STATE_NORMAL,
    STATE_ENABLE_PASSWORD,
    STATE_ENABLE
};

struct unp {
    char *username;
    char *password;
    struct unp *next;
};

struct cli_filter_cmds
{
    const char *cmd;
    const char *help;
};

/* free and zero (to avoid double-free) */
#define free_z(p) do { if (p) { free(p); (p) = 0; } } while (0)

int cli_match_filter_init(struct cli_def *cli, int argc, char **argv, struct cli_filter *filt);
int cli_range_filter_init(struct cli_def *cli, int argc, char **argv, struct cli_filter *filt);
int cli_count_filter_init(struct cli_def *cli, int argc, char **argv, struct cli_filter *filt);
int cli_match_filter(struct cli_def *cli, const char *string, void *data);
int cli_range_filter(struct cli_def *cli, const char *string, void *data);
int cli_count_filter(struct cli_def *cli, const char *string, void *data);

static struct cli_filter_cmds filter_cmds[] =
{
    { "begin",   "Begin with lines that match" },
    { "between", "Between lines that match" },
    { "count",   "Count of lines"   },
    { "exclude", "Exclude lines that match" },
    { "include", "Include lines that match" },
    { "grep",    "Include lines that match regex (options: -v, -i, -e)" },
    { "egrep",   "Include lines that match extended regex" },
    { NULL, NULL}
};

static ssize_t __write(int fd, const void *buf, size_t count)
{
    size_t written = 0;
    ssize_t thisTime =0;
    while (count != written)
    {
        thisTime = write(fd, (char*)buf + written, count - written);
        if (thisTime == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        written += thisTime;
    }
    return written;
}

char *cli_command_name(struct cli_def *cli, struct cli_command *command)
{
    char *name = cli->commandname;
    char *o;

    if (name) free(name);
    if (!(name = calloc(1, 1)))
        return NULL;

    while (command)
    {
        o = name;
        if (asprintf(&name, "%s%s%s", command->command, *o ? " " : "", o) == -1)
        {
            fprintf(stderr, "Couldn't allocate memory for command_name: %s", strerror(errno));
            free(o);
            return NULL;
        }
        command = command->parent;
        free(o);
    }
    cli->commandname = name;
    return name;
}

/**
 * Enables or disables callback based authentication.
 *
 * If auth_callback is not NULL, then authentication will be required on
 * connection. auth_callback will be called with the username and password that
 * the user enters.
 *
 * @param cli
 * @param auth_callback must return a non-zero value if authentication is
 *        successful. If auth_callback is NULL, then callback based
 *        authentication will be disabled.
 */
void cli_set_auth_callback(struct cli_def *cli, int (*auth_callback)(const char *, const char *))
{
    cli->auth_callback = auth_callback;
}

/**
 * Just like cli_set_auth_callback, this takes a pointer to a callback function
 * to authorize privileged access. However this callback only takes a single
 * string - the password.
 *
 * @param cli
 * @param enable_callback
 */
void cli_set_enable_callback(struct cli_def *cli, int (*enable_callback)(const char *))
{
    cli->enable_callback = enable_callback;
}

/**
 * Enables internal authentication, and adds username/password to the list of
 * allowed users.
 *
 * The internal list of users will be checked before callback based
 * authentication is tried.
 *
 * @param cli
 * @param username
 * @param password
 */
void cli_allow_user(struct cli_def *cli, const char *username, const char *password)
{
    struct unp *u, *n;
    if (!(n = malloc(sizeof(struct unp))))
    {
        fprintf(stderr, "Couldn't allocate memory for user: %s", strerror(errno));
        return;
    }
    if (!(n->username = strdup(username)))
    {
        fprintf(stderr, "Couldn't allocate memory for username: %s", strerror(errno));
        free(n);
        return;
    }
    if (!(n->password = strdup(password)))
    {
        fprintf(stderr, "Couldn't allocate memory for password: %s", strerror(errno));
        free(n->username);
        free(n);
        return;
    }
    n->next = NULL;

    if (!cli->users)
    {
        cli->users = n;
    }
    else
    {
        for (u = cli->users; u && u->next; u = u->next);
        if (u) u->next = n;
    }
}

/**
 * This will allow a static password to be used for the enable command. This
 * static password will be checked before running any enable callbacks.
 *
 * Set this to NULL to not have a static enable password.
 *
 * @param cli
 * @param password
 */
void cli_allow_enable(struct cli_def *cli, const char *password)
{
    free_z(cli->enable_password);
    if (!(cli->enable_password = strdup(password)))
    {
        fprintf(stderr, "Couldn't allocate memory for enable password: %s", strerror(errno));
    }
}

/**
 * Removes username/password from the list of allowed users.
 * If this is the last combination in the list, then internal authentication
 * will be disabled.
 *
 * @param cli
 * @param username
 */
void cli_deny_user(struct cli_def *cli, const char *username)
{
    struct unp *u, *p = NULL;
    if (!cli->users) return;
    for (u = cli->users; u; u = u->next)
    {
        if (strcmp(username, u->username) == 0)
        {
            if (p)
                p->next = u->next;
            else
                cli->users = u->next;
            free(u->username);
            free(u->password);
            free(u);
            break;
        }
        p = u;
    }
}

/**
 * Sets the greeting that clients will be presented with when they connect.
 * This may be a security warning for example.
 *
 * If this function is not called or called with a NULL argument, no banner
 * will be presented.
 *
 * @param cli
 * @param banner
 */
void cli_set_banner(struct cli_def *cli, const char *banner)
{
    free_z(cli->banner);
    if (banner && *banner)
        cli->banner = strdup(banner);
}

/**
 * Sets the hostname to be displayed as the first part of the prompt.
 *
 * @param cli
 * @param hostname
 */
void cli_set_hostname(struct cli_def *cli, const char *hostname)
{
    free_z(cli->hostname);
    if (hostname && *hostname)
        cli->hostname = strdup(hostname);
}

void cli_set_promptchar(struct cli_def *cli, const char *promptchar)
{
    free_z(cli->promptchar);
    cli->promptchar = strdup(promptchar);
}

static int cli_build_shortest(struct cli_def *cli, struct cli_command *commands)
{
    struct cli_command *c, *p;
    char *cp, *pp;
    unsigned len;

    for (c = commands; c; c = c->next)
    {
        c->unique_len = strlen(c->command);
        if ((c->mode != MODE_ANY && c->mode != cli->mode) || c->privilege > cli->privilege)
            continue;

        c->unique_len = 1;
        for (p = commands; p; p = p->next)
        {
            if (c == p)
                continue;

            if ((p->mode != MODE_ANY && p->mode != cli->mode) || p->privilege > cli->privilege)
                continue;

            cp = c->command;
            pp = p->command;
            len = 1;

            while (*cp && *pp && *cp++ == *pp++)
                len++;

            if (len > c->unique_len)
                c->unique_len = len;
        }

        if (c->children)
            cli_build_shortest(cli, c->children);
    }

    return CLI_OK;
}

int cli_set_privilege(struct cli_def *cli, int priv)
{
    int old = cli->privilege;
    cli->privilege = priv;

    if (priv != old)
    {
        cli_set_promptchar(cli, priv == PRIVILEGE_PRIVILEGED ? "] # " : "] > ");
        cli_build_shortest(cli, cli->commands);
    }

    return old;
}

void cli_set_modestring(struct cli_def *cli, const char *modestring)
{
    free_z(cli->modestring);
    if (modestring)
        cli->modestring = strdup(modestring);
}

/**
 * This will set the configuration mode. Once set, commands will be restricted
 * to only ones in the selected configuration mode, plus any set to MODE_ANY.
 * The previous mode value is returned.
 *
 * The string passed will be used to build the prompt in the set configuration
 * mode. e.g. if you set the string *test*, the prompt will become:
 * hostname(config-test)#
 *
 * @param cli
 * @param mode
 * @param config_desc
 * @return
 */
int cli_set_configmode(struct cli_def *cli, int mode, const char *config_desc)
{
    int old = cli->mode;
    cli->mode = mode;

    if (mode != old)
    {
        if (!cli->mode)
        {
            // Not config mode
            cli_set_modestring(cli, NULL);
        }
        else if (config_desc && *config_desc)
        {
            char string[64];
            snprintf(string, sizeof(string), "%s /config/%s%s", ANSI_CYAN, config_desc, ANSI_NONE);
            cli_set_modestring(cli, string);
        }
        else
        {
            char string[256];
            snprintf(string, sizeof(string), "%s /config%s", ANSI_CYAN, ANSI_NONE);
            cli_set_modestring(cli, string);
        }

        cli_build_shortest(cli, cli->commands);
    }

    return old;
}

/**
 * Add a command to the internal command tree.
 *
 * @param cli The handle of the cli structure
 *
 * @param parent If parent is NULL, the command is added to the top level of
 * commands, otherwise it is a subcommand of parent.
 *
 * @param command Command name
 *
 * @param callback  When the command has been entered by the user, callback is checked.
 * If it is not NULL, then the callback is called with:
 *
 * struct cli_def * - the handle of the cli structure. This must be passed to
 * all cli functions, including cli_print()\n
 * char * - the entire command which was entered. This is after command expansion \n
 * char ** - the list of arguments entered \n
 * int - the number of arguments entered\n
 *
 * The callback must return CLI_OK if the command was successful, CLI_ERROR if
 * processing wasn't successful and the next matching command should be tried
 * (if any), or CLI_QUIT to drop the connection (e.g. on a fatal error).
 *
 * @param privilege privilege should be set to either PRIVILEGE_PRIVILEGED or
 * PRIVILEGE_UNPRIVILEGED. If set to PRIVILEGE_PRIVILEGED then the user must
 * have entered enable before running this command.
 *
 * @param mode Mode should be set to MODE_EXEC for no configuration mode, MODE_CONFIG for
 * generic configuration commands, or your own config level. The user can enter
 * the generic configuration level by entering configure terminal, and can
 * return to MODE_EXEC by entering exit or CTRL-Z. You can define commands to
 * enter your own configuration levels, which should call the
 * cli_set_configmode() function.
 *
 * @param help If help is provided, it is given to the user when the use the
 * help command or press ?.
 *
 * @return  Returns a struct cli_command *, which you can pass as parent to
 *          another call to cli_register_command().
 */
struct cli_command *cli_register_command(
        struct cli_def *cli, struct cli_command *parent, const char *command,
        int (*callback)(struct cli_def *cli, const char *, char **, int),
        int privilege, int mode, const char *help)
{
    struct cli_command *c, *p;

    if (!command) return NULL;
    if (!(c = calloc(sizeof(struct cli_command), 1))) return NULL;

    c->system = 0;
    c->callback = callback;
    c->next = NULL;
    if (!(c->command = strdup(command))) {
        free(c);
        return NULL;
    }
    c->parent = parent;
    c->privilege = privilege;
    c->mode = mode;
    if (help && !(c->help = strdup(help))) {
        free(c->command);
        free(c);
        return NULL;
    }

    if (parent)
    {
        if (!parent->children)
        {
            parent->children = c;
        }
        else
        {
            for (p = parent->children; p && p->next; p = p->next);
            if (p) p->next = c;
        }
    }
    else
    {
        if (!cli->commands)
        {
            cli->commands = c;
        }
        else
        {
            for (p = cli->commands; p && p->next; p = p->next);
            if (p) p->next = c;
        }
    }
    return c;
}

static void cli_free_command(struct cli_command *cmd)
{
    struct cli_command *c, *p;

    for (c = cmd->children; c;)
    {
        p = c->next;
        cli_free_command(c);
        c = p;
    }

    free(cmd->command);
    if (cmd->help) free(cmd->help);
    free(cmd);
}

/**
 * Remove a command command and all children. There is not provision yet for
 * removing commands at lower than the top level.
 *
 * @param cli
 * @param command
 * @return
 */
int cli_unregister_command(struct cli_def *cli, const char *command)
{
    struct cli_command *c, *p = NULL;

    if (!command) return -1;
    if (!cli->commands) return CLI_OK;

    for (c = cli->commands; c; c = c->next)
    {
        if (strcmp(c->command, command) == 0)
        {
            if (p)
                p->next = c->next;
            else
                cli->commands = c->next;

            cli_free_command(c);
            return CLI_OK;
        }
        p = c;
    }

    return CLI_OK;
}

int cli_show_help(struct cli_def *cli, struct cli_command *c) {

    struct cli_command *p;

    for(p = c; p; p = p->next) {

        if(p->command && p->callback && cli->privilege >= p->privilege &&
            (p->mode == cli->mode || p->mode == MODE_ANY)) {

            cli_error(cli, "  %s%-20s%s %s",
                      (p->system ? ANSI_YELLOW : ANSI_CYAN),
                      cli_command_name(cli, p),
                      ANSI_NONE,
                      (p->help != NULL ? p->help : ""));
        }

        if(p->children) cli_show_help(cli, p->children);
    }

    return CLI_OK;
}

int cli_int_enable(struct cli_def *cli, UNUSED(const char *command),
                   UNUSED(char *argv[]), UNUSED(int argc)) {

    if (cli->privilege == PRIVILEGE_PRIVILEGED)
        return CLI_OK;

    if (!cli->enable_password && !cli->enable_callback)
    {
        // No password required, set privilege immediately
        cli_set_privilege(cli, PRIVILEGE_PRIVILEGED);
        cli_set_configmode(cli, MODE_EXEC, NULL);
    }
    else
    {
        // Require password entry
        cli->state = STATE_ENABLE_PASSWORD;
    }

    return CLI_OK;
}

int cli_int_disable(struct cli_def *cli, UNUSED(const char *command),
                    UNUSED(char *argv[]), UNUSED(int argc)) {

    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);

    return CLI_OK;
}

int cli_int_help(struct cli_def *cli, UNUSED(const char *command),
        UNUSED(char *argv[]), UNUSED(int argc)) {

    // Telnet hot keys
    cli_error(cli, "\n%sKey bindings:%s\n", ANSI_NONE/*ANSI_MAGENTA*/, ANSI_NONE);

    typedef char *key_def[4];
    key_def keys[] = {
        { ANSI_GREEN, "TAB", ANSI_NONE,               "Command completion" },
        { ANSI_GREEN, "Up Arrow/CTRL+P", ANSI_NONE,   "Show previous command from history" },
        { ANSI_GREEN, "Down Arrow/CTRL+N", ANSI_NONE, "Show previous command from history" },
        { ANSI_GREEN, "CTRL+A", ANSI_NONE,            "To start of line" },
        { ANSI_GREEN, "CTRL+E", ANSI_NONE,            "To end of line" },
        { ANSI_GREEN, "CTRL+L", ANSI_NONE,            "Redraw" },
        { ANSI_GREEN, "CTRL+K", ANSI_NONE,            "Remove the text after the cursor and save it in kill area" },
        { ANSI_GREEN, "CTRL+U", ANSI_NONE,            "Clear line" },
        { ANSI_GREEN, "CTRL+D", ANSI_NONE,            "Call logout" }
    };
    int i, len = sizeof(keys)/sizeof(key_def);
    for(i = 0; i < len; i++)
        cli_error(cli, "  %s%-20s%s %s", keys[i][0], keys[i][1], keys[i][2], keys[i][3]);

    // Server commands
    cli_error(cli, "\n%sCommands available:%s\n", ANSI_NONE/*ANSI_MAGENTA*/, ANSI_NONE);
    cli_show_help(cli, cli->commands);
    cli_error(cli, " ");

    return CLI_OK;
}

/**
 * Show history command
 *
 * @param cli
 * param command
 * param argv
 * param argc
 * @return
 */
int cli_int_history(struct cli_def *cli, UNUSED(const char *command),
                    UNUSED(char *argv[]), UNUSED(int argc)) {

    int i;

    cli_error(cli, "\n%sCommand history:%s\n", ANSI_NONE/*ANSI_MAGENTA*/, ANSI_NONE);
    for (i = 0; i < MAX_HISTORY; i++) {

        if (cli->history[i])
            cli_error(cli, "%3d. %s", i, cli->history[i]);
    }
    cli_error(cli, " ");

    return CLI_OK;
}

int cli_int_quit(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);
    return CLI_QUIT;
}

int cli_int_exit(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    if (cli->mode == MODE_EXEC)
        return cli_int_quit(cli, command, argv, argc);

    if (cli->mode > MODE_CONFIG)
        cli_set_configmode(cli, MODE_CONFIG, NULL);
    else
        cli_set_configmode(cli, MODE_EXEC, NULL);

    cli->service = NULL;
    return CLI_OK;
}

int cli_int_idle_timeout(struct cli_def *cli)
{
    cli_print(cli, "Idle timeout");
    return CLI_QUIT;
}

int cli_int_configure_terminal(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_configmode(cli, MODE_CONFIG, NULL);
    return CLI_OK;
}

/**
 * CLI Initialize
 *
 * This must be called before any other cli_xxx function. It sets up the
 * internal data structures used for command-line processing.
 *
 * @param ke KSNet event manager class pointer
 * @return Returns a struct cli_def * which must be passed to all other cli_xxx
 *         functions.
 */
struct cli_def *cli_init(ksnetEvMgrClass *ke)
{
    struct cli_def *cli;
    struct cli_command *c;

    if (!(cli = calloc(sizeof(struct cli_def), 1)))
        return 0;

    ke->cli = cli;
    cli->ke = ke;
    cli->regular_count = 0;
    cli->debug_regular = 0;
    cli->auth_callback = NULL;
    cli->enable_callback = NULL;
    cli->regular_callback = NULL;

    cli->buf_size = KSN_BUFFER_SIZE; // 1024;
    if (!(cli->buffer = calloc(cli->buf_size, 1)))
    {
        free_z(cli);
        return 0;
    }
    cli->telnet_protocol = 1;

    cli_register_command(cli, 0, "help", cli_int_help, PRIVILEGE_UNPRIVILEGED, MODE_ANY, "Show available commands")->system = 1;
    cli_register_command(cli, 0, "quit", cli_int_quit, PRIVILEGE_UNPRIVILEGED, MODE_ANY, "Disconnect")->system = 1;
    cli_register_command(cli, 0, "logout", cli_int_quit, PRIVILEGE_UNPRIVILEGED, MODE_ANY, "Disconnect")->system = 1;
    cli_register_command(cli, 0, "exit", cli_int_exit, PRIVILEGE_UNPRIVILEGED, MODE_ANY, "Exit from current mode")->system = 1;
    cli_register_command(cli, 0, "history", cli_int_history, PRIVILEGE_UNPRIVILEGED, MODE_ANY,
                         "Show a list of previously run commands")->system = 1;

    cli_register_command(cli, 0, "enable", cli_int_enable, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "Turn on privileged commands")->system = 1;
    cli_register_command(cli, 0, "disable", cli_int_disable, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                         "Turn off privileged commands")->system = 1;

    c = cli_register_command(cli, 0, "configure", 0, PRIVILEGE_PRIVILEGED, MODE_EXEC, "Enter configuration mode");
    c->system = 1;
    cli_register_command(cli, c, "terminal", cli_int_configure_terminal, PRIVILEGE_PRIVILEGED, MODE_EXEC,
                         "Configure from the terminal")->system = 1;

    cli->privilege = cli->mode = -1;
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, 0);

    // Default to 1 second timeout intervals
    cli->timeout_tm.tv_sec = 1;
    cli->timeout_tm.tv_usec = 0;

    // Set default idle timeout callback, but no timeout
    cli_set_idle_timeout_callback(cli, 0, cli_int_idle_timeout);

    return cli;
}

void cli_unregister_all(struct cli_def *cli, struct cli_command *command)
{
    struct cli_command *c, *p = NULL;

    if (!command) command = cli->commands;
    if (!command) return;

    for (c = command; c; )
    {
        p = c->next;

        // Unregister all child commands
        if (c->children)
            cli_unregister_all(cli, c->children);

        if (c->command) free(c->command);
        if (c->help) free(c->help);
        free(c);

        c = p;
    }
}

/**
 * This is optional, but it's a good idea to call this when you are finished
 * with libcli. This frees memory used by libcli.
 *
 * @param cli
 * @return
 */
int cli_done(struct cli_def *cli)
{
    struct unp *u = cli->users, *n;

    if (!cli) return CLI_OK;
    cli_free_history(cli);

    // Free all users
    while (u)
    {
        if (u->username) free(u->username);
        if (u->password) free(u->password);
        n = u->next;
        free(u);
        u = n;
    }

    /* free all commands */
    cli_unregister_all(cli, 0);

    free_z(cli->commandname);
    free_z(cli->modestring);
    free_z(cli->banner);
    free_z(cli->promptchar);
    free_z(cli->hostname);
    free_z(cli->buffer);
    free_z(cli);

    return CLI_OK;
}

/**
 * Add command to history
 * @param cli
 * @param cmd
 * @return Index in_history
 */
static int cli_add_history(struct cli_def *cli, const char *cmd) {

    int i;

    // Trim command
    trim((char*)cmd);

    // Find last empty string in history array
    for (i = 0; i < MAX_HISTORY; i++) {

        // Check the same command exist in history array
        if (cli->history[i]) {
            if(!strcasecmp(cli->history[i], cmd)) return i;
        }

        // Add command to the first empty history array position
        else {
            if (!(cli->history[i] = strdup(cmd))) return CLI_ERROR;
            return i;
        }
    }

    // No space found, drop one off the beginning of the list
    free(cli->history[0]);
    for (i = 0; i < MAX_HISTORY-1; i++) {
        cli->history[i] = cli->history[i+1];
    }
    if (!(cli->history[MAX_HISTORY - 1] = strdup(cmd))) return CLI_ERROR;
    return MAX_HISTORY - 1;
}

/**
 * Clear history array
 *
 * @param cli
 */
void cli_free_history(struct cli_def *cli) {

    int i;
    for (i = 0; i < MAX_HISTORY; i++) {
        if (cli->history[i]) free_z(cli->history[i]);
    }
}

static int cli_parse_line(const char *line, char *words[], int max_words) {

    int nwords = 0;
    const char *p = line;
    const char *word_start = 0;
    int inquote = 0;

    while (*p)
    {
        if (!isspace(*p))
        {
            word_start = p;
            break;
        }
        p++;
    }

    while (nwords < max_words - 1)
    {
        if (!*p || *p == inquote || (word_start && !inquote && (isspace(*p) || *p == '|')))
        {
            if (word_start)
            {
                int len = p - word_start;

                memcpy(words[nwords] = malloc(len + 1), word_start, len);
                words[nwords++][len] = 0;
            }

            if (!*p)
                break;

            if (inquote)
                p++; /* skip over trailing quote */

            inquote = 0;
            word_start = 0;
        }
        else if (*p == '"' || *p == '\'')
        {
            inquote = *p++;
            word_start = p;
        }
        else
        {
            if (!word_start)
            {
                if (*p == '|')
                {
                    if (!(words[nwords++] = strdup("|")))
                        return 0;
                }
                else if (!isspace(*p))
                    word_start = p;
            }

            p++;
        }
    }

    return nwords;
}

static char *join_words(int argc, char **argv)
{
    char *p;
    int len = 0;
    int i;

    for (i = 0; i < argc; i++)
    {
        if (i)
            len += 1;

        len += strlen(argv[i]);
    }

    p = malloc(len + 1);
    p[0] = 0;

    for (i = 0; i < argc; i++)
    {
        if (i)
            strcat(p, " ");

        strcat(p, argv[i]);
    }

    return p;
}

static int cli_find_command(struct cli_def *cli, struct cli_command *commands,
                            int num_words, char *words[],
                            int start_word, int filters[]) {

    struct cli_command *c, *again_config = NULL, *again_any = NULL;
    int c_words = num_words;

    if (filters[0]) c_words = filters[0];

    // Deal with ? for help
    if (!words[start_word]) return CLI_ERROR;

    if (words[start_word][strlen(words[start_word]) - 1] == '?') {

        int l = strlen(words[start_word])-1;

        if (commands->parent && commands->parent->callback) {
            cli_error(cli, "%-20s %s", cli_command_name(cli, commands->parent),
                (commands->parent->help != NULL ? commands->parent->help : ""));
        }

        cli_error(cli, " ");
        for (c = commands; c; c = c->next) {

            if (strncasecmp(c->command, words[start_word], l) == 0
                && (c->callback || c->children)
                && cli->privilege >= c->privilege
                && (c->mode == cli->mode || c->mode == MODE_ANY)) {

                    cli_error(cli,
                              "  %s%-20s%s %s",
                              (c->system ? ANSI_YELLOW : ANSI_CYAN),
                              c->command,
                              ANSI_NONE,
                              (c->help != NULL ? c->help : ""));
            }
        }
        cli_error(cli, " ");

        return CLI_OK;
    }

    for (c = commands; c; c = c->next)
    {
        if (cli->privilege < c->privilege)
            continue;

        if (strncasecmp(c->command, words[start_word], c->unique_len))
            continue;

        if (strncasecmp(c->command, words[start_word], strlen(words[start_word])))
            continue;

        AGAIN:
        if (c->mode == cli->mode || (c->mode == MODE_ANY && again_any != NULL))
        {
            int rc = CLI_OK;
            int f;
            struct cli_filter **filt = &cli->filters;

            // Found a word!
            if (!c->children)
            {
                // Last word
                if (!c->callback)
                {
                    cli_error(cli, "No callback for \"%s\"", cli_command_name(cli, c));
                    return CLI_ERROR;
                }
            }
            else
            {
                if (start_word == c_words - 1)
                {
                    if (c->callback)
                        goto CORRECT_CHECKS;

                    cli_error(cli, "Incomplete command");
                    return CLI_ERROR;
                }
                rc = cli_find_command(cli, c->children, num_words, words, start_word + 1, filters);
                if (rc == CLI_ERROR_ARG)
                {
                    if (c->callback)
                    {
                        rc = CLI_OK;
                        goto CORRECT_CHECKS;
                    }
                    else
                    {
                        cli_error(cli, "Invalid %s \"%s\"", commands->parent ? "argument" : "command",
                                  words[start_word]);
                    }
                }
                return rc;
            }

            if (!c->callback)
            {
                cli_error(cli, "Internal server error processing \"%s\"", cli_command_name(cli, c));
                return CLI_ERROR;
            }

            CORRECT_CHECKS:
            for (f = 0; rc == CLI_OK && filters[f]; f++)
            {
                int n = num_words;
                char **argv;
                int argc;
                int len;

                if (filters[f+1])
                n = filters[f+1];

                if (filters[f] == n - 1)
                {
                    cli_error(cli, "Missing filter");
                    return CLI_ERROR;
                }

                argv = words + filters[f] + 1;
                argc = n - (filters[f] + 1);
                len = strlen(argv[0]);
                if (argv[argc - 1][strlen(argv[argc - 1]) - 1] == '?')
                {
                    if (argc == 1)
                    {
                        int i;
                        for (i = 0; filter_cmds[i].cmd; i++)
                            cli_error(cli, "  %-20s %s", filter_cmds[i].cmd, filter_cmds[i].help );
                    }
                    else
                    {
                        if (argv[0][0] != 'c') // count
                            cli_error(cli, "  WORD");

                        if (argc > 2 || argv[0][0] == 'c') // count
                            cli_error(cli, "  <cr>");
                    }

                    return CLI_OK;
                }

                if (argv[0][0] == 'b' && len < 3) // [beg]in, [bet]ween
                {
                    cli_error(cli, "Ambiguous filter \"%s\" (begin, between)", argv[0]);
                    return CLI_ERROR;
                }
                *filt = calloc(sizeof(struct cli_filter), 1);

                if (!strncmp("include", argv[0], len) || !strncmp("exclude", argv[0], len) ||
                    !strncmp("grep", argv[0], len) || !strncmp("egrep", argv[0], len))
                    rc = cli_match_filter_init(cli, argc, argv, *filt);
                else if (!strncmp("begin", argv[0], len) || !strncmp("between", argv[0], len))
                    rc = cli_range_filter_init(cli, argc, argv, *filt);
                else if (!strncmp("count", argv[0], len))
                    rc = cli_count_filter_init(cli, argc, argv, *filt);
                else
                {
                    cli_error(cli, "Invalid filter \"%s\"", argv[0]);
                    rc = CLI_ERROR;
                }

                if (rc == CLI_OK)
                {
                    filt = &(*filt)->next;
                }
                else
                {
                    free(*filt);
                    *filt = 0;
                }
            }

            if (rc == CLI_OK)
                rc = c->callback(cli, cli_command_name(cli, c), words + start_word + 1, c_words - start_word - 1);

            while (cli->filters)
            {
                struct cli_filter *filt = cli->filters;

                // call one last time to clean up
                filt->filter(cli, NULL, filt->data);
                cli->filters = filt->next;
                free(filt);
            }

            return rc;
        }
        else if (cli->mode > MODE_CONFIG && c->mode == MODE_CONFIG)
        {
            // command matched but from another mode,
            // remember it if we fail to find correct command
            again_config = c;
        }
        else if (c->mode == MODE_ANY)
        {
            // command matched but for any mode,
            // remember it if we fail to find correct command
            again_any = c;
        }
    }

    // drop out of config submode if we have matched command on MODE_CONFIG
    if (again_config)
    {
        c = again_config;
        cli_set_configmode(cli, MODE_CONFIG, NULL);
        goto AGAIN;
    }
    if (again_any)
    {
        c = again_any;
        goto AGAIN;
    }

    if (start_word == 0)
        cli_error(cli, "Invalid %s \"%s\"", commands->parent ? "argument" : "command", words[start_word]);

    return CLI_ERROR_ARG;
}

int cli_run_command(struct cli_def *cli, const char *command)
{
    int r;
    unsigned int num_words, i, f;
    char *words[CLI_MAX_LINE_WORDS] = {0};
    int filters[CLI_MAX_LINE_WORDS] = {0};

    if (!command) return CLI_ERROR;
    while (isspace(*command))
        command++;

    if (!*command) return CLI_OK;

    num_words = cli_parse_line(command, words, CLI_MAX_LINE_WORDS);
    for (i = f = 0; i < num_words && f < CLI_MAX_LINE_WORDS - 1; i++)
    {
        if (words[i][0] == '|')
            filters[f++] = i;
    }

    filters[f] = 0;

    if (num_words)
        r = cli_find_command(cli, cli->commands, num_words, words, 0, filters);
    else
        r = CLI_ERROR;

    for (i = 0; i < num_words; i++)
        free(words[i]);

    if (r == CLI_QUIT)
        return r;

    return CLI_OK;
}

static int cli_get_completions(struct cli_def *cli, const char *command, char **completions, int max_completions)
{
    struct cli_command *c;
    struct cli_command *n;
    int num_words, save_words, i, k=0;
    char *words[CLI_MAX_LINE_WORDS] = {0};
    int filter = 0;

    if (!command) return 0;
    while (isspace(*command))
        command++;

    save_words = num_words = cli_parse_line(command, words, sizeof(words)/sizeof(words[0]));
    if (!command[0] || command[strlen(command)-1] == ' ')
        num_words++;

    if (!num_words)
        goto out;

    for (i = 0; i < num_words; i++)
    {
        if (words[i] && words[i][0] == '|')
            filter = i;
    }

    if (filter) // complete filters
    {
        unsigned len = 0;

        if (filter < num_words - 1) // filter already completed
            goto out;

        if (filter == num_words - 1)
            len = strlen(words[num_words-1]);

        for (i = 0; filter_cmds[i].cmd && k < max_completions; i++)
        {
            if (!len || (len < strlen(filter_cmds[i].cmd) && !strncmp(filter_cmds[i].cmd, words[num_words - 1], len)))
                completions[k++] = (char *)filter_cmds[i].cmd;
        }

        completions[k] = NULL;
        goto out;
    }

    for (c = cli->commands, i = 0; c && i < num_words && k < max_completions; c = n)
    {
        n = c->next;

        if (cli->privilege < c->privilege)
            continue;

        if (c->mode != cli->mode && c->mode != MODE_ANY)
            continue;

        if (words[i] && strncasecmp(c->command, words[i], strlen(words[i])))
            continue;

        if (i < num_words - 1)
        {
            if (strlen(words[i]) < c->unique_len)
                continue;

            n = c->children;
            i++;
            continue;
        }

        completions[k++] = c->command;
    }

out:
    for (i = 0; i < save_words; i++)
        free(words[i]);

    return k;
}

static void cli_clear_line(int sockfd, char *cmd, int l, int cursor)
{
    int i;
    if (cursor < l)
    {
        for (i = 0; i < (l - cursor); i++)
            __write(sockfd, " ", 1);
    }
    for (i = 0; i < l; i++)
        cmd[i] = '\b';
    for (; i < l * 2; i++)
        cmd[i] = ' ';
    for (; i < l * 3; i++)
        cmd[i] = '\b';
    __write(sockfd, cmd, i);
    memset((char *)cmd, 0, i);
    l = cursor = 0;
}

void cli_reprompt(struct cli_def *cli)
{
    if (!cli) return;
    cli->showprompt = 1;
}

/**
 * Adds a callback function which will be called every second that a user is
 * connected to the cli. This can be used for regular processing such as
 * debugging, time counting or implementing idle timeouts.
 *
 * Pass NULL as the callback function to disable this at runtime.
 *
 * If the callback function does not return CLI_OK, then the user will be
 * disconnected.
 *
 * @param cli
 * @param callback
 */
void cli_regular(struct cli_def *cli, int (*callback)(struct cli_def *cli))
{
    if (!cli) return;
    cli->regular_callback = callback;
}

void cli_regular_interval(struct cli_def *cli, int seconds)
{
    if (seconds < 1) seconds = 1;
    cli->timeout_tm.tv_sec = seconds;
    cli->timeout_tm.tv_usec = 0;
}

#define DES_PREFIX "{crypt}"        /* to distinguish clear text from DES crypted */
#define MD5_PREFIX "$1$"

static int pass_matches(const char *pass, const char *try)
{
    int des;
    if ((des = !strncasecmp(pass, DES_PREFIX, sizeof(DES_PREFIX)-1)))
        pass += sizeof(DES_PREFIX)-1;

#ifndef WIN32
    /**
     * \todo - find a small crypt(3) function for use on windows
     */
    if (des || !strncmp(pass, MD5_PREFIX, sizeof(MD5_PREFIX)-1))
        try = crypt(try, pass);
#endif

    return !strcmp(pass, try);
}

#undef CTRL
#define CTRL(c) (c - '@')

static int show_prompt(struct cli_def *cli, int sockfd)
{
    int len = 0;

    len += write(sockfd, "[", 1);

    // User name
    //len += write(sockfd, "@", 1);

    // Host name
    if (cli->hostname) {
        len += write(sockfd, ANSI_GREEN, strlen(ANSI_LIGHTCYAN));
        len += write(sockfd, cli->hostname, strlen(cli->hostname));
        len += write(sockfd, ANSI_NONE, strlen(ANSI_NONE));
    }

    // Mode
    if (cli->modestring) {
        len += write(sockfd, cli->modestring, strlen(cli->modestring));
    }

    return len + write(sockfd, cli->promptchar, strlen(cli->promptchar));
}

struct cli_loop_data_def {

        int l, oldl, is_telnet_option, skip, esc;
        int cursor, insertmode;
        char *cmd, *oldcmd;
        char *username, *password;

        signed int in_history;
        int last_was_history;
        int lastchar;
        struct timeval tm;

        struct cli_def *cli;
        int sockfd;
        ev_io w;
        ev_idle iw;
        ev_timer tw;
};

void cli_loop_set_show_prompt(struct cli_loop_data_def *cd) {

    cd->cli->showprompt = 1;

    if (cd->oldcmd)
    {
        cd->l = cd->cursor = cd->oldl;
        (cd->oldcmd)[cd->l] = 0;
        cd->cli->showprompt = 1;
        cd->oldcmd = NULL;
        cd->oldl = 0;
    }
    else
    {
        memset(cd->cmd, 0, CLI_MAX_LINE_LENGTH);
        cd->l = 0;
        cd->cursor = 0;
    }

    memcpy(&cd->tm, &cd->cli->timeout_tm, sizeof(cd->tm));
}

/**
 * Show prompt before select or ask User/Password
 *
 * @param cd Pointer to cli_loop_data_def structure
 */
void cli_loop_show_prompt(struct cli_loop_data_def *cd) { //struct cli_def *cli, int sockfd, int cursor, char *cmd, int l) {

    if (cd->cli->showprompt)
    {
          // Empty line before prompt
//        if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
//            _write(cd->sockfd, "\r\n", 2);

        switch (cd->cli->state)
        {
            case STATE_LOGIN: {
                const char *username = "\r\nUsername: ";
                __write(cd->sockfd, username, strlen(username));
                break;
            }

            case STATE_PASSWORD:
                __write(cd->sockfd, "Password: ", strlen("Password: "));
                break;

            case STATE_NORMAL:
            case STATE_ENABLE:
                show_prompt(cd->cli, cd->sockfd);
                __write(cd->sockfd, cd->cmd, cd->l);
                if (cd->cursor < cd->l)
                {
                    int n = cd->l - cd->cursor;
                    while (n--)
                        __write(cd->sockfd, "\b", 1);
                }
                break;

            case STATE_ENABLE_PASSWORD:
                __write(cd->sockfd, "Password: ", strlen("Password: "));
                break;

        }

        cd->cli->showprompt = 0;
    }
}

/**
 * Timeout every second
 *
 * @param cd Pointer to cli_loop_data_def structure
 * @return
 */
int cli_loop_idle(struct cli_loop_data_def *cd) {

    /* timeout every second */
    if (cd->cli->regular_callback && cd->cli->regular_callback(cd->cli) != CLI_OK)
    {
        cd->l = -1;
        return 1; //break;
    }

    if (cd->cli->idle_timeout)
    {
        if (time(NULL) - cd->cli->last_action >= cd->cli->idle_timeout)
        {
            if (cd->cli->idle_timeout_callback)
            {
                // Call the callback and continue on if successful
                if (cd->cli->idle_timeout_callback(cd->cli) == CLI_OK)
                {
                    // Reset the idle timeout counter
                    time(&cd->cli->last_action);
                    return 0; //continue;
                }
            }

            // Otherwise, break out of the main loop
            cd->l = -1;
            return 1; //break;
        }
    }

    memcpy(&cd->tm, &cd->cli->timeout_tm, sizeof(cd->tm));
    return 0; //continue;
}

int cli_loop_read(struct cli_loop_data_def *cd) {

    int n;
    unsigned char c;

    // Read char
    if ((n = read(cd->sockfd, &c, 1)) < 0) {

        if (errno == EINTR)
            return 0; //continue;

        perror("read");
        cd->l = -1;
        return 1; //break;
    }

    if (cd->cli->idle_timeout) time(&cd->cli->last_action);

    if (n == 0) {
        cd->l = -1;
        return 1; //break;
    }

    if (cd->skip) {
        (cd->skip)--;
        return 0; //continue;
    }

    // CLIType: Telnet option
    if (c == 255 && !cd->is_telnet_option)
    {
        (cd->is_telnet_option)++;
        return 0; //continue;
    }
    if (cd->is_telnet_option)
    {
        if (c >= 251 && c <= 254)
        {
            cd->is_telnet_option = c;
            return 0; //continue;
        }

        if (c != 255)
        {
            cd->is_telnet_option = 0;
            return 0; //continue;
        }

        cd->is_telnet_option = 0;
    }

    // CLIType: Handle ANSI arrows
    if (cd->esc) {

        if (cd->esc == '[')
        {
            /* remap to readline control codes */
            switch (c)
            {
                case 'A': /* Up */
                    c = CTRL('P');
                    break;

                case 'B': /* Down */
                    c = CTRL('N');
                    break;

                case 'C': /* Right */
                    c = CTRL('F');
                    break;

                case 'D': /* Left */
                    c = CTRL('B');
                    break;

                default:
                    c = 0;
            }

            cd->esc = 0;
        }
        else
        {
            cd->esc = (c == '[') ? c : 0;
            return 0; //continue;
        }
    }

    // CLIType: Ignore
    if (c == 0 || c == '\n') {
        cd->last_was_history = 0; // Set last key was history key
        return 0; //continue;
    }

    // CLIType: Line feed
    if (c == '\r') {

        cd->last_was_history = 0; // Set last key was history key
        if (cd->cli->state != STATE_PASSWORD &&
            cd->cli->state != STATE_ENABLE_PASSWORD)
            __write(cd->sockfd, "\r\n", 2);

        return 1; //break;
    }

    // CLIType: ESC
    if (c == 27) {

        cd->esc = 1;
        return 0; //continue;
    }

    // CLIType: CTRL+C
    if (c == CTRL('C')) {

        cd->last_was_history = 0; // Set last key was history key
        __write(cd->sockfd, "\a", 1);
        return 0; //continue;
    }

    // CLIType: Back word, backspace/delete
    if (c == CTRL('W') || c == CTRL('H') || c == 0x7f) {

        int back = 0;

        cd->last_was_history = 0; // Set last key was history key
        if (c == CTRL('W')) /* word */
        {
            int nc = cd->cursor;

            if (cd->l == 0 || cd->cursor == 0)
                return 0; //continue;

            while (nc && cd->cmd[nc - 1] == ' ')
            {
                nc--;
                back++;
            }

            while (nc && cd->cmd[nc - 1] != ' ')
            {
                nc--;
                back++;
            }
        }
        else /* char */
        {
            if (cd->l == 0 || cd->cursor == 0)
            {
                __write(cd->sockfd, "\a", 1);
                return 0; //continue;
            }

            back = 1;
        }

        if (back)
        {
            while (back--)
            {
                if (cd->l == cd->cursor)
                {
                    cd->cmd[--(cd->cursor)] = 0;
                    if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
                        __write(cd->sockfd, "\b \b", 3);
                }
                else
                {
                    int i;
                    (cd->cursor)--;
                    if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
                    {
                        for (i = cd->cursor; i <= cd->l; i++) cd->cmd[i] = cd->cmd[i+1];
                        __write(cd->sockfd, "\b", 1);
                        __write(cd->sockfd, cd->cmd + cd->cursor, strlen(cd->cmd + cd->cursor));
                        __write(cd->sockfd, " ", 1);
                        for (i = 0; i <= (int)strlen(cd->cmd + cd->cursor); i++)
                            __write(cd->sockfd, "\b", 1);
                    }
                }
                (cd->l)--;
            }

            return 0; //continue;
        }
    }

    // CLIType: Redraw
    if (c == CTRL('L')) {

        int i;
        int cursorback = cd->l - cd->cursor;

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cli->state == STATE_PASSWORD ||
            cd->cli->state == STATE_ENABLE_PASSWORD)
            return 0; //continue;

        __write(cd->sockfd, "\r\n", 2);
        show_prompt(cd->cli, cd->sockfd);
        __write(cd->sockfd, cd->cmd, cd->l);

        for (i = 0; i < cursorback; i++)
            __write(cd->sockfd, "\b", 1);

        return 0; //continue;
    }

    // CLIType: Clear line
    if (c == CTRL('U')) {

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cli->state == STATE_PASSWORD ||
            cd->cli->state == STATE_ENABLE_PASSWORD)
            memset(cd->cmd, 0, cd->l);
        else
            cli_clear_line(cd->sockfd, cd->cmd, cd->l, cd->cursor);

        cd->l = cd->cursor = 0;
        return 0; //continue;
    }

    // CLIType: Kill to EOL
    if (c == CTRL('K')) {

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cursor == cd->l) return 0; //continue;

        if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
        {
            int c;
            for (c = cd->cursor; c < cd->l; c++)
                __write(cd->sockfd, " ", 1);

            for (c = cd->cursor; c < cd->l; c++)
                __write(cd->sockfd, "\b", 1);
        }

        memset(cd->cmd + cd->cursor, 0, cd->l - cd->cursor);
        cd->l = cd->cursor;
        return 0; //continue;
    }

    // CLIType: EOT
    if (c == CTRL('D')) {

        // Next line
        __write(cd->sockfd, "\r\n", 2);

        // Exit immediately during authentication
        if(cd->cli->state == STATE_PASSWORD ||
            cd->cli->state == STATE_ENABLE_PASSWORD) {

            cd->l = -1; // initiate quit
            return 1; //break;
        }

        // Exit as exit command (to level up or exit)
        if(cli_int_exit(cd->cli, NULL, NULL, 0) == CLI_QUIT) {

            cd->l = -1; // initiate quit
            return 1; //break;
        }

        // Show prompt and continue
        cd->cli->showprompt = 1;
        return 0; //continue;

//        if(cd->l)
//            return 0; //continue;
//
//        cd->l = -1;
//        return 1; //break;
    }

    // CLIType: Disable
    if (c == CTRL('Z')) {

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cli->mode != MODE_EXEC) {

            cli_clear_line(cd->sockfd, cd->cmd, cd->l, cd->cursor);
            cli_set_configmode(cd->cli, MODE_EXEC, NULL);
            cd->cli->showprompt = 1;
        }

        return 0; //continue;
    }

    // CLIType: TAB completion
    if (c == CTRL('I')) {

        char *completions[CLI_MAX_LINE_WORDS];
        int num_completions = 0;

        //cd->last_was_history = 0; // Set last key was history key

        if (cd->cli->state == STATE_LOGIN ||
            cd->cli->state == STATE_PASSWORD ||
            cd->cli->state == STATE_ENABLE_PASSWORD)
            return 0; //continue;

        if (cd->cursor != cd->l) return 0; //continue;

        num_completions = cli_get_completions(cd->cli, cd->cmd, completions, CLI_MAX_LINE_WORDS);
        if (num_completions == 0)
        {
            __write(cd->sockfd, "\a", 1);
        }
        else if (num_completions == 1)
        {
            // Single completion
            for (; cd->l > 0; (cd->l)--, (cd->cursor)--)
            {
                if (cd->cmd[cd->l-1] == ' ' || cd->cmd[cd->l-1] == '|')
                    break;
                __write(cd->sockfd, "\b", 1);
            }
            strcpy((cd->cmd + cd->l), completions[0]);
            cd->l += strlen(completions[0]);
            cd->cmd[(cd->l)++] = ' ';
            cd->cursor = cd->l;
            __write(cd->sockfd, completions[0], strlen(completions[0]));
            __write(cd->sockfd, " ", 1);
        }
        else if (cd->lastchar == CTRL('I'))
        {
            // double tab
            int i;
            __write(cd->sockfd, "\r\n", 2);
            for (i = 0; i < num_completions; i++)
            {
                __write(cd->sockfd, completions[i], strlen(completions[i]));
                if (i % 4 == 3)
                    __write(cd->sockfd, "\r\n", 2);
                else
                    __write(cd->sockfd, "     ", 1);
            }
            if (i % 4 != 3) __write(cd->sockfd, "\r\n", 2);
                cd->cli->showprompt = 1;
        }
        else
        {
            // More than one completion
            cd->lastchar = c;
            __write(cd->sockfd, "\a", 1);
        }
        return 0; //continue;
    }

    // CLIType: History
    if (c == CTRL('P') || c == CTRL('N')) {

        int history_found = 0;

        // Skip Login, password & enable
        if (cd->cli->state == STATE_LOGIN ||
            cd->cli->state == STATE_PASSWORD ||
            cd->cli->state == STATE_ENABLE_PASSWORD)
            return 0; //continue;

        // Key Up Pressed
        if (c == CTRL('P')) // Up
        {
            if(cd->last_was_history) (cd->in_history)--;
            if ((cd->in_history) < 0)
            {
                for ((cd->in_history) = MAX_HISTORY-1; (cd->in_history) >= 0; (cd->in_history)--)
                {
                    if (cd->cli->history[(cd->in_history)])
                    {
                        history_found = 1;
                        break;
                    }
                }
            }
            else
            {
                if (cd->cli->history[(cd->in_history)]) history_found = 1;
            }
        }

        // Key Down pressed
        else // Down
        {
            if(cd->last_was_history) (cd->in_history)++;
            if ((cd->in_history) >= MAX_HISTORY || !cd->cli->history[(cd->in_history)])
            {
                int i = 0;
                for (i = 0; i < MAX_HISTORY; i++)
                {
                    if (cd->cli->history[i])
                    {
                        (cd->in_history) = i;
                        history_found = 1;
                        break;
                    }
                }
            }
            else
            {
                if (cd->cli->history[(cd->in_history)]) history_found = 1;
            }
        }

        if (history_found && cd->cli->history[(cd->in_history)]) {

            // Show history item
            cli_clear_line(cd->sockfd, cd->cmd, cd->l, cd->cursor);
            memset(cd->cmd, 0, CLI_MAX_LINE_LENGTH);
            strncpy(cd->cmd, cd->cli->history[(cd->in_history)], CLI_MAX_LINE_LENGTH - 1);
            cd->l = cd->cursor = strlen(cd->cmd);
            __write(cd->sockfd, cd->cmd, cd->l);
        }

        // Set last key was history key
        cd->last_was_history = 1;

        return 0; //continue;
    }

    // CLIType: Left/right cursor motion
    if (c == CTRL('B') || c == CTRL('F')) {

        //cd->last_was_history = 0; // Set last key was history key

        if (c == CTRL('B')) /* Left */
        {
            if (cd->cursor)
            {
                if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
                    __write(cd->sockfd, "\b", 1);

                (cd->cursor)--;
            }
        }
        else /* Right */
        {
            if (cd->cursor < cd->l)
            {
                if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
                    __write(cd->sockfd, &cd->cmd[cd->cursor], 1);

                (cd->cursor)++;
            }
        }

        return 0; //continue;
    }

    // CLIType: Start of line
    if (c == CTRL('A')) {

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cursor)
        {
            if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
            {
                __write(cd->sockfd, "\r", 1);
                show_prompt(cd->cli, cd->sockfd);
            }

            cd->cursor = 0;
        }

        return 0; //continue;
    }

    // CLIType: End of line
    if (c == CTRL('E')) {

        cd->last_was_history = 0; // Set last key was history key

        if (cd->cursor < cd->l)
        {
            if (cd->cli->state != STATE_PASSWORD && cd->cli->state != STATE_ENABLE_PASSWORD)
                __write(cd->sockfd, &cd->cmd[cd->cursor], cd->l - cd->cursor);

            cd->cursor = cd->l;
        }

        return 0; //continue;
    }

    // CLIType: Normal character typed
    if (cd->cursor == cd->l) {

        //cd->last_was_history = 0; // Set last key was history key

        // Append to end of line
        cd->cmd[cd->cursor] = c;
        if (cd->l < CLI_MAX_LINE_LENGTH - 1)
        {
            (cd->l)++;
            (cd->cursor)++;
        }
        else
        {
            __write(cd->sockfd, "\a", 1);
            return 0; //continue;
        }
    }
    else
    {
        cd->last_was_history = 0; // Set last key was history key

        // Middle of text
        if (cd->insertmode)
        {
            int i;
            // Move everything one character to the right
            if (cd->l >= CLI_MAX_LINE_LENGTH - 2) (cd->l)--;
            for (i = cd->l; i >= cd->cursor; i--)
                cd->cmd[i + 1] = cd->cmd[i];
            // Write what we've just added
            cd->cmd[cd->cursor] = c;

            __write(cd->sockfd, &cd->cmd[cd->cursor], cd->l - cd->cursor + 1);
            for (i = 0; i < (cd->l - cd->cursor + 1); i++)
                __write(cd->sockfd, "\b", 1);
            (cd->l)++;
        }
        else
        {
            cd->cmd[cd->cursor] = c;
        }
        (cd->cursor)++;
    }

    if (cd->cli->state != STATE_PASSWORD &&
        cd->cli->state != STATE_ENABLE_PASSWORD) {

        if (c == '?' && cd->cursor == cd->l)
        {
            __write(cd->sockfd, "\r\n", 2);
            cd->oldcmd = cd->cmd;
            cd->oldl = cd->cursor = cd->l - 1;
            return 1; //break;
        }
        __write(cd->sockfd, &c, 1);
    }

    cd->oldcmd = 0;
    cd->oldl = 0;
    cd->lastchar = c;

    return 0; //continue;
}

int cli_loop_select_read(struct cli_loop_data_def *cd) {

    int sr;
    fd_set r;

    FD_ZERO(&r);
    FD_SET(cd->sockfd, &r);

    // Select
    if ((sr = select(cd->sockfd + 1, &r, NULL, NULL, &cd->tm)) < 0) {

        // select error
        if (errno == EINTR)
            return 0; // continue;

        perror("select");
        cd->l = -1;
        return 1; // break;
    }

    // Timeout every second
    if (sr == 0) {

        if(cli_loop_idle(cd)) return 1; // break;
        else return 0; // continue;
    }

    if(cli_loop_read(cd)) return 1; // break;
    else return 0; // continue;
}

int cli_loop_check_state(struct cli_loop_data_def *cd) {

    if (cd->l < 0) return 1; // break;

    if (cd->cli->state == STATE_LOGIN) {

        if (cd->l == 0) return 0; //continue;

        // Require login
        free_z(cd->username);
        if (!(cd->username = strdup(cd->cmd)))
            return 0;
        cd->cli->state = STATE_PASSWORD;
        cd->cli->showprompt = 1;
    }

    else if (cd->cli->state == STATE_PASSWORD) {

        // Require password
        int allowed = 0;

        free_z(cd->password);
        if (!(cd->password = strdup(cd->cmd))) return 0;
        if (cd->cli->auth_callback) {

            if (cd->cli->auth_callback(cd->username, cd->password) == CLI_OK)
                allowed++;
        }

        if (!allowed) {

            struct unp *u;
            for (u = cd->cli->users; u; u = u->next) {

                if (!strcmp(u->username, cd->username) &&
                    pass_matches(u->password, cd->password)) {

                    allowed++;
                    return 1; // break;
                }
            }
        }

        if (allowed) {
            cli_error(cd->cli, "\r\n");
            cd->cli->state = STATE_NORMAL;
        }
        else {
            cli_error(cd->cli, "\r\nAccess denied");
            free_z(cd->username);
            free_z(cd->password);
            cd->cli->state = STATE_LOGIN;
        }

        cd->cli->showprompt = 1;
    }

    else if (cd->cli->state == STATE_ENABLE_PASSWORD) {

        int allowed = 0;
        if (cd->cli->enable_password) {
            // Check stored static enable password
            if (pass_matches(cd->cli->enable_password, cd->cmd))
                allowed++;
        }

        if (!allowed && cd->cli->enable_callback) {
            // Check callback
            if (cd->cli->enable_callback(cd->cmd))
                allowed++;
        }

        if (allowed) {
            cli_error(cd->cli, " ");
            cd->cli->state = STATE_ENABLE;
            cli_set_privilege(cd->cli, PRIVILEGE_PRIVILEGED);
        }
        else {
            cli_error(cd->cli, "\r\nAccess denied");
            cd->cli->state = STATE_NORMAL;
        }
    }
    else
    {
        if (cd->l == 0) return 0; //continue;
        if (cd->cmd[cd->l - 1] != '?' && strcasecmp(cd->cmd, "history") != 0)
            cd->in_history = cli_add_history(cd->cli, cd->cmd);

        if (cli_run_command(cd->cli, cd->cmd) == CLI_QUIT)
            return 1; // break;
    }

    // Update the last_action time now as the last command run could take a
    // long time to return
    if (cd->cli->idle_timeout) time(&cd->cli->last_action);

    return 0;
}

void cli_loop_free(struct cli_loop_data_def *cd) {

    // Disconnected
    cli_free_history(cd->cli);
    free_z(cd->username);
    free_z(cd->password);
    free_z(cd->cmd);

    fclose(cd->cli->client);
    cd->cli->client = 0;
    #ifndef WIN32
    shutdown(cd->sockfd, SHUT_RDWR);
    #endif
    close(cd->sockfd);
    free(cd->cli);
    free(cd);
}

void cli_loop_read_cb (EV_P_ ev_io *w, int revents) {

    struct cli_loop_data_def *cd = w->data;

    if(cli_loop_read(cd)) {

        if(cli_loop_check_state(cd)) {

            // Stop this watcher
            ev_io_stop(EV_A_ w);

            // Stop timer watchers
            ev_timer_stop(EV_A_ &cd->tw);
            ev_idle_stop(EV_A_ &cd->iw);

            cli_loop_free(cd);
            return; // break;
        }
        cli_loop_set_show_prompt(cd);
    }

    cli_loop_show_prompt(cd);
}

void cli_loop_timer_cb (EV_P_ ev_timer *w, int revents) {

    // Stop this watcher
    ev_timer_stop(EV_A_ w);

    struct cli_loop_data_def *cd = w->data;

    // Start idle watcher
    ev_idle_start(EV_A_ & cd->iw);
}

void cli_loop_idle_cb (EV_P_ ev_idle *w, int revents) {

    // Stop this watcher
    ev_idle_stop(EV_A_ w);
    struct cli_loop_data_def *cd = w->data;

    // Check cli loop idle action
    if(cli_loop_idle(cd)) {

        // Stop io watcher
        ev_io_stop(EV_A_ & cd->w);

        cli_loop_free(cd);
        return; // break;
    }

    // Start timer watcher
    else {
        ev_timer_start(EV_A_ & cd->tw);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

/**
 * The main loop of the command-line environment. This must be called with the
 * FD of a socket open for bi-directional communication (sockfd).
 *
 * cli_loop() handles the telnet negotiation and authentication. It returns
 * only when the connection is finished, either by a server or client disconnect.
 * Returns CLI_OK.
 *
 * @param cli
 * @param sockfd
 * @return
 */
int cli_loop(struct cli_def *cli, int sockfd)
{
    //#define cki_sync_mode

    // Initialize cli loop data
    struct cli_loop_data_def *cd = malloc(sizeof(*cd));

    cd->cli = cli;
    cd->sockfd = sockfd;
    cd->oldl = 0;
    cd->is_telnet_option = 0;
    cd->skip = 0;
    cd->esc = 0;
    cd->cursor = 0;
    cd->insertmode = 1;
    cd->cmd = NULL;
    cd->oldcmd = 0;
    cd->username = NULL;
    cd->password = NULL;
    cd->in_history = 0;
    cd->lastchar = 0;
    cd->last_was_history = 0;

    cli_build_shortest(cli, cli->commands);

    if(cli->auth_callback)
        cli->state = STATE_LOGIN;
    else
        cli->state = STATE_NORMAL;

    cli_free_history(cli);
    if (cli->telnet_protocol)
    {
        static const char *negotiate =
            "\xFF\xFB\x03"
            "\xFF\xFB\x01"
            "\xFF\xFD\x03"
            "\xFF\xFD\x01";
        __write(sockfd, negotiate, strlen(negotiate));
    }

    if ((cd->cmd = malloc(CLI_MAX_LINE_LENGTH)) == NULL) {
        free(cd);
        return CLI_ERROR;
    }

    #ifdef WIN32
    /*
     * OMG, HACK
     */
    if (!(cli->client = fdopen(_open_osfhandle(sockfd, 0), "w+"))) {
        free(cd->cmd);
        free(cd);
        return CLI_ERROR;
    }
    cli->client->_file = sockfd;
    #else
    if (!(cli->client = fdopen(sockfd, "w+"))) {
        free(cd->cmd);
        free(cd);
        return CLI_ERROR;
    }
    #endif

    setbuf(cli->client, NULL);
    if (cli->banner)
        cli_error(cli, "%s", cli->banner);

    // Set the last action now so we don't time immediately
    if (cli->idle_timeout)
        time(&cli->last_action);

    /* start off in unprivileged mode */
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);

    // no auth required?
    if (!cli->users && !cli->auth_callback) cli->state = STATE_NORMAL;

    cli_loop_set_show_prompt(cd);
    cli_loop_show_prompt(cd);

    // Sync connection to TCP server
    #ifdef cki_sync_mode

    while(1) {

        // Select and read data
        if(cli_loop_select_read(cd)) {

            if(cli_loop_check_state(cd)) {

                //! \todo: Stop and free watcher

                cli_loop_free(cd);
                break;
            }
            cli_loop_set_show_prompt(cd);
        }

        cli_loop_show_prompt(cd);
    }

    // Async connection to TCP server
    #else

    // Create and start watcher (start client processing)
    ev_init (&cd->w, cli_loop_read_cb);
    ev_io_set (&cd->w, cd->sockfd, EV_READ);
    cd->w.data = cd;
    ev_io_start (cli->ke->ev_loop, &cd->w);

    // Initialize and start timer watcher and its idle watcher
    ev_idle_init (&cd->iw, cli_loop_idle_cb);
    cd->iw.data = cd;
    ev_timer_init (&cd->tw, cli_loop_timer_cb, 1.0, 1.0);
    cd->tw.data = cd;
    ev_timer_start (cli->ke->ev_loop, &cd->tw);

    #endif

    return CLI_OK;
    #undef sync_mode
}

#pragma GCC diagnostic pop

/**
 * This reads and processes every line read from f as if it were entered at the
 * console. The privilege level will be set to privilege and mode set to mode
 * during the processing of the file.
 *
 * @param cli
 * @param fh
 * @param privilege
 * @param mode
 * @return
 */
int cli_file(struct cli_def *cli, FILE *fh, int privilege, int mode)
{
    int oldpriv = cli_set_privilege(cli, privilege);
    int oldmode = cli_set_configmode(cli, mode, NULL);
    char buf[CLI_MAX_LINE_LENGTH];

    while (1)
    {
        char *p;
        char *cmd;
        char *end;

        if (fgets(buf, CLI_MAX_LINE_LENGTH - 1, fh) == NULL)
            break; /* end of file */

        if ((p = strpbrk(buf, "#\r\n")))
            *p = 0;

        cmd = buf;
        while (isspace(*cmd))
            cmd++;

        if (!*cmd)
            continue;

        for (p = end = cmd; *p; p++)
            if (!isspace(*p))
                end = p;

        *++end = 0;
        if (strcasecmp(cmd, "quit") == 0)
            break;

        if (cli_run_command(cli, cmd) == CLI_QUIT)
            break;
    }

    cli_set_privilege(cli, oldpriv);
    cli_set_configmode(cli, oldmode, NULL /* didn't save desc */);

    return CLI_OK;
}

static void _print(struct cli_def *cli, int print_mode, const char *format, va_list ap)
{
    va_list aq;
    int n;
    char *p;

    if (!cli) return; // sanity check

    while (1)
    {
        va_copy(aq, ap);
        if ((n = vsnprintf(cli->buffer, cli->buf_size, format, ap)) == -1)
            return;

        if ((unsigned)n >= cli->buf_size)
        {
            cli->buf_size = n + 1;
            cli->buffer = realloc(cli->buffer, cli->buf_size);
            if (!cli->buffer)
                return;
            va_end(ap);
            va_copy(ap, aq);
            continue;
        }
        break;
    }


    p = cli->buffer;
    do
    {
        char *next = strchr(p, '\n');
        struct cli_filter *f = (print_mode & PRINT_FILTERED) ? cli->filters : 0;
        int print = 1;

        if (next)
            *next++ = 0;
        else if (print_mode & PRINT_BUFFERED)
            break;

        while (print && f)
        {
            print = (f->filter(cli, p, f->data) == CLI_OK);
            f = f->next;
        }
        if (print)
        {
            if (cli->print_callback)
                cli->print_callback(cli, p);
            else if (cli->client)
                fprintf(cli->client, "%s\r\n", p);
        }

        p = next;
    } while (p);

    if (p && *p)
    {
        if (p != cli->buffer)
        memmove(cli->buffer, p, strlen(p));
    }
    else *cli->buffer = 0;
}

void cli_bufprint(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_BUFFERED|PRINT_FILTERED, format, ap);
    va_end(ap);
}

void cli_vabufprint(struct cli_def *cli, const char *format, va_list ap)
{
    _print(cli, PRINT_BUFFERED, format, ap);
}

/**
 * This function should be called for any output generated by a command callback.
 *
 * It takes a printf() style format string and a variable number of arguments.
 *
 * Be aware that any output generated by cli_print() will be passed through any
 * filter currently being applied, and the output will be redirected to the
 * cli_print_callback() if one has been specified.
 *
 * @param cli
 * @param format
 * @param ...
 */
void cli_print(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_FILTERED, format, ap);
    va_end(ap);
}

/**
 * A variant of cli_print() which does not have filters applied.
 *
 * @param cli
 * @param format
 * @param ...
 */
void cli_error(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_PLAIN, format, ap);
    va_end(ap);
}

struct cli_match_filter_state
{
    int flags;
    union {
        char *string;
        regex_t re;
    } match;
};

int cli_match_filter_init(struct cli_def *cli, int argc, char **argv, struct cli_filter *filt)
{
    struct cli_match_filter_state *state;
    int rflags;
    int i;
    char *p;

    if (argc < 2)
    {
        if (cli->client)
            fprintf(cli->client, "Match filter requires an argument\r\n");

        return CLI_ERROR;
    }

    filt->filter = cli_match_filter;
    filt->data = state = calloc(sizeof(struct cli_match_filter_state), 1);

    if (argv[0][0] == 'i' || (argv[0][0] == 'e' && argv[0][1] == 'x'))  // include/exclude
    {
        if (argv[0][0] == 'e')
            state->flags = MATCH_INVERT;

        state->match.string = join_words(argc-1, argv+1);
        return CLI_OK;
    }

#ifdef WIN32
    /*
     * No regex functions in windows, so return an error
     */
    return CLI_ERROR;
#endif

    state->flags = MATCH_REGEX;

    // grep/egrep
    rflags = REG_NOSUB;
    if (argv[0][0] == 'e') // egrep
        rflags |= REG_EXTENDED;

    i = 1;
    while (i < argc - 1 && argv[i][0] == '-' && argv[i][1])
    {
        int last = 0;
        p = &argv[i][1];

        if (strspn(p, "vie") != strlen(p))
            break;

        while (*p)
        {
            switch (*p++)
            {
                case 'v':
                    state->flags |= MATCH_INVERT;
                    break;

                case 'i':
                    rflags |= REG_ICASE;
                    break;

                case 'e':
                    last++;
                    break;
            }
        }

        i++;
        if (last)
            break;
    }

    p = join_words(argc-i, argv+i);
    if ((i = regcomp(&state->match.re, p, rflags)))
    {
        if (cli->client)
            fprintf(cli->client, "Invalid pattern \"%s\"\r\n", p);

        free_z(p);
        return CLI_ERROR;
    }

    free_z(p);
    return CLI_OK;
}

int cli_match_filter(UNUSED(struct cli_def *cli), const char *string, void *data)
{
    struct cli_match_filter_state *state = data;
    int r = CLI_ERROR;

    if (!string) // clean up
    {
        if (state->flags & MATCH_REGEX)
            regfree(&state->match.re);
        else
            free(state->match.string);

        free(state);
        return CLI_OK;
    }

    if (state->flags & MATCH_REGEX)
    {
        if (!regexec(&state->match.re, string, 0, NULL, 0))
            r = CLI_OK;
    }
    else
    {
        if (strstr(string, state->match.string))
            r = CLI_OK;
    }

    if (state->flags & MATCH_INVERT)
    {
        if (r == CLI_OK)
            r = CLI_ERROR;
        else
            r = CLI_OK;
    }

    return r;
}

struct cli_range_filter_state {
    int matched;
    char *from;
    char *to;
};

int cli_range_filter_init(struct cli_def *cli, int argc, char **argv, struct cli_filter *filt)
{
    struct cli_range_filter_state *state;
    char *from = 0;
    char *to = 0;

    if (!strncmp(argv[0], "bet", 3)) // between
    {
        if (argc < 3)
        {
            if (cli->client)
                fprintf(cli->client, "Between filter requires 2 arguments\r\n");

            return CLI_ERROR;
        }

        if (!(from = strdup(argv[1])))
            return CLI_ERROR;
        to = join_words(argc-2, argv+2);
    }
    else // begin
    {
        if (argc < 2)
        {
            if (cli->client)
                fprintf(cli->client, "Begin filter requires an argument\r\n");

            return CLI_ERROR;
        }

        from = join_words(argc-1, argv+1);
    }

    filt->filter = cli_range_filter;
    filt->data = state = calloc(sizeof(struct cli_range_filter_state), 1);

    state->from = from;
    state->to = to;

    return CLI_OK;
}

int cli_range_filter(UNUSED(struct cli_def *cli), const char *string, void *data)
{
    struct cli_range_filter_state *state = data;
    int r = CLI_ERROR;

    if (!string) // clean up
    {
        free_z(state->from);
        free_z(state->to);
        free_z(state);
        return CLI_OK;
    }

    if (!state->matched)
    state->matched = !!strstr(string, state->from);

    if (state->matched)
    {
        r = CLI_OK;
        if (state->to && strstr(string, state->to))
            state->matched = 0;
    }

    return r;
}

int cli_count_filter_init(struct cli_def *cli, int argc, UNUSED(char **argv), struct cli_filter *filt)
{
    if (argc > 1)
    {
        if (cli->client)
            fprintf(cli->client, "Count filter does not take arguments\r\n");

        return CLI_ERROR;
    }

    filt->filter = cli_count_filter;
    if (!(filt->data = calloc(sizeof(int), 1)))
        return CLI_ERROR;

    return CLI_OK;
}

int cli_count_filter(struct cli_def *cli, const char *string, void *data)
{
    int *count = data;

    if (!string) // clean up
    {
        // print count
        if (cli->client)
            fprintf(cli->client, "%d\r\n", *count);

        free(count);
        return CLI_OK;
    }

    while (isspace(*string))
        string++;

    if (*string)
        (*count)++;  // only count non-blank lines

    return CLI_ERROR; // no output
}

/**
 * Whenever cli_print() or cli_error() is called, the output generally goes to
 * the user. If you specify a callback using this function, then the output
 * will be sent to that callback. The function will be called once for each
 * line, and it will be passed a single null-terminated string, without any
 * newline characters.
 *
 * Specifying NULL as the callback parameter will make libcli use the default
 * cli_print() function.
 *
 * @param cli
 * @param callback
 */
void cli_print_callback(struct cli_def *cli, void (*callback)(struct cli_def *, const char *))
{
    cli->print_callback = callback;
}

void cli_set_idle_timeout(struct cli_def *cli, unsigned int seconds)
{
    if (seconds < 1)
        seconds = 0;
    cli->idle_timeout = seconds;
    time(&cli->last_action);
}

void cli_set_idle_timeout_callback(struct cli_def *cli, unsigned int seconds, int (*callback)(struct cli_def *))
{
    cli_set_idle_timeout(cli, seconds);
    cli->idle_timeout_callback = callback;
}

void cli_telnet_protocol(struct cli_def *cli, int telnet_protocol) {
    cli->telnet_protocol = !!telnet_protocol;
}

void cli_set_context(struct cli_def *cli, void *context) {
    cli->user_context = context;
}

void *cli_get_context(struct cli_def *cli) {
    return cli->user_context;
}

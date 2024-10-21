#include "launch_options.h"

// ugly, but readable command check
#define CMD_CHECK(len, e_len, arg, e_arg) (len == e_len) && (strcmp(arg, e_arg) == 0)
#define IS_STREAM(len, arg) CMD_CHECK(len, 1, arg, "stream")
#define IS_LIST(len, arg) CMD_CHECK(len, 1, arg, "list")
#define IS_NEW(len, arg) CMD_CHECK(len, 2, arg, "new")
#define IS_TRACK(len, arg) CMD_CHECK(len, 2, arg, "track")
#define IS_PULL(len, arg) CMD_CHECK(len, 1, arg, "pull")
#define IS_POST(len, arg) CMD_CHECK(len, 1, arg, "post")
#define IS_REGISTER(len, arg) CMD_CHECK(len, 2, arg, "reg")

LAUNCH_SETTINGS parse_config(LAUNCH_SETTINGS ls);
LAUNCH_SETTINGS parse_command(LAUNCH_SETTINGS ls, int argc, char **argv);
LAUNCH_SETTINGS parse_flags(LAUNCH_SETTINGS ls, int argc, char **argv);

LAUNCH_SETTINGS LAUNCH_SETTINGS_construct(int argc, char **argv) {
    LAUNCH_SETTINGS ls;
    ls.append = false;
    ls.remove = false;
    ls.msg = NULL;
    ls.arg = NULL;
    ls.argc = 0;
    ls.cmd = CMD_INVALID;

    ls = parse_flags(ls, argc, argv);
    ls = parse_command(ls, argc, argv);
    ls = parse_config(ls);

    return ls;
};

LAUNCH_SETTINGS parse_config(LAUNCH_SETTINGS ls) {
    uint16_t setting_offset;
    uint8_t *shared;
    uint8_t *home;

    shared = getenv("XDG_DATA_HOME");
    if (shared == NULL) {
        home = getenv("HOME");
        if (home == NULL) {
            home = getpwuid(getuid())->pw_dir;
        }

        // printnts default to path
        sprintf(ls.config_dir, "%s/.local/share", home);
        setting_offset = strlen(ls.config_dir);
    }

    sprintf(ls.config_dir + setting_offset, "/.krauk");
    sprintf(ls.config_file, "%s/self", ls.config_dir);

    return ls;
}

LAUNCH_SETTINGS parse_command(LAUNCH_SETTINGS ls, int argc, char **argv) {
    // sets up arg
    ls.argc = argc - optind;
    ls.arg = argv + optind;

    if (IS_REGISTER(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_REGISTER;
        ls.arg++;
    } else if (IS_TRACK(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_TRACK;
        ls.arg++;
    } else if (IS_NEW(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_NEW;
        ls.arg++;
    } else if (IS_LIST(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_LIST;
        ls.arg++;
    } else if (IS_PULL(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_PULL;
        ls.arg++;
    } else if (IS_POST(ls.argc, ls.arg[0])) {
        ls.cmd = CMD_POST;
        ls.arg++;
    }

    return ls;
}

LAUNCH_SETTINGS parse_flags(LAUNCH_SETTINGS ls, int argc, char **argv) {
    int32_t opt;

    // manage flags
    while ((opt = getopt(argc, argv, "m:a::r::")) != -1) {
        switch (opt) {
            case 'm':
                if (optarg != NULL) {
                    ls.msg = optarg;
                }
                break;
            case 'a':
                ls.append = true;
                ls.remove = false;
                break;
            case 'r':
                ls.append = false;
                ls.remove = true;
                break;
            default:
                // In case of an unexpected option
                fprintf(stderr, "[-] Unexpected option: -%c\n", opt);
                exit(EXIT_FAILURE);
        }
    }

    return ls;
}
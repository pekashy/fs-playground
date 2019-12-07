#include "proc_stat_parse.h"

char *decode_tty(int32_t tty) {
    /*uint32_t minor = tty & 0b11111111111100000000000011111111;
    uint32_t major = tty & 0b00000000000000001111111100000000;*/
    uint32_t minor = MINOR(tty);
    uint32_t major = MAJOR(tty);
    dev_t device = MKDEV(major, minor);
    std::ifstream infile("/proc/devices");
    std::string line;
    char *ttyName;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        int character;
        iss >> character;
        if (character == major) {
            std::string _ttyName;
            iss >> _ttyName;

            if (_ttyName == "/dev/vc/0" || _ttyName == "ttyS") { // some manual fixes, possible to add more
                _ttyName = "tty";
            }
            if (_ttyName == "pts") {
                _ttyName = "pts/";
            }
            _ttyName += std::to_string(minor);
            ttyName = (char *) calloc(_ttyName.length() + 2, sizeof(char));
            strcpy(ttyName, _ttyName.c_str());    // or pass &s[0]
            ttyName[_ttyName.length()] = '\0';
            return ttyName;
        }
    }
    return NULL; // Never get here
};

int main(int argc, char *argv[]) {
    std::string arg(argv[1]);
    if (arg == "-A" || arg == "-e") {
        DIR *dr = opendir("/proc/");
        if (dr == nullptr) {
            printf("Could not open current directory");
            return 0;
        }
        struct dirent *de;
        printf("PID\t| UID\t| TTY\t| COMMAND\n");
        while ((de = readdir(dr)) != NULL) {
            uint32_t _pid = atoi(de->d_name);
            if (_pid) {
                procinfo info;
                get_proc_info(_pid, &info);
                char *tty = (char *) malloc(2 * sizeof(char));
                tty[0] = '?';
                tty[1] = '\0';
                if (info.tty) {
                    free(tty);
                    tty = decode_tty(info.tty);
                }
                printf("%d\t| %s\t| %d\t| %s\n", _pid, tty, info.euid, info.exName);
                free(tty);
            }

        }

    } else if (arg == "-d") {
        printf("Coming soon!");
    } else if (arg == "-f") {
        printf("Coming soon!");
    } else if (arg == "-g") {
        printf("Coming soon!");
    } else if (arg == "-G") {
        printf("Coming soon!");
    } else if (arg == "-l") {
        printf("Coming soon!");
    } else if (arg == "-n") {
        printf("Coming soon!");
    } else if (arg == "-o") {
        printf("Coming soon!");
    } else if (arg == "-p") {
        printf("Coming soon!");
    } else if (arg == "-t") {
        printf("Coming soon!");
    } else if (arg == "-u") {
        printf("Coming soon!");
    } else if (arg == "-U") {
        printf("Coming soon!");
    }

}

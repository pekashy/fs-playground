#include <pwd.h>
#include "proc_stat_parse.h"

using std::string;
using std::vector;

vector<string> get_open_files(char *pid) {
    string pid_s(pid);
    string dir_fd = "/proc/" + pid_s + "/fd/";
    vector<string> opened_files;
    DIR *dr = opendir(dir_fd.c_str());
    if (!dr) {
        return opened_files;
    }
    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        char opened_file[1024];
        int len = readlink((dir_fd + string(de->d_name)).c_str(), opened_file,
                           sizeof(opened_file) - 1);
        if (len) {
            opened_file[len] = 0; // truncate
            opened_files.push_back(opened_file);
        }
    }
}

int main(int argc, char *argv[]) {
    DIR *dr = opendir("/proc/");
    if (dr == nullptr) {
        printf("Could not open current directory");
        return 0;
    }
    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        uint32_t _pid = atoi(de->d_name);
        if (_pid) {
            procinfo info;
            get_proc_info(_pid, &info);
            char userbuff[1024];
            struct passwd us;
            struct passwd *uinfo;
            getpwuid_r(info.euid, &us, userbuff, sizeof(userbuff), &uinfo);
            string username = "none";
            if (uinfo) {
                username = string(uinfo->pw_name);
            }
            std::cout << _pid << "\t" << info.exName << '\t' << username << '\n';
            vector<string> opened_files = get_open_files(de->d_name);
            for (auto f: opened_files) {
                if (f.empty()) {
                    continue;
                }
                std::cout << f << '\n';
            }
        }
    }
}
#pragma once
#ifndef __YUKI_DIRECTORY_H__
#define __YUKI_DIRECTORY_H__

#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include <string>
#include <vector>
#include "log.h"
using namespace std;

namespace Yuki {
    class DirBrowser {
    private:
        static string cur_abs_path;
        static void update_current_path();
        static vector<string> file_paths;
        static string pattern;

        DirBrowser();
        ~DirBrowser() {}

        static void dfs_find_files(bool recursive);

    public:
        // true: directory exits; false: no such directory
        static bool set_directory(string path = ".");
        // return the abs path of the file with certain parten
        static vector<string> find_files(string pattern = "*.*", bool recursive = false);
    };
}

#endif  // !__YUKI_DIRECTORY_H__
#include "directory.h"

namespace Yuki {

    string DirBrowser::cur_abs_path;
    string DirBrowser::pattern;
    vector<string> DirBrowser::file_paths;

    void DirBrowser::update_current_path() {
        char path[_MAX_PATH];
        char vpath[_MAX_PATH];
        getcwd(path, _MAX_PATH);
        // replace all '\' and ' '
        int idx = 0;
        for (int i = 0; path[i]; ++i) {
            if (path[i] == ' ') {
                vpath[idx++] = '\\';
                vpath[idx++] = ' ';
            }
            else if (path[i] == '\\') {
                vpath[idx++] = '/';
            }
            else {
                vpath[idx++] = path[i];
            }
        }
        vpath[idx] = 0;
        cur_abs_path = vpath;
        if (cur_abs_path[cur_abs_path.length() - 1] != '/')
            cur_abs_path += '/';
        LOG::notice("Current path: %s\n", cur_abs_path.c_str());
    }

    DirBrowser::DirBrowser() {}

    bool DirBrowser::set_directory(string path) {
        if (_chdir(path.c_str()) != 0) return false;
        update_current_path();
        return true;
    }

    vector<string> DirBrowser::find_files(string query, bool recursive) {
        file_paths.clear();

        pattern = query;
        dfs_find_files(recursive);

        return file_paths;
    }

    void DirBrowser::dfs_find_files(bool recursive) {
        string old_path = cur_abs_path;

        intptr_t handle;
        _finddata_t info;
        if ((handle = _findfirst(pattern.c_str(), &info)) != -1) {
            // find the first
            do {
                // if not a directory, insert into result
                if (!(info.attrib & _A_SUBDIR)) {
                    string file_path = cur_abs_path;
                    file_path += info.name;
                    file_paths.push_back(file_path);
                    LOG::notice("Find file: %s\n", file_path.c_str());
                }
            } while (_findnext(handle, &info) == 0);
            _findclose(handle);
        }

        if (recursive) {
            vector<string> sub_dirs;
            if ((handle = _findfirst("*", &info)) != -1) {
                // find the first
                do {
                    // sub dir
                    if (info.attrib & _A_SUBDIR) {
                        if (strcmp(info.name, ".") &&
                            strcmp(info.name, ".."))
                            sub_dirs.push_back(info.name);  
                    }
                } while (_findnext(handle, &info) == 0);
                _findclose(handle);
            }
            for (size_t i = 0; i < sub_dirs.size(); ++i) {
                cur_abs_path += sub_dirs[i] + '/';
                _chdir(sub_dirs[i].c_str());
                dfs_find_files(true);
                cur_abs_path = old_path;
                _chdir("..");
            }
        }

        return;
    }

}
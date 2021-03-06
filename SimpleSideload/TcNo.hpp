// WSA-SimpleSideload
// Copyright (C) 2021 TechNobo (Wesley Pyburn)
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Just note that this requires curl to be installed. I used "vcpkg install curl[openssl]:x64-windows-static" in Microsoft's vcpkg to accomplish that.
// This is based on my other project's code: the TcNo Account Switcher - https://github.com/TcNobo/TcNo-Acc-Switcher

#pragma comment(lib, "urlmon.lib")
#pragma comment (lib, "User32.lib")
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <filesystem>
#include <Shlwapi.h>

#include <iostream>
#include <string>
#include <tchar.h>
#include <thread>

#include "progress_bar.hpp"

#include <curl/curl.h>
#include <openssl/ssl.h>
using namespace std;

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>


inline string getExe()
{
    const HMODULE h_module = GetModuleHandleW(nullptr);
    WCHAR pth[MAX_PATH];
    GetModuleFileNameW(h_module, pth, MAX_PATH);
    wstring ws(pth);
    const string path(ws.begin(), ws.end());
    return path;
}

inline std::filesystem::path getOperatingPath() {
    const string path = getExe();
    return path.substr(0, path.find_last_of('\\') + 1);
}

inline void insert_empty_line()
{
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);

    std::string s(static_cast<double>(buffer_info.srWindow.Right) - static_cast<double>(buffer_info.srWindow.Left), ' ');
    std::cout << s << '\r';
}

inline double round_off(const double n) {
    double d = n * 100.0;
    const int i = d + 0.5;
    d = static_cast<float>(i) / 100.0;
    return d;
}

inline size_t write_data(void* ptr, const size_t size, size_t n_mem_b, FILE* stream) {
    const size_t written = fwrite(ptr, size, n_mem_b, stream);
    return written;
}

inline string convert_size(size_t size) {
    static const char* sizes[] = { "B", "KB", "MB", "GB" };
    int div = 0;
    size_t rem = 0;

    while (size >= 1024 && div < (sizeof sizes / sizeof * sizes)) {
        rem = (size % 1024);
        div++;
        size /= 1024;
    }

    const double size_d = static_cast<float>(size) + static_cast<float>(rem) / 1024.0;
    string result = to_string(round_off(size_d)) + " " + sizes[div];
    return result;
}

inline string current_download;
chrono::time_point<chrono::system_clock> last_time = std::chrono::system_clock::now();

inline int progress_bar(
    void* client_progress_data,
    const double dl_total,
    const double dl_now,
    double ul_total,
    double ul_now)
{
    if (const double n = dl_total; n > 0) {
        const string dls = "Downloading " + current_download + " (" + convert_size(static_cast<size_t>(dl_total)) + ")";
        auto bar1 = new ProgressBar(static_cast<unsigned long>(n), dls.c_str());
        if (const std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - last_time; elapsed_seconds.count() >= 0.2)
        {
            last_time = std::chrono::system_clock::now();
            bar1->Progressed(static_cast<unsigned long>(dl_now));
        }
    }
    return 0;
}

// https://stackoverflow.com/a/46348112/5165437
inline bool download_file(const char* url, const char* dest) {
    if (CURL* curl = curl_easy_init(); curl) {
        FILE* fp = fopen(dest, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_bar);
        curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, true);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true); // Follows 301 redirects, like in the WebView2 link
        const CURLcode res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);

        insert_empty_line();

        last_time = std::chrono::system_clock::from_time_t(0);
        cout << " Finished downloading " << current_download << endl;
        return res == CURLE_OK;
    }
    return false;
}


std::string exec(const char* cmd) {
    //cout << "Executing: " << cmd << endl << endl;
    char buffer[128];
    std::string result = "";
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    }
    catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    cout << result << endl;
    return result;
}
string exec(const string cmd) {
    return exec(cmd.c_str());
}

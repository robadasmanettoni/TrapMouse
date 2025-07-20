#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
    int  autolock;
    bool block_keys;
    bool no_pass_rules;
	int  cipher;
    int  win_pos_x;
    int  win_pos_y;
    bool win_center;
    std::string passwordEncrypted;
};

bool LoadSettings(Settings& s);
bool SaveSettings(const Settings& s);

#endif

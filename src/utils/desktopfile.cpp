#include "desktopfile.h"
#include <algorithm>
#include <sstream>
#include "gio/gdesktopappinfo.h"
#include "utils/moreader.h"
#include "utils/string.h"
#include "utils/system.h"

DL_NS_BEGIN
// https://developer.gnome.org/gio/stable/gio-Desktop-file-based-GAppInfo.html#g-desktop-app-info-get-string
namespace desktopfile_util
{
    map<string, appinfo_t> cache;

    GKeyFile* get_key_file(const string& app_name, const string& desktop_file)
    {
        GKeyFile* key_file = nullptr;
        GError* error = nullptr;

        key_file = g_key_file_new();
        auto const found = g_key_file_load_from_file(key_file, desktop_file.c_str(),
                                                     GKeyFileFlags::G_KEY_FILE_NONE, &error);

        if (error) {
            g_warning("desktopfile:get_key_file: ERROR: %s", error->message);
            g_error_free(error);
            error = nullptr;
        }

        if (!found && key_file != nullptr) {
            g_key_file_free(key_file);
            return nullptr;
        }

        GDesktopAppInfo* app_info = g_desktop_app_info_new_from_keyfile(key_file);

        // Gets the value of the NoDisplay key, which helps determine if the application
        // info should be shown in menus. See G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY and
        // g_app_info_should_show().
        if (g_desktop_app_info_get_nodisplay(app_info)) {
            g_key_file_free(key_file);
            return nullptr;
        }

        // A desktop file is hidden if the Hidden key in it is set to True.
        // Hidden should have been called Deleted. It means the user deleted (at his level)
        // something that was present (at an upper level, e.g. in the system dirs).
        // It's strictly equivalent to the .desktop file not existing at all, as far as that
        // user is concerned. This can also be used to "uninstall" existing files
        //(e.g. due to a renaming) - by letting make install install a file with Hidden=true in it.
        if (g_desktop_app_info_get_is_hidden(app_info)) {
            g_key_file_free(key_file);
            return nullptr;
        }

        if (!g_desktop_app_info_has_key(app_info, "Exec")) {
            g_key_file_free(key_file);
            return nullptr;
        }

        return key_file;
    }

    // Recognized desktop entry keys
    // https://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
    void set_info(GKeyFile* key_file, const string& desktop_file, appinfo_t& info)
    {
        GDesktopAppInfo* app_info = g_desktop_app_info_new_from_keyfile(key_file);

        info.m_desktop_file = desktop_file;
        info.m_locale = getenv("LANG");  // en_US.UTF-8

        char* title = g_desktop_app_info_get_locale_string(app_info, "Name");
        if (title == nullptr) {
            title = g_desktop_app_info_get_string(app_info, "Name");
        }

        if (title == nullptr) {
            title = g_desktop_app_info_get_string(app_info, "GenericName");
        }

        if (title) info.m_title = title;

        char* comment = g_desktop_app_info_get_locale_string(app_info, "Comment");
        if (comment == nullptr) {
            comment = g_desktop_app_info_get_string(app_info, "Comment");
        }
        if (comment) info.m_comment = comment;

        char* icon_name = g_desktop_app_info_get_string(app_info, "Icon");
        if (icon_name) info.m_icon_name = icon_name;
    }

    bool get_app_info(WnckWindow* window, appinfo_t& info)
    {
        const std::string extensions[] = {".py", ".exe", ".sh"};

        const char* group = wnck_window_get_class_group_name(window);
        if (group == nullptr) {
            // Wie assume that this application don't have any desktop file.
            // --------------------------------------------------------------
            // Gets the name of window ,
            // Always returns some value, even if window has no name set;
            group = wnck_window_get_name(window);

            // build the essential data
            if (group != nullptr) {
                info.m_group = group;
                info.m_group = string_util::remove_extension(info.m_group, extensions);

                info.m_name = group;
                replace(info.m_name.begin(), info.m_name.end(), ' ', '-');

                info.m_title = group;
                info.m_icon_name = wnck_window_get_icon_name(window);

                return true;
            }

            g_critical("Application group not found!!!\n");
            return false;
        }

        info.m_group = group;
        info.m_group = string_util::remove_extension(info.m_group, extensions);

        const char* instance_name = wnck_window_get_class_instance_name(window);
        if (instance_name != nullptr) {
            info.m_instance = instance_name;
            info.m_instance = string_util::remove_extension(info.m_instance, extensions);
        }

        // the main heuristic app name
        info.m_name = string_util::string_to_lower(info.m_group.c_str());
        size_t idx = info.m_name.find(" ");
        if (idx != string::npos) {
            info.m_name = info.m_name.substr(0, idx);
        }

        info.m_title = info.m_group;
        info.m_icon_name = info.m_name;

        return get_app_info(info);
    }

    bool get_app_info(appinfo_t& info)
    {
        if (info.m_name.empty() || info.m_name == "Untitled window") return false;

        if (cache.count(info.m_name) == 1) {
            auto const cache_info = cache[info.m_name];

            if (cache_info.m_locale == getenv("LANG")) {
                info.m_title = cache_info.m_title;
                info.m_comment = cache_info.m_comment;
                info.m_desktop_file = cache_info.m_desktop_file;
                info.m_icon_name = cache_info.m_icon_name;
                info.m_locale = cache_info.m_locale;
                info.m_cache = true;

                return true;
            }
        }

        GKeyFile* key_file = nullptr;
        bool found = false;
        string desktop_file = "";

        // clang-format off
        const string desktop_directories[] = {
            {"/usr/share/applications/"},
            {"/usr/local/share/applications/"}};
        // clang-format on

        for (auto const directory : desktop_directories) {
            desktop_file = directory + info.m_name + ".desktop";
            if (system_util::file_exists(desktop_file)) {
                key_file = get_key_file(info.m_name, desktop_file);
                if (key_file != nullptr) {
                    set_info(key_file, desktop_file, info);
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            char*** result_list = g_desktop_app_info_search(info.m_name.c_str());
            char*** groups;

            for (groups = result_list; *groups; groups++) {
                string file_name(*groups[0]);

                for (auto const directory : desktop_directories) {
                    desktop_file = directory + file_name;
                    if (system_util::file_exists(desktop_file)) {
                        key_file = get_key_file(info.m_name, desktop_file);
                        if (key_file != nullptr) {
                            set_info(key_file, desktop_file, info);
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (result_list) {
                g_free(result_list);
            }
        }

        // cache
        cache[info.m_name] = info;

        if (key_file != nullptr) {
            g_key_file_free(key_file);
        }

        return true;
    }

}  // namespace desktopfile_util

DL_NS_END
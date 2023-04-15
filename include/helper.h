/**
 * Copyright (c) 2019-2022 WineGUI
 *
 * \file    helper.h
 * \brief   Helper class for Bottle Manager and CLI
 * \author  Melroy van den Berg <webmaster1989@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <glibmm/dispatcher.h>
#include <string>
#include <vector>

#include "bottle_types.h"
#include "dll_override_types.h"

using std::endl;
using std::string;

/**
 * \class Helper
 * \brief Provide some helper methods for Bottle Manager and CLI
 */
class Helper
{
public:
  // Signals
  Glib::Dispatcher failure_on_exec; /*!< Dispatch signal (thus in main thread) when exit code was non-zero */

  // Singleton
  static Helper& get_instance();

  static std::vector<string> get_bottles_paths(const string& dir_path, bool display_default_wine_machine);
  static string run_program(const string& prefix_path, int debug_log_level, const string& program, bool give_error = true, bool stderr_output = true);
  static string run_program_under_wine(
      bool wine_64_bit, const string& prefix_path, int debug_log_level, const string& program, bool give_error = true, bool stderr_output = true);
  static void write_to_log_file(const string& logging_bottle_prefix, const string& logging);
  static string get_log_file_path(const string& logging_bottle_prefix);
  static void wait_until_wineserver_is_terminated(const string& prefix_path);
  static int determine_wine_executable();
  static string get_wine_executable_location(bool bit64);
  static string get_winetricks_location();
  static string get_wine_version(bool wine_64_bit);
  static string open_file_from_uri(const string& uri);
  static void create_wine_bottle(bool wine_64_bit, const string& prefix_path, BottleTypes::Bit bit, const bool disable_gecko_mono);
  static void remove_wine_bottle(const string& prefix_path);
  static void rename_wine_bottle_folder(const string& current_prefix_path, const string& new_prefix_path);
  static string get_folder_name(const string& prefix_path);
  static BottleTypes::Windows get_windows_version(const string& prefix_path);
  static BottleTypes::Bit get_windows_bitness(const string& prefix_path);
  static BottleTypes::AudioDriver get_audio_driver(const string& prefix_path);
  static string get_virtual_desktop(const string& prefix_path);
  static string get_last_wine_updated(const string& prefix_path);
  static bool get_bottle_status(const string& prefix_path);
  static string get_program_icon_path(const string& shortcut_path);
  static string get_c_letter_drive(const string& prefix_path);
  static bool dir_exists(const string& dir_path);
  static bool create_dir(const string& dir_path);
  static bool file_exists(const string& filer_path);
  static void install_or_update_winetricks();
  static void self_update_winetricks();
  static void set_windows_version(const string& prefix_path, BottleTypes::Windows windows);
  static void set_virtual_desktop(const string& prefix_path, string resolution);
  static void disable_virtual_desktop(const string& prefix_path);
  static void set_audio_driver(const string& prefix_path, BottleTypes::AudioDriver audio_driver);
  static std::vector<string> get_menu_items(const string& prefix_path);
  static string log_level_to_winedebug_string(int log_level);
  static string get_wine_guid(bool wine_64_bit, const string& prefix_path, const string& application_name);
  static bool get_dll_override(const string& prefix_path, const string& dll_name, DLLOverride::LoadOrder load_order = DLLOverride::LoadOrder::Native);
  static string get_uninstaller(const string& prefix_path, const string& uninstallerKey);
  static string get_font_filename(const string& prefix_path, BottleTypes::Bit bit, const string& fontName);
  static string get_image_location(const string& filename);
  static bool is_default_wine_bottle(const string& prefix_path);
  static string encode_text(const std::string& string);

private:
  Helper();
  ~Helper();
  Helper(const Helper&) = delete;
  Helper& operator=(const Helper&) = delete;

  static string exec(const char* cmd);
  static string exec_error_message(const char* cmd);
  static int close_exec_stream(std::FILE* file);
  static void write_file(const string& filename, const string& contents);
  static string read_file(const string& filename);
  static string get_winetricks_version();
  static string get_reg_value(const string& filename, const string& key_name, const string& value_name);
  static std::vector<string> get_reg_keys(const string& file_path, const string& key_name);
  static std::vector<string> get_reg_keys_data(const string& file_path, const string& key_name);
  static std::vector<string> get_reg_keys_data_filter(const string& file_path, const string& key_name, const string& key_value_filter = "");
  static std::vector<string> get_reg_keys_data_filter_ignore(const string& file_path,
                                                             const string& key_name,
                                                             const string& key_value_filter = "",
                                                             const string& key_name_ignore_filter = "");
  static string get_reg_meta_data(const string& filename, const string& meta_value_name);
  static string get_bottle_dir_from_prefix(const string& prefix_path);
  static std::vector<string> read_file_lines(const string& file_path);
  static std::vector<string> split(const string& s, const char delimiter);
  static bool case_insensitive_compare(const std::string& a, const std::string& b);
  static string unescape_reg_key_data(const string& src);
};

/**
 * Copyright (c) 2019-2025 WineGUI
 *
 * \file    main.cc
 * \brief   Main, where it all begins
 * \author  Melroy van den Berg <melroy@melroy.org>
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
#include "about_dialog.h"
#include "application.h"

#include <iostream>

/**
 * \brief Main function, setup and starting the app main loop
 * \return Status code
 */
int main(int argc, char* argv[])
{
  if (argc > 1)
  {
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      if (arg == "--version")
      {
        // Retrieve version and print it
        std::string version = AboutDialog::get_version();
        std::cout << "WineGUI " << version << std::endl;
        return 0;
      }
    }
    std::cerr << "Error: Parameter not understood (only --version is an accepted parameter)!" << std::endl;
    return 1;
  }
  else
  {
    // Start app
    auto application = Application::create();
    const int status = application->run(argc, argv);
    return status;
  }
}

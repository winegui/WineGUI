/**
 * Copyright (c) 2019-2023 WineGUI
 *
 * \file    dll_override_types.h
 * \brief   DLL override enum definitions (native, builtin, etc.)
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

#include <string>

/**
 * \class DLLOverride
 * \brief DLL override enum definition
 */
namespace DLLOverride
{
  /**
   * \enum LoadOrder
   * \brief List load orders
   */
  enum class LoadOrder
  {
    Builtin = 0,
    Native,
    BuiltinNative,
    NativeBuiltin,
    Disabled,
  };

  inline static std::string to_string(LoadOrder order)
  {
    switch (order)
    {
    case LoadOrder::Builtin:
      return "builtin";
    case LoadOrder::Native:
      return "native";
    case LoadOrder::BuiltinNative:
      return "builtin,native";
    case LoadOrder::NativeBuiltin:
      return "native,builtin";
    case LoadOrder::Disabled:
      return "";
    }
    return "";
  }
};

#pragma once

// CMake Generated File (see 'version.h.in')
// ----------------------------------------------------------------------------------------
// This file is generated by CMake. Changes to this file will be overwritten
// To update the contents of this file edit ''./include/version.h.in'.
// ----------------------------------------------------------------------------------------

#include <format>
#include <iostream>
#include <string_view>

namespace slx
{
   struct product_version_info_t
   {
      static constexpr std::string_view project_name = "@MY_PROJECT_NAME@";
      static constexpr std::string_view project_ver = "@MY_PROJECT_VERSION@";
      static constexpr std::string_view project_description = "@MY_PROJECT_DESCRIPTION@";
      static constexpr std::string_view project_homepage_url = "@MY_PROJECT_HOMEPAGE_URL@";
      static constexpr std::string_view company_url = "@MY_PROJECT_COMPANY_URL@";
      static constexpr std::string_view project_copyright = "@MY_COPYRIGHT@";
   };

   inline constexpr std::string_view product_version() { return product_version_info_t::project_ver; }
   inline constexpr std::string_view product_name() { return product_version_info_t::project_name; }

   inline auto product_about()
   {
      using T = product_version_info_t;
      static const auto about = std::format(R"(About: 
        {}
        {}
        {}
        {}
        {}
        {})",
                                            T::project_name, T::project_ver, T::project_description,
                                            T::project_homepage_url, T::company_url, T::project_copyright);
      return about;
   }
}

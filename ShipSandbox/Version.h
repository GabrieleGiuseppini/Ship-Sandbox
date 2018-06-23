/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <string>

#define VERSION "2.2.0"

enum class VersionFormat
{
	Short,
	Long,
    LongWithDate
};

inline std::string GetVersionInfo(VersionFormat versionFormat)
{
	switch (versionFormat)
	{
		case VersionFormat::Short:
		{
			return std::string(VERSION);
		}

        case VersionFormat::Long:
        {
            return std::string("Ship Sandbox v" VERSION);
        }

        case VersionFormat::LongWithDate:
        {
            return std::string("Ship Sandbox v" VERSION " (" __DATE__ ")");
        }

		default:
		{
			assert(false);
			return "";
		}
	}
}

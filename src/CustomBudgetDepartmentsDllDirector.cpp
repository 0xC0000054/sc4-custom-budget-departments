////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-custom-budget-departments, a DLL Plugin for
// SimCity 4 that allows new budget departments to be added to the game
// using a building's exemplar.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////

#include "version.h"
#include "CustomBudgetDepartmentManager.h"
#include "DebugUtil.h"
#include "Logger.h"
#include "cIGZApp.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cRZBaseString.h"
#include "cRZCOMDllDirector.h"

#include <array>

#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

// This must be unique for every plugin. Generate a 32-bit random number and use it.
// DO NOT REUSE DIRECTOR IDS EVER.
static constexpr uint32_t kCustomBudgetDepartmentsDirectorID = 0x810A913B;

using namespace std::string_view_literals;

static constexpr std::string_view PluginLogFileName = "SC4CustomBudgetDepartments.log"sv;

namespace
{
	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}
}

class CustomBudgetDepartmentsDllDirector final : public cRZCOMDllDirector
{
public:
	CustomBudgetDepartmentsDllDirector()
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error, false);
		logger.WriteLogFileHeader("SC4CustomBudgetDepartment v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kCustomBudgetDepartmentsDirectorID;
	}

private:
	bool OnStart(cIGZCOM* pCOM)
	{
		cIGZFrameWork* const pFramework = pCOM->FrameWork();

		if (pFramework->GetState() < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}

		return true;
	}

	bool PostAppInit()
	{
		customBudgetDepartmentManager.Init();

		return true;
	}

	bool PreAppShutdown()
	{
		customBudgetDepartmentManager.Shutdown();

		return true;
	}

	CustomBudgetDepartmentManager customBudgetDepartmentManager;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static CustomBudgetDepartmentsDllDirector sDirector;
	return &sDirector;
}
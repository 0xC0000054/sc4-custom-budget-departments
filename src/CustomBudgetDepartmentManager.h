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

#pragma once
#include "cIGZMessageTarget2.h"
#include "StringResourceKey.h"
#include <vector>

class cIGZMessage2Standard;
class cISC4BudgetSimulator;
class cISC4BuildingOccupant;
class cISC4City;
class cISC4DepartmentBudget;
class cISC4LineItem;
class cISCPropertyHolder;

class CustomBudgetDepartmentManager final : private cIGZMessageTarget2
{
public:
	CustomBudgetDepartmentManager();

	bool Init();
	bool Shutdown();

private:
	enum class CustomBudgetDepartmentItemType : uint32_t
	{
		Expense,
		Income
	};

	struct CustomBudgetDepartmentInfo
	{
		CustomBudgetDepartmentItemType type;
		uint32_t department;
		uint32_t lineNumber;
		uint32_t budgetGroup;
		int64_t cost;
		StringResourceKey departmentNameKey;

		CustomBudgetDepartmentInfo()
			: type(CustomBudgetDepartmentItemType::Expense),
			  department(0),
			  lineNumber(0),
			  budgetGroup(0),
			  cost(0),
			  departmentNameKey()
		{
		}

		CustomBudgetDepartmentInfo(
			CustomBudgetDepartmentItemType type,
			uint32_t department,
			uint32_t line,
			uint32_t budgetGroup,
			int64_t cost,
			StringResourceKey departmentNameKey)
			: type(type),
			  department(department),
			  lineNumber(line),
			  budgetGroup(budgetGroup),
			  cost(cost),
			  departmentNameKey(departmentNameKey)
		{
		}
	};

	bool QueryInterface(uint32_t riid, void** ppVoid) override;
	uint32_t AddRef() override;
	uint32_t Release() override;
	bool DoMessage(cIGZMessage2* pMsg) override;

	void PostCityInit(cISC4City* pCity);
	void PostCityShutdown();
	void InsertOccupant(cIGZMessage2Standard* pStandardMsg);
	void RemoveOccupant(cIGZMessage2Standard* pStandardMsg);

	cISC4DepartmentBudget* GetOrCreateBudgetDepartment(const CustomBudgetDepartmentInfo& info);
	cISC4LineItem* GetOrCreateLineItem(
		cISC4BuildingOccupant* pBuildingOccupant,
		cISC4DepartmentBudget* pDepartment,
		const CustomBudgetDepartmentInfo& info);

	std::vector<CustomBudgetDepartmentInfo> LoadCustomBudgetDepartmentInfo(const cISCPropertyHolder* pPropertyHolder);

	uint32_t refCount;
	cISC4BudgetSimulator* pBudgetSim;
};


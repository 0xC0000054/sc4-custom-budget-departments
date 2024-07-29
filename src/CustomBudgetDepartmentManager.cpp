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

#include "CustomBudgetDepartmentManager.h"
#include "Logger.h"
#include "cGZPersistResourceKey.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cIGZVariant.h"
#include "cISC4BudgetSimulator.h"
#include "cISC4BuildingOccupant.h"
#include "cISC4City.h"
#include "cISC4DepartmentBudget.h"
#include "cISC4LineItem.h"
#include "cISC4Occupant.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cRZAutoRefCount.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include "SCPropertyUtil.h"
#include "StringResourceKey.h"
#include <array>

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePostCityShutdown = 0x26D31EC3;
static constexpr uint32_t kSC4MessageInsertOccupant = 0x99EF1142;
static constexpr uint32_t kSC4MessageRemoveOccupant = 0x99EF1143;

static const std::array<uint32_t, 4> MessageIds =
{
	kSC4MessagePostCityInit,
	kSC4MessagePostCityShutdown,
	kSC4MessageInsertOccupant,
	kSC4MessageRemoveOccupant,
};

static constexpr uint32_t kBudgetGroupBusinessDeals = 0xA5A72D1;
static constexpr uint32_t kBudgetGroupCityBeautification = 0x6A357B96;
static constexpr uint32_t kBudgetGroupGovernmentBuildings = 0xEA597195;
static constexpr uint32_t kBudgetGroupHealthAndEducation = 0x6A357B7F;
static constexpr uint32_t kBudgetGroupPublicSafety = 0x4A357B40;
static constexpr uint32_t kBudgetGroupTransportation = 0xAA369059;
static constexpr uint32_t kBudgetGroupUtilities = 0x4A357EAF;

static constexpr uint32_t kOccupantType_Building = 0x278128A0;

static constexpr uint32_t kBudgetItemDepartmentProperty = 0xEA54D283;
static constexpr uint32_t kBudgetItemLineProperty = 0xEA54D284;
static constexpr uint32_t kBudgetItemPurpose = 0xEA54D285;
static constexpr uint32_t kBudgetItemCostProperty = 0xEA54D286;

static constexpr uint32_t kCustomBudgetDepartmentBudgetGroupProperty = 0x90222B81;
static constexpr uint32_t kCustomBudgetDepartmentNameKeyProperty = 0x4252085F;

static constexpr uint32_t kCustomBudgetDepartmentExpensePurposeId = 0x87BD3990;
static constexpr uint32_t kCustomBudgetDepartmentIncomePurposeId = 0x46261226;

namespace
{
	bool IsValidBudgetGroup(uint32_t budgetGroup)
	{
		switch (budgetGroup)
		{
		case kBudgetGroupBusinessDeals:
		case kBudgetGroupCityBeautification:
		case kBudgetGroupGovernmentBuildings:
		case kBudgetGroupHealthAndEducation:
		case kBudgetGroupPublicSafety:
		case kBudgetGroupTransportation:
		case kBudgetGroupUtilities:
			return true;
		default:
			return false;
		}
	}

	bool GetPropertyValue(const cISCPropertyHolder* pPropertyHolder, uint32_t id, std::vector<uint32_t>& values)
	{
		if (pPropertyHolder)
		{
			const cISCProperty* property = pPropertyHolder->GetProperty(id);

			if (property)
			{
				const cIGZVariant* pVariant = property->GetPropertyValue();

				if (pVariant)
				{
					if (pVariant->GetType() == cIGZVariant::Type::Uint32Array)
					{
						const uint32_t count = pVariant->GetCount();
						values.reserve(count);

						const uint32_t* pData = pVariant->RefUint32();

						for (uint32_t i = 0; i < count; i++)
						{
							values.push_back(pData[i]);
						}

						return true;
					}
				}
			}
		}

		return false;
	}

	bool GetPropertyValue(const cISCPropertyHolder* pPropertyHolder, uint32_t id, std::vector<int64_t>& values)
	{
		if (pPropertyHolder)
		{
			const cISCProperty* property = pPropertyHolder->GetProperty(id);

			if (property)
			{
				const cIGZVariant* pVariant = property->GetPropertyValue();

				if (pVariant)
				{
					if (pVariant->GetType() == cIGZVariant::Type::Sint64Array)
					{
						const uint32_t count = pVariant->GetCount();
						values.reserve(count);

						const int64_t* pData = pVariant->RefSint64();

						for (uint32_t i = 0; i < count; i++)
						{
							values.push_back(pData[i]);
						}

						return true;
					}
				}
			}
		}

		return false;
	}

	bool GetPropertyValue(
		const cISCPropertyHolder* pPropertyHolder,
		uint32_t id,
		std::vector<StringResourceKey>& values)
	{
		if (pPropertyHolder)
		{
			const cISCProperty* property = pPropertyHolder->GetProperty(id);

			if (property)
			{
				const cIGZVariant* pVariant = property->GetPropertyValue();

				if (pVariant)
				{
					if (pVariant->GetType() == cIGZVariant::Type::Uint32Array)
					{
						const uint32_t count = pVariant->GetCount();

						// The values are an array of 2 items each.
						if (count >= 2 && (count % 2) == 0)
						{
							const uint32_t* pData = pVariant->RefUint32();
							values.reserve(count / 2);

							for (uint32_t i = 0; i < count; i += 2)
							{
								values.push_back(StringResourceKey(pData[i], pData[i + 1]));
							}

							return true;
						}
					}
				}
			}
		}

		return false;
	}

	bool PurposeIdsAreCustomBudgetDepartmentItems(const std::vector<uint32_t>& purposeIds)
	{
		if (purposeIds.empty())
		{
			return false;
		}

		for (const uint32_t& item : purposeIds)
		{
			if (item != kCustomBudgetDepartmentExpensePurposeId
				&& item != kCustomBudgetDepartmentIncomePurposeId)
			{
				return false;
			}
		}

		return true;
	}
}

CustomBudgetDepartmentManager::CustomBudgetDepartmentManager()
	: refCount(0),
	  pBudgetSim(nullptr)
{
}

bool CustomBudgetDepartmentManager::Init()
{
	cIGZMessageServer2Ptr pMsgServ;

	if (pMsgServ)
	{
		for (uint32_t messageID : MessageIds)
		{
			pMsgServ->AddNotification(this, messageID);
		}
	}

	return true;
}

bool CustomBudgetDepartmentManager::Shutdown()
{
	cIGZMessageServer2Ptr pMsgServ;

	if (pMsgServ)
	{
		for (uint32_t messageID : MessageIds)
		{
			pMsgServ->RemoveNotification(this, messageID);
		}
	}

	return true;
}

bool CustomBudgetDepartmentManager::QueryInterface(uint32_t riid, void** ppVoid)
{
	if (riid == GZCLSID::kcIGZMessageTarget2)
	{
		*ppVoid = static_cast<cIGZMessageTarget2*>(this);
		AddRef();

		return true;
	}
	else if (riid == GZIID_cIGZUnknown)
	{
		*ppVoid = static_cast<cIGZUnknown*>(this);
		AddRef();

		return true;
	}

	return false;
}

uint32_t CustomBudgetDepartmentManager::AddRef()
{
	return ++refCount;
}

uint32_t CustomBudgetDepartmentManager::Release()
{
	if (refCount > 0)
	{
		--refCount;
	}

	return refCount;
}

bool CustomBudgetDepartmentManager::DoMessage(cIGZMessage2* pMsg)
{
	cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMsg);

	switch (pStandardMsg->GetType())
	{
	case kSC4MessagePostCityInit:
		PostCityInit(static_cast<cISC4City*>(pStandardMsg->GetVoid1()));
		break;
	case kSC4MessagePostCityShutdown:
		PostCityShutdown();
		break;
	case kSC4MessageInsertOccupant:
		InsertOccupant(pStandardMsg);
		break;
	case kSC4MessageRemoveOccupant:
		RemoveOccupant(pStandardMsg);
		break;
	}

	return true;
}

void CustomBudgetDepartmentManager::PostCityInit(cISC4City* pCity)
{
	pBudgetSim = nullptr;

	if (pCity)
	{
		pBudgetSim = pCity->GetBudgetSimulator();
	}
}

void CustomBudgetDepartmentManager::PostCityShutdown()
{
	pBudgetSim = nullptr;
}

void CustomBudgetDepartmentManager::InsertOccupant(cIGZMessage2Standard* pStandardMsg)
{
	cISC4Occupant* const pOccupant = static_cast<cISC4Occupant*>(pStandardMsg->GetVoid1());

	if (pOccupant->GetType() == kOccupantType_Building && pBudgetSim)
	{
		cISCPropertyHolder* const pPropertyHolder = pOccupant->AsPropertyHolder();

		const std::vector<CustomBudgetDepartmentInfo> items = LoadCustomBudgetDepartmentInfo(pPropertyHolder);

		if (!items.empty())
		{
			cRZAutoRefCount<cISC4BuildingOccupant> buildingOccupant;

			if (pOccupant->QueryInterface(GZIID_cISC4BuildingOccupant, buildingOccupant.AsPPVoid()))
			{
				for (const CustomBudgetDepartmentInfo& item : items)
				{
					if (item.type == CustomBudgetDepartmentItemType::Expense)
					{
						cISC4DepartmentBudget* const pDepartment = GetOrCreateBudgetDepartment(item);

						if (pDepartment)
						{
							cISC4LineItem* const pLineItem = GetOrCreateLineItem(
								buildingOccupant,
								pDepartment,
								item);

							if (pLineItem)
							{
								// We use the secondary info field to track the number of buildings
								// of each type in the city.

								int64_t buildingCount = pLineItem->GetSecondaryInfoField();
								buildingCount++;
								pLineItem->SetSecondaryInfoField(buildingCount);

								// Add the cost of the new building tho the current expenses.
								pLineItem->AddToFullExpenses(item.cost);

								if (buildingCount > 1)
								{
									// If there are two or more buildings of the same type we tell the game to display the building count
									// in the UI.
									// It will be displayed using the following format: <Building name> (<Building count>) <Total expense>
									pLineItem->SetDisplayFlag(cISC4LineItem::DisplayFlag::ShowSecondaryInfoField, true);
								}
							}
						}
					}
					else if (item.type == CustomBudgetDepartmentItemType::Income)
					{
						cISC4DepartmentBudget* const pDepartment = GetOrCreateBudgetDepartment(item);

						if (pDepartment)
						{
							cISC4LineItem* const pLineItem = GetOrCreateLineItem(
								buildingOccupant,
								pDepartment,
								item);

							if (pLineItem)
							{
								// We use the secondary info field to track the number of buildings
								// of each type in the city.

								int64_t buildingCount = pLineItem->GetSecondaryInfoField();
								buildingCount++;
								pLineItem->SetSecondaryInfoField(buildingCount);

								// Add the cost of the new building tho the current income.
								pLineItem->AddToIncome(item.cost);

								if (buildingCount > 1)
								{
									// If there are two or more buildings of the same type we tell the game to display the building count
									// in the UI.
									// It will be displayed using the following format: <Building name> (<Building count>) <Total expense>
									pLineItem->SetDisplayFlag(cISC4LineItem::DisplayFlag::ShowSecondaryInfoField, true);
								}
							}
						}
					}
				}
			}
		}
	}
}

void CustomBudgetDepartmentManager::RemoveOccupant(cIGZMessage2Standard* pStandardMsg)
{
	cISC4Occupant* const pOccupant = static_cast<cISC4Occupant*>(pStandardMsg->GetVoid1());

	if (pOccupant->GetType() == kOccupantType_Building && pBudgetSim)
	{
		cISCPropertyHolder* const pPropertyHolder = pOccupant->AsPropertyHolder();

		const std::vector<CustomBudgetDepartmentInfo> items = LoadCustomBudgetDepartmentInfo(pPropertyHolder);

		if (!items.empty())
		{
			for (const CustomBudgetDepartmentInfo & item : items)
			{
				if (item.type == CustomBudgetDepartmentItemType::Expense)
				{
					cISC4DepartmentBudget* const pDepartment = pBudgetSim->GetDepartmentBudget(item.department);

					if (pDepartment)
					{
						cISC4LineItem* const pLineItem = pDepartment->GetLineItem(item.lineNumber);

						if (pLineItem)
						{
							// We use the secondary info field to track the number of buildings
							// of each type in the city.

							int64_t buildingCount = pLineItem->GetSecondaryInfoField();

							// Subtract the cost of the building from the current expenses.
							pLineItem->AddToFullExpenses(-item.cost);

							if (buildingCount > 1)
							{
								buildingCount--;
								pLineItem->SetSecondaryInfoField(buildingCount);

								// If there are two or more buildings of the same type we tell the game to display the building count
								// in the UI.
								// It will be displayed using the following format: <Building name> (<Building count>) <Total expense>
								if (buildingCount == 1)
								{
									pLineItem->SetDisplayFlag(cISC4LineItem::DisplayFlag::ShowSecondaryInfoField, false);
								}
							}
							else
							{
								pDepartment->RemoveLineItem(item.lineNumber);
							}
						}
					}
				}
				else if (item.type == CustomBudgetDepartmentItemType::Income)
				{
					cISC4DepartmentBudget* const pDepartment = pBudgetSim->GetDepartmentBudget(item.department);

					if (pDepartment)
					{
						cISC4LineItem* const pLineItem = pDepartment->GetLineItem(item.lineNumber);

						if (pLineItem)
						{
							// We use the secondary info field to track the number of buildings
							// of each type in the city.

							int64_t buildingCount = pLineItem->GetSecondaryInfoField();

							// Subtract the cost of the new building tho the current income.
							pLineItem->AddToIncome(-item.cost);

							if (buildingCount > 1)
							{
								buildingCount--;
								pLineItem->SetSecondaryInfoField(buildingCount);

								// If there are two or more buildings of the same type we tell the game to display the building count
								// in the UI.
								// It will be displayed using the following format: <Building name> (<Building count>) <Total expense>
								if (buildingCount == 1)
								{
									pLineItem->SetDisplayFlag(cISC4LineItem::DisplayFlag::ShowSecondaryInfoField, false);
								}
							}
							else
							{
								pDepartment->RemoveLineItem(item.lineNumber);
							}
						}
					}
				}
			}
		}
	}
}

cISC4DepartmentBudget* CustomBudgetDepartmentManager::GetOrCreateBudgetDepartment(const CustomBudgetDepartmentInfo& info)
{
	Logger& logger = Logger::GetInstance();

	cISC4DepartmentBudget* pDepartmentBudget = nullptr;

	if (pBudgetSim)
	{
		pDepartmentBudget = pBudgetSim->GetDepartmentBudget(info.department);

		if (!pDepartmentBudget)
		{
			if (IsValidBudgetGroup(info.budgetGroup))
			{
				pDepartmentBudget = pBudgetSim->CreateDepartmentBudget(info.department, info.budgetGroup);

				if (pDepartmentBudget)
				{
					pDepartmentBudget->SetFixedFunding(true);

					const StringResourceKey& nameKey = info.departmentNameKey;

					pDepartmentBudget->SetDepartmentName(nameKey.groupID, nameKey.instanceID);
				}
				else
				{
					logger.WriteLineFormatted(LogLevel::Error, "Failed to create budget department: 0x%08x", info.department);
				}
			}
			else
			{
				logger.WriteLineFormatted(LogLevel::Error, "Invalid budget group: 0x%08x", info.budgetGroup);
			}
		}
	}

	return pDepartmentBudget;
}

cISC4LineItem* CustomBudgetDepartmentManager::GetOrCreateLineItem(
	cISC4BuildingOccupant* pBuildingOccupant,
	cISC4DepartmentBudget* pDepartment,
	const CustomBudgetDepartmentInfo& info)
{
	cISC4LineItem* pLineItem = nullptr;

	if (pDepartment)
	{
		pLineItem = pDepartment->GetLineItem(info.lineNumber);

		if (!pLineItem && pBuildingOccupant)
		{
			pLineItem = pDepartment->CreateLineItem(info.lineNumber, false);

			if (pLineItem)
			{
				uint32_t buildingType = pBuildingOccupant->GetBuildingType();

				pLineItem->SetName(0, buildingType);

				// Expense is the default type.
				if (info.type == CustomBudgetDepartmentItemType::Income)
				{
					pLineItem->SetType(cISC4LineItem::Type::Income);
				}
			}
			else
			{
				Logger::GetInstance().WriteLineFormatted(
					LogLevel::Error,
					"Failed to create line item 0x%08x in department: 0x%08x",
					info.lineNumber,
					info.department);
			}
		}
	}

	return pLineItem;
}

std::vector<CustomBudgetDepartmentManager::CustomBudgetDepartmentInfo> CustomBudgetDepartmentManager::LoadCustomBudgetDepartmentInfo(
	const cISCPropertyHolder* pPropertyHolder)
{
	std::vector<CustomBudgetDepartmentInfo> items;

	std::vector<uint32_t> purposeIds;

	if (GetPropertyValue(pPropertyHolder, kBudgetItemPurpose, purposeIds))
	{
		if (PurposeIdsAreCustomBudgetDepartmentItems(purposeIds))
		{
			const size_t count = purposeIds.size();

			std::vector<uint32_t> departmentIds;
			std::vector<uint32_t> lines;
			std::vector<int64_t> costs;
			std::vector<uint32_t> budgetGroups;
			std::vector<StringResourceKey> departmentNameKeys;

			if (GetPropertyValue(pPropertyHolder, kBudgetItemDepartmentProperty, departmentIds)
				&& GetPropertyValue(pPropertyHolder, kBudgetItemLineProperty, lines)
				&& GetPropertyValue(pPropertyHolder, kBudgetItemCostProperty, costs)
				&& GetPropertyValue(pPropertyHolder, kCustomBudgetDepartmentBudgetGroupProperty, budgetGroups)
				&& GetPropertyValue(pPropertyHolder, kCustomBudgetDepartmentNameKeyProperty, departmentNameKeys))
			{
				if (departmentIds.size() == count
					&& lines.size() == count
					&& costs.size() == count
					&& budgetGroups.size() == count
					&& departmentNameKeys.size() == count)
				{
					for (size_t i = 0; i < count; i++)
					{
						const uint32_t purpose = purposeIds[i];

						if (purpose == kCustomBudgetDepartmentExpensePurposeId)
						{
							items.push_back(CustomBudgetDepartmentInfo(
								CustomBudgetDepartmentItemType::Expense,
								departmentIds[i],
								lines[i],
								budgetGroups[i],
								costs[i],
								departmentNameKeys[i]));

						}
						else if (purpose == kCustomBudgetDepartmentIncomePurposeId)
						{
							items.push_back(CustomBudgetDepartmentInfo(
								CustomBudgetDepartmentItemType::Income,
								departmentIds[i],
								lines[i],
								budgetGroups[i],
								costs[i],
								departmentNameKeys[i]));
						}
					}
				}
			}
		}
	}

	return items;
}
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
#include "cIGZPersistDBSegment.h"
#include "cIGZVariant.h"
#include "cISC4BudgetSimulator.h"
#include "cISC4BuildingOccupant.h"
#include "cISC4City.h"
#include "cISC4DBSegment.h"
#include "cISC4DBSegmentIStream.h"
#include "cISC4DBSegmentOStream.h"
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
#include "TransactionAlgorithmFactory.h"
#include "TransactionAlgorithmStaticPointers.h"
#include <array>

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePostCityShutdown = 0x26D31EC3;
static constexpr uint32_t kSC4MessageInsertOccupant = 0x99EF1142;
static constexpr uint32_t kSC4MessageRemoveOccupant = 0x99EF1143;
static constexpr uint32_t kSC4MessageLoad = 0x26C63341;
static constexpr uint32_t kSC4MessageSave = 0x26C63344;
static constexpr uint32_t kSC4MessageSimNewMonth = 0x66956816;

static const std::array<uint32_t, 7> MessageIds =
{
	kSC4MessagePostCityInit,
	kSC4MessagePostCityShutdown,
	kSC4MessageInsertOccupant,
	kSC4MessageRemoveOccupant,
	kSC4MessageLoad,
	kSC4MessageSave,
	kSC4MessageSimNewMonth,
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
static constexpr uint32_t kCustomBudgetLineItemAlgorithm = 0x9EE1240F;
// See TransactionAlgorithmFactory.cpp for the custom budget line item algorithm
// tuning property ids, each custom budget line item algorithm that takes tuning
// parameters has expense and income tuning properties defined for that purpose.

static constexpr uint32_t kCustomBudgetDepartmentExpensePurposeId = 0x87BD3990;
static constexpr uint32_t kCustomBudgetDepartmentIncomePurposeId = 0x46261226;

static constexpr uint32_t CustomBudgetDepartmentManagerTypeId = 0xFE005706;
static constexpr uint32_t CustomBudgetDepartmentManagerGroupId = 0xFE005707;
static constexpr uint32_t CustomBudgetDepartmentManagerInstanceId = 0;

IPopulationProvider* spPopulationProvider;

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
		std::unordered_map<uint32_t, uint32_t>& values)
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
								values.try_emplace(pData[i], pData[i + 1]);
							}

							return true;
						}
					}
				}
			}
		}

		return false;
	}

	bool GetBudgetDepartmentNameProperty(
		const cISCPropertyHolder* pPropertyHolder,
		uint32_t id,
		std::unordered_map<uint32_t, StringResourceKey>& values)
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

						// The values are an array of 3 items each.
						// The format is: <department id> <department name key group id> <department name key instance id>
						if (count >= 3 && (count % 3) == 0)
						{
							const uint32_t* pData = pVariant->RefUint32();
							values.reserve(count / 3);

							for (uint32_t i = 0; i < count; i += 3)
							{
								values.try_emplace(pData[i], StringResourceKey(pData[i + 1], pData[i + 2]));
							}

							return true;
						}
					}
				}
			}
		}

		return false;
	}

	bool CreateLineItemTransactionCore(
		const cISCPropertyHolder* pPropertyHolder,
		TransactionAlgorithmType type,
		uint32_t lineNumber,
		int64_t cost,
		bool isIncome,
		std::unordered_map<uint32_t, std::unique_ptr<LineItemTransaction>>& destination)
	{
		bool result = false;

		try
		{
			auto pair = destination.emplace(lineNumber, std::make_unique<LineItemTransaction>(pPropertyHolder, type, cost, lineNumber, isIncome));
			result = pair.second;
		}
		catch (const CreateTransactionAlgorithmException& e)
		{
			Logger::GetInstance().WriteLine(LogLevel::Error, e.what());
			result = false;
		}

		return result;
	}

	bool CreateLineItemTransaction(
		const cISCPropertyHolder* pPropertyHolder,
		uint32_t lineNumber,
		int64_t cost,
		bool isIncome,
		std::unordered_map<uint32_t, std::unique_ptr<LineItemTransaction>>& destination)
	{
		bool result = false;

		std::unordered_map<uint32_t, uint32_t> lineItemTransactionAlgorithms;

		if (GetPropertyValue(pPropertyHolder, kCustomBudgetLineItemAlgorithm, lineItemTransactionAlgorithms))
		{
			const auto& item = lineItemTransactionAlgorithms.find(lineNumber);

			if (item != lineItemTransactionAlgorithms.end())
			{
				result = CreateLineItemTransactionCore(
					pPropertyHolder,
					static_cast<TransactionAlgorithmType>(item->second),
					lineNumber,
					cost,
					isIncome,
					destination);
			}
			else
			{
				result = CreateLineItemTransactionCore(
					pPropertyHolder,
					TransactionAlgorithmType::Fixed,
					lineNumber,
					cost,
					isIncome,
					destination);
			}
		}
		else
		{
			result = CreateLineItemTransactionCore(
				pPropertyHolder,
				TransactionAlgorithmType::Fixed,
				lineNumber,
				cost,
				isIncome,
				destination);
		}

		return result;
	}

	bool ContainsCustomBudgetDepartmentPurposeId(const std::vector<uint32_t>& purposeIds)
	{
		for (const uint32_t& item : purposeIds)
		{
			if (item == kCustomBudgetDepartmentExpensePurposeId
				|| item == kCustomBudgetDepartmentIncomePurposeId)
			{
				return true;
			}
		}

		return false;
	}

	LineItemTransaction* GetLineItemTransactionPtr(
		std::unordered_map<uint32_t, std::unique_ptr<LineItemTransaction>>& collection,
		uint32_t lineNumber)
	{
		LineItemTransaction* result = nullptr;

		const auto& item = collection.find(lineNumber);

		if (item != collection.end())
		{
			result = item->second.get();
		}

		return result;
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

	spPopulationProvider = &populationProvider;

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
	case kSC4MessageLoad:
		Load(static_cast<cIGZPersistDBSegment*>(pStandardMsg->GetVoid1()));
		break;
	case kSC4MessageSave:
		Save(static_cast<cIGZPersistDBSegment*>(pStandardMsg->GetVoid1()));
		break;
	case kSC4MessageSimNewMonth:
		SimNewMonth();
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
		populationProvider.Init();
	}
}

void CustomBudgetDepartmentManager::PostCityShutdown()
{
	pBudgetSim = nullptr;
	populationProvider.Shutdown();
	customBudgetDepartments.clear();
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
					LineItemTransaction* pTransaction = GetOrCreateLineItemTransaction(pPropertyHolder, item);

					if (pTransaction)
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

								if (item.type == CustomBudgetDepartmentItemType::Expense)
								{
									// Add the cost of the new building tho the current expenses.
									pLineItem->SetFullExpenses(pTransaction->CalculateLineItemTotal(buildingCount));
								}
								else
								{
									// Add the cost of the new building tho the current income.
									pLineItem->SetIncome(pTransaction->CalculateLineItemTotal(buildingCount));
								}

								if (buildingCount > 1)
								{
									// If there are two or more buildings of the same type we tell the game to display the building count
									// in the UI.
									// It will be displayed using the following format: <Building name> (<Building count>) <Total expense>
									pLineItem->SetDisplayFlag(cISC4LineItem::DisplayFlag::ShowSecondaryInfoField, true);
								}
							}
							else
							{
								RemoveLineItemTransaction(item);
							}
						}
						else
						{
							RemoveLineItemTransaction(item);
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
			for (const CustomBudgetDepartmentInfo& item : items)
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

						LineItemTransaction* pTransaction = GetLineItemTransaction(item);

						if (item.type == CustomBudgetDepartmentItemType::Expense)
						{
							// Subtract the cost of the building from the current expenses.
							if (pTransaction)
							{
								pLineItem->SetFullExpenses(pTransaction->CalculateLineItemTotal(buildingCount - 1));
							}
							else
							{
								// Handle buildings that were in the city before the transaction system was introduced.
								pLineItem->AddToFullExpenses(-item.cost);
							}
						}
						else
						{
							// Subtract the cost of the building from the current income.
							if (pTransaction)
							{
								pLineItem->SetIncome(pTransaction->CalculateLineItemTotal(buildingCount - 1));
							}
							else
							{
								// Handle buildings that were in the city before the transaction system was introduced.
								pLineItem->AddToIncome(-item.cost);
							}
						}

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

							if (pTransaction)
							{
								RemoveLineItemTransaction(item);
							}
						}
					}
				}
			}
		}
	}
}

void CustomBudgetDepartmentManager::SimNewMonth()
{
	for (auto& department : customBudgetDepartments)
	{
		if (!department.second.empty())
		{
			cISC4DepartmentBudget* pDepartment = pBudgetSim->GetDepartmentBudget(department.first);

			if (pDepartment)
			{
				for (auto& lineItem : department.second)
				{
					auto& transaction = lineItem.second;

					// Fixed cost line items don't need to be updated as the cost is set
					// in the building's exemplar and never changes.
					if (transaction && !transaction->IsFixedCost())
					{
						cISC4LineItem* pLineItem = pDepartment->GetLineItem(lineItem.first);

						if (pLineItem)
						{
							int64_t buildingCount = pLineItem->GetSecondaryInfoField();
							const int64_t newTotal = transaction->CalculateLineItemTotal(buildingCount);

							if (transaction->IsIncome())
							{
								pLineItem->SetIncome(newTotal);
							}
							else
							{
								pLineItem->SetFullExpenses(newTotal);
							}
						}
					}
				}
			}
		}
	}
}

void CustomBudgetDepartmentManager::Load(cIGZPersistDBSegment* pSegment)
{
	if (pSegment)
	{
		cRZAutoRefCount<cISC4DBSegment> pSC4DBSegment;

		if (pSegment->QueryInterface(GZIID_cISC4DBSegment, pSC4DBSegment.AsPPVoid()))
		{
			cGZPersistResourceKey key(
				CustomBudgetDepartmentManagerTypeId,
				CustomBudgetDepartmentManagerGroupId,
				CustomBudgetDepartmentManagerInstanceId);

			cRZAutoRefCount<cISC4DBSegmentIStream> pStream;

			if (pSC4DBSegment->OpenIStream(key, pStream.AsPPObj()))
			{
				ReadFromDBSegment(*pStream);
			}
		}
	}
}

void CustomBudgetDepartmentManager::Save(cIGZPersistDBSegment* pSegment) const
{
	if (pSegment && !customBudgetDepartments.empty())
	{
		cRZAutoRefCount<cISC4DBSegment> pSC4DBSegment;

		if (pSegment->QueryInterface(GZIID_cISC4DBSegment, pSC4DBSegment.AsPPVoid()))
		{
			cGZPersistResourceKey key(
				CustomBudgetDepartmentManagerTypeId,
				CustomBudgetDepartmentManagerGroupId,
				CustomBudgetDepartmentManagerInstanceId);

			cRZAutoRefCount<cISC4DBSegmentOStream> pStream;

			if (pSC4DBSegment->OpenOStream(key, pStream.AsPPObj(), true))
			{
				WriteToDBSegment(*pStream);
			}
		}
	}
}

void CustomBudgetDepartmentManager::ReadFromDBSegment(cISC4DBSegmentIStream& stream)
{
	uint32_t version = 0;

	if (stream.GetUint32(version))
	{
		if (version == 1)
		{
			uint32_t departmentCount = 0;
			stream.GetUint32(departmentCount);

			customBudgetDepartments.clear();
			customBudgetDepartments.reserve(departmentCount);

			for (uint32_t i = 0; i < departmentCount; i++)
			{
				uint32_t departmentId = 0;
				stream.GetUint32(departmentId);

				uint32_t lineItemCount = 0;
				stream.GetUint32(lineItemCount);

				std::unordered_map<uint32_t, std::unique_ptr<LineItemTransaction>> lineItems;
				lineItems.reserve(lineItemCount);

				for (uint32_t i = 0; i < lineItemCount; i++)
				{
					uint32_t lineItemId = 0;
					stream.GetUint32(lineItemId);

					std::unique_ptr<LineItemTransaction> transaction = std::make_unique<LineItemTransaction>();
					transaction->Read(stream);

					lineItems.emplace(lineItemId, std::move(transaction));
				}

				customBudgetDepartments.emplace(departmentId, std::move(lineItems));
			}
		}
	}
}

void CustomBudgetDepartmentManager::WriteToDBSegment(cISC4DBSegmentOStream& stream) const
{
	if (stream.SetUint32(1))
	{
		stream.SetUint32(customBudgetDepartments.size());

		for (const auto& departments : customBudgetDepartments)
		{
			stream.SetUint32(departments.first); // department id

			const auto& lineItems = departments.second;

			stream.SetUint32(lineItems.size()); // line item count

			for (const auto& lineItem : lineItems)
			{
				stream.SetUint32(lineItem.first); // line item id
				lineItem.second->Write(stream);   // line item transaction
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

LineItemTransaction* CustomBudgetDepartmentManager::GetOrCreateLineItemTransaction(
	const cISCPropertyHolder* pPropertyHolder,
	const CustomBudgetDepartmentInfo& info)
{
	LineItemTransaction* result = nullptr;

	const auto& departmentLineItems = customBudgetDepartments.find(info.department);

	if (departmentLineItems != customBudgetDepartments.end())
	{
		result = GetLineItemTransactionPtr(departmentLineItems->second, info.lineNumber);

		if (!result)
		{
			if (CreateLineItemTransaction(
				pPropertyHolder,
				info.lineNumber,
				info.cost,
				info.type == CustomBudgetDepartmentItemType::Income,
				departmentLineItems->second))
			{
				result = GetLineItemTransactionPtr(departmentLineItems->second, info.lineNumber);
			}
		}
	}
	else
	{
		std::unordered_map<uint32_t, std::unique_ptr<LineItemTransaction>> lineItems;

		if (CreateLineItemTransaction(
			pPropertyHolder,
			info.lineNumber,
			info.cost,
			info.type == CustomBudgetDepartmentItemType::Income,
			lineItems))
		{
			const auto& pair = customBudgetDepartments.try_emplace(info.department, std::move(lineItems));

			result = GetLineItemTransactionPtr(pair.first->second, info.lineNumber);
		}
	}

	return result;
}

LineItemTransaction* CustomBudgetDepartmentManager::GetLineItemTransaction(const CustomBudgetDepartmentInfo& info)
{
	LineItemTransaction* result = nullptr;

	const auto& departmentLineItems = customBudgetDepartments.find(info.department);

	if (departmentLineItems != customBudgetDepartments.end())
	{
		result = GetLineItemTransactionPtr(departmentLineItems->second, info.lineNumber);
	}

	return result;
}

void CustomBudgetDepartmentManager::RemoveLineItemTransaction(const CustomBudgetDepartmentInfo& info)
{
	const auto& departmentLineItems = customBudgetDepartments.find(info.department);

	if (departmentLineItems != customBudgetDepartments.end())
	{
		auto& lineItems = departmentLineItems->second;

		if (lineItems.erase(info.lineNumber) == 1)
		{
			if (lineItems.size() == 0)
			{
				customBudgetDepartments.erase(departmentLineItems);
			}
		}
	}
}

std::vector<CustomBudgetDepartmentManager::CustomBudgetDepartmentInfo> CustomBudgetDepartmentManager::LoadCustomBudgetDepartmentInfo(
	const cISCPropertyHolder* pPropertyHolder)
{
	std::vector<CustomBudgetDepartmentInfo> items;

	std::vector<uint32_t> purposeIds;

	if (GetPropertyValue(pPropertyHolder, kBudgetItemPurpose, purposeIds))
	{
		if (ContainsCustomBudgetDepartmentPurposeId(purposeIds))
		{
			std::vector<uint32_t> departmentIds;
			std::vector<uint32_t> lines;
			std::vector<int64_t> costs;
			std::unordered_map<uint32_t, uint32_t> budgetGroups;
			std::unordered_map<uint32_t, StringResourceKey> departmentNameKeys;

			if (GetPropertyValue(pPropertyHolder, kBudgetItemDepartmentProperty, departmentIds)
				&& GetPropertyValue(pPropertyHolder, kBudgetItemLineProperty, lines)
				&& GetPropertyValue(pPropertyHolder, kBudgetItemCostProperty, costs)
				&& GetPropertyValue(pPropertyHolder, kCustomBudgetDepartmentBudgetGroupProperty, budgetGroups)
				&& GetBudgetDepartmentNameProperty(pPropertyHolder, kCustomBudgetDepartmentNameKeyProperty, departmentNameKeys))
			{
				const size_t budgetDepartmentCount = departmentIds.size();
				const size_t customBudgetDepartmentCount = budgetGroups.size();

				if (purposeIds.size() == budgetDepartmentCount
					&& lines.size() == budgetDepartmentCount
					&& costs.size() == budgetDepartmentCount
					&& departmentNameKeys.size() == customBudgetDepartmentCount)
				{
					for (size_t i = 0; i < budgetDepartmentCount; i++)
					{
						CustomBudgetDepartmentItemType type = CustomBudgetDepartmentItemType::Invalid;

						switch (purposeIds[i])
						{
						case kCustomBudgetDepartmentExpensePurposeId:
							type = CustomBudgetDepartmentItemType::Expense;
							break;
						case kCustomBudgetDepartmentIncomePurposeId:
							type = CustomBudgetDepartmentItemType::Income;
							break;
						}

						if (type != CustomBudgetDepartmentItemType::Invalid)
						{
							const uint32_t departmentId = departmentIds[i];
							const auto& budgetGroupItem = budgetGroups.find(departmentId);
							const auto& departmentNameItem = departmentNameKeys.find(departmentId);

							if (budgetGroupItem != budgetGroups.end()
								&& departmentNameItem != departmentNameKeys.end())
							{
								items.push_back(CustomBudgetDepartmentInfo(
									type,
									departmentId,
									lines[i],
									budgetGroupItem->second,
									costs[i],
									departmentNameItem->second));
							}
						}
					}
				}
			}
		}
	}

	return items;
}
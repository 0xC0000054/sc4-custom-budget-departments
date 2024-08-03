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

#include "LineItemTransaction.h"
#include "cIGZIStream.h"
#include "cIGZOStream.h"

namespace
{
	bool ReadBoolean(cIGZIStream& stream, bool& value)
	{
		// We use this because GetUint8 always returns false.
		uint8_t temp = 0;

		if (!stream.GetVoid(&temp, 1))
		{
			return false;
		}

		value = temp != 0;
		return true;
	}

	bool WriteBoolean(cIGZOStream& stream, bool value)
	{
		uint8_t temp = static_cast<uint8_t>(value);

		return stream.SetVoid(&temp, 1);
	}
}

LineItemTransaction::LineItemTransaction()
	: algorithm(),
	  perBuildingFixedCashFlow(0),
	  isIncome(false)
{
}

LineItemTransaction::LineItemTransaction(
	const cISCPropertyHolder* pPropertyHolder,
	TransactionAlgorithmType type,
	int64_t perBuildingFixedCashFlow,
	uint32_t lineNumber,
	bool isIncome)
	: algorithm(TransactionAlgorithmFactory::Create(pPropertyHolder, type, lineNumber, isIncome)),
	  perBuildingFixedCashFlow(perBuildingFixedCashFlow),
	  isIncome(isIncome)
{
}

LineItemTransaction::LineItemTransaction(LineItemTransaction&& other) noexcept
{
	algorithm = std::move(other.algorithm);
	perBuildingFixedCashFlow = std::exchange(other.perBuildingFixedCashFlow, 0);
	isIncome = std::exchange(other.isIncome, false);
}

LineItemTransaction& LineItemTransaction::operator=(LineItemTransaction&& other) noexcept
{
	algorithm = std::move(other.algorithm);
	perBuildingFixedCashFlow = std::exchange(other.perBuildingFixedCashFlow, 0);
	isIncome = std::exchange(other.isIncome, false);

	return *this;
}

int64_t LineItemTransaction::CalculateLineItemTotal(int64_t buildingCount)
{
	int64_t total = 0;

	if (buildingCount > 0)
	{
		total = perBuildingFixedCashFlow * buildingCount;

		if (algorithm)
		{
			total = algorithm->Calculate(total);
		}
	}

	return total;
}

bool LineItemTransaction::IsFixedCost() const
{
	// Fixed expense/income is represented by a null ITransactionAlgorithm.
	return !algorithm;
}

bool LineItemTransaction::IsIncome() const
{
	return isIncome;
}

bool LineItemTransaction::Read(cIGZIStream& stream)
{
	uint32_t version = 0;
	if (!stream.GetUint32(version) || version != 1)
	{
		return false;
	}

	if (!stream.GetSint64(perBuildingFixedCashFlow))
	{
		return false;
	}

	if (!ReadBoolean(stream, isIncome))
	{
		return false;
	}

	uint32_t algorithmTypeAsUInt32 = 0;
	if (!stream.GetUint32(algorithmTypeAsUInt32))
	{
		return false;
	}

	TransactionAlgorithmType algorithmType = static_cast<TransactionAlgorithmType>(algorithmTypeAsUInt32);
	// The Fixed algorithm type is represented by a null ITransactionAlgorithm.
	// Any variable expense/income algorithm will have an actual ITransactionAlgorithm instance
	// that calculates the line item costs using to the specified algorithm.

	algorithm = TransactionAlgorithmFactory::Create(algorithmType);

	if (algorithm)
	{
		if (!algorithm->Read(stream))
		{
			return false;
		}
	}

	return true;
}


bool LineItemTransaction::Write(cIGZOStream& stream) const
{
	if (!stream.SetUint32(1)) // version
	{
		return false;
	}

	if (!stream.SetSint64(perBuildingFixedCashFlow))
	{
		return false;
	}

	if (!WriteBoolean(stream, isIncome))
	{
		return false;
	}

	// Fixed expense/income is represented by a null ITransactionAlgorithm.
	// Any variable expense/income algorithm will have an actual TransactionAlgorithm instance
	// that calculates the line item costs using to the specified algorithm.

	if (algorithm)
	{
		if (!stream.SetUint32(static_cast<uint32_t>(algorithm->GetAlgorithmType())))
		{
			return false;
		}

		if (!algorithm->Write(stream))
		{
			return false;
		}
	}
	else
	{
		if (!stream.SetUint32(static_cast<uint32_t>(TransactionAlgorithmType::Fixed)))
		{
			return false;
		}
	}

	return true;
}

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
#include "cIGZSerializable.h"
#include "ITransactionAlgorithm.h"
#include "TransactionAlgorithmFactory.h"
#include <memory>

class LineItemTransaction final
{
public:
	LineItemTransaction();
	LineItemTransaction(
		const cISCPropertyHolder* pPropertyHolder,
		TransactionAlgorithmType type,
		int64_t perBuildingFixedCashFlow,
		uint32_t lineNumber,
		bool isIncome);

	LineItemTransaction(const LineItemTransaction&) = delete;
	LineItemTransaction(LineItemTransaction&&) noexcept;

	LineItemTransaction& operator=(const LineItemTransaction&) = delete;
	LineItemTransaction& operator=(LineItemTransaction&&) noexcept;

	int64_t CalculateLineItemTotal(int64_t buildingCount);

	bool IsFixedCost() const;
	bool IsIncome() const;

	bool Read(cIGZIStream& stream);
	bool Write(cIGZOStream& stream) const;

private:
	std::unique_ptr<ITransactionAlgorithm> algorithm;
	int64_t perBuildingFixedCashFlow;
	bool isIncome;
};


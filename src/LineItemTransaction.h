/*
 * This file is part of sc4-custom-budget-departments, a DLL Plugin for
 * SimCity 4 that allows new budget departments to be added to the game
 * using a building's exemplar.
 *
 * Copyright (C) 2024, 2025 Nicholas Hayes
 *
 * sc4-custom-budget-departments is free software: you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * sc4-custom-budget-departments is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with sc4-custom-budget-departments.
 * If not, see <http://www.gnu.org/licenses/>.
 */

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

	int64_t CalculateLineItemTotal(int64_t buildingCount) const;

	bool IsFixedCost() const;
	bool IsIncome() const;

	bool Read(cIGZIStream& stream);
	bool Write(cIGZOStream& stream) const;

private:
	std::unique_ptr<ITransactionAlgorithm> algorithm;
	int64_t perBuildingFixedCashFlow;
	bool isIncome;
};


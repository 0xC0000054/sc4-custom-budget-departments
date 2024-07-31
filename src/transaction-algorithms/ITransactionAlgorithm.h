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
#include <cstdint>

class cIGZIStream;
class cIGZOStream;

enum class TransactionAlgorithmType : uint32_t
{
	Fixed = 0,
	ResidentialTotalPopulation = 1,
};

class ITransactionAlgorithm
{
public:
	virtual TransactionAlgorithmType GetAlgorithmType() const = 0;

	/**
	 * @brief Calculates the line item's total income or expense.
	 * @param initialTotal The initial total income or expense for the line item.
	 * @return The calculated total income or expense for the line item.
	 */
	virtual int64_t Calculate(int64_t initialTotal) = 0;

	virtual bool Read(cIGZIStream& stream) = 0;
	virtual bool Write(cIGZOStream& stream) const = 0;
};

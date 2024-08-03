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
#include "ITransactionAlgorithm.h"

class TourismAlgorithm : public ITransactionAlgorithm
{
public:
	TourismAlgorithm();
	TourismAlgorithm(
		float nationalAndInternationalTourismFactor,
		int64_t geopoliticsFactor);

	TransactionAlgorithmType GetAlgorithmType() const override;

	int64_t Calculate(int64_t initialTotal) override;

	bool Read(cIGZIStream& stream) override;
	bool Write(cIGZOStream& stream) const override;

private:
	int64_t GetRegionalTourismPopulation(uint32_t demandId) const;

	float nationalAndInternationalTourismFactor;
	int64_t geopoliticsFactor;
};


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

class ResidentialWealthGroupPopulationAlgorithm : public ITransactionAlgorithm
{
public:
	ResidentialWealthGroupPopulationAlgorithm();
	ResidentialWealthGroupPopulationAlgorithm(
		float residentialLowWealthFactor,
		float residentialMediumWealthFactor,
		float residentialHighWealthFactor);

	TransactionAlgorithmType GetAlgorithmType() const override;

	int64_t Calculate(int64_t initialTotal) override;

	bool Read(cIGZIStream& stream) override;
	bool Write(cIGZOStream& stream) const override;

private:
	float lowWealthPopulationFactor;
	float mediumWealthPopulationFactor;
	float highWealthPopulationFactor;
};


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

class cISC4ResidentialSimulator;

class ResidentialTotalPopulationAlgorithm : public ITransactionAlgorithm
{
public:
	ResidentialTotalPopulationAlgorithm();
	ResidentialTotalPopulationAlgorithm(float factor);

	TransactionAlgorithmType GetAlgorithmType() const override;

	int64_t Calculate(int64_t fixedCashFlow) override;

	bool Read(cIGZIStream& stream) override;
	bool Write(cIGZOStream& stream) const override;

private:
	float populationFactor;
};


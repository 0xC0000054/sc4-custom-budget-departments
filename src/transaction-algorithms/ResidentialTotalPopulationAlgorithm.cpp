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

#include "ResidentialTotalPopulationAlgorithm.h"
#include "cIGZIStream.h"
#include "cIGZOStream.h"
#include "TransactionAlgorithmStaticPointers.h"

ResidentialTotalPopulationAlgorithm::ResidentialTotalPopulationAlgorithm()
	: populationFactor(0.005f)
{
}

ResidentialTotalPopulationAlgorithm::ResidentialTotalPopulationAlgorithm(float factor)
	: populationFactor(factor)
{
}

TransactionAlgorithmType ResidentialTotalPopulationAlgorithm::GetAlgorithmType() const
{
	return TransactionAlgorithmType::ResidentialTotalPopulation;
}

int64_t ResidentialTotalPopulationAlgorithm::Calculate(int64_t initialTotal)
{
	int64_t newTotal = initialTotal;

	if (spPopulationProvider)
	{
		const double cityPopulation = static_cast<double>(spPopulationProvider->GetCityResidentialPopulation());
		const double variableTotal = cityPopulation * populationFactor;

		newTotal += static_cast<int64_t>(variableTotal);
	}

	return newTotal;
}

bool ResidentialTotalPopulationAlgorithm::Read(cIGZIStream& stream)
{
	return stream.GetFloat32(populationFactor);
}

bool ResidentialTotalPopulationAlgorithm::Write(cIGZOStream& stream) const
{
	return stream.SetFloat32(populationFactor);
}

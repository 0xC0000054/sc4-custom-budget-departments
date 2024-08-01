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

#include "ResidentialWealthGroupPopulationAlgorithm.h"
#include "cIGZIStream.h"
#include "cIGZOStream.h"
#include "TransactionAlgorithmStaticPointers.h"

ResidentialWealthGroupPopulationAlgorithm::ResidentialWealthGroupPopulationAlgorithm()
	: lowWealthPopulationFactor(0),
	  mediumWealthPopulationFactor(0),
	  highWealthPopulationFactor(0)
{
}

ResidentialWealthGroupPopulationAlgorithm::ResidentialWealthGroupPopulationAlgorithm(
	float lowWealthFactor,
	float mediumWealthFactor,
	float highWealthFactor)
	: lowWealthPopulationFactor(lowWealthFactor),
	  mediumWealthPopulationFactor(mediumWealthFactor),
	  highWealthPopulationFactor(highWealthFactor)

{
}

TransactionAlgorithmType ResidentialWealthGroupPopulationAlgorithm::GetAlgorithmType() const
{
	return TransactionAlgorithmType::ResidentialWealthGroupPopulation;
}

int64_t ResidentialWealthGroupPopulationAlgorithm::Calculate(int64_t initialTotal)
{
	int64_t newTotal = initialTotal;

	if (spPopulationProvider)
	{
		const double lowWealthPopulation = static_cast<double>(spPopulationProvider->GetCityPopulation(0x1010));
		const double lowWealthVariableTotal = lowWealthPopulation * lowWealthPopulationFactor;

		newTotal += static_cast<int64_t>(lowWealthVariableTotal);

		const double mediumWealthPopulation = static_cast<double>(spPopulationProvider->GetCityPopulation(0x1020));
		const double mediumWealthVariableTotal = mediumWealthPopulation * mediumWealthPopulationFactor;

		newTotal += static_cast<int64_t>(mediumWealthVariableTotal);

		const double highWealthPopulation = static_cast<double>(spPopulationProvider->GetCityPopulation(0x1030));
		const double highWealthVariableTotal = highWealthPopulation * highWealthPopulationFactor;

		newTotal += static_cast<int64_t>(highWealthVariableTotal);
	}

	return newTotal;
}

bool ResidentialWealthGroupPopulationAlgorithm::Read(cIGZIStream& stream)
{
	return stream.GetFloat32(lowWealthPopulationFactor)
		&& stream.GetFloat32(mediumWealthPopulationFactor)
		&& stream.GetFloat32(highWealthPopulationFactor);
}

bool ResidentialWealthGroupPopulationAlgorithm::Write(cIGZOStream& stream) const
{
	return stream.SetFloat32(lowWealthPopulationFactor)
		&& stream.SetFloat32(mediumWealthPopulationFactor)
		&& stream.SetFloat32(highWealthPopulationFactor);
}

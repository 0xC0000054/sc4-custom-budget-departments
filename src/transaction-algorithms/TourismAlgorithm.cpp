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

#include "TourismAlgorithm.h"
#include "cIGZIStream.h"
#include "cIGZOStream.h"
#include "TransactionAlgorithmStaticPointers.h"

TourismAlgorithm::TourismAlgorithm()
	: nationalAndInternationalTourismFactor(0),
	  geopoliticsFactor(0)
{
}

TourismAlgorithm::TourismAlgorithm(
	float nationalAndInternationalTourismFactor,
	int64_t geopoliticsFactor)
	: nationalAndInternationalTourismFactor(nationalAndInternationalTourismFactor),
	  geopoliticsFactor(geopoliticsFactor)
{
}

TransactionAlgorithmType TourismAlgorithm::GetAlgorithmType() const
{
	return TransactionAlgorithmType::Tourism;
}

int64_t TourismAlgorithm::Calculate(int64_t initialTotal)
{
	int64_t newTotal = initialTotal;

	if (spPopulationProvider)
	{
		// The city and regional residential populations are used to simulate a local/national
		// tourism mechanic. The algorithm is described below:
		//
		// x = Low Wealth Population City
		// y = Medium Wealth Population City
		// z = High Wealth Population City
		// j = Low Wealth Population Region
		// k = Medium Wealth Population Region
		// l = High Wealth Population Region
		// p = Geopolitics Factor
		// d = National & International Tourism factor
		//
		// Variable Expense/Income = [x + y + z + (j * d) + (k * d) + (l * d)] / p

		const int64_t cityLowWealthPopulation = static_cast<int64_t>(spPopulationProvider->GetCityPopulation(0x1010));
		const int64_t cityMediumWealthPopulation = static_cast<int64_t>(spPopulationProvider->GetCityPopulation(0x1020));
		const int64_t cityHighWealthPopulation = static_cast<int64_t>(spPopulationProvider->GetCityPopulation(0x1030));
		const int64_t regionLowWealthTourismPopulation = GetRegionalTourismPopulation(0x1010);
		const int64_t regionMediumWealthTourismPopulation = GetRegionalTourismPopulation(0x1020);
		const int64_t regionHighWealthTourismPopulation = GetRegionalTourismPopulation(0x1030);

		const int64_t populationSum = cityLowWealthPopulation
									+ cityMediumWealthPopulation
									+ cityHighWealthPopulation
									+ regionLowWealthTourismPopulation
									+ regionMediumWealthTourismPopulation
									+ regionHighWealthTourismPopulation;

		const int64_t variableTransaction = populationSum / geopoliticsFactor;

		newTotal += variableTransaction;
	}

	return newTotal;
}

bool TourismAlgorithm::Read(cIGZIStream& stream)
{
	return stream.GetFloat32(nationalAndInternationalTourismFactor)
		&& stream.GetSint64(geopoliticsFactor);
}

bool TourismAlgorithm::Write(cIGZOStream& stream) const
{
	return stream.SetFloat32(nationalAndInternationalTourismFactor)
		&& stream.SetSint64(geopoliticsFactor);
}

int64_t TourismAlgorithm::GetRegionalTourismPopulation(uint32_t demandId) const
{
	const double population = static_cast<double>(spPopulationProvider->GetRegionPopulation(demandId));

	return static_cast<int64_t>(population * static_cast<double>(nationalAndInternationalTourismFactor));
}

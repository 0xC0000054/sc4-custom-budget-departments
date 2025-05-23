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

int64_t ResidentialWealthGroupPopulationAlgorithm::Calculate(int64_t perBuildingFixedCashFlow, int64_t buildingCount) const
{
	int64_t newTotal = perBuildingFixedCashFlow * buildingCount;

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

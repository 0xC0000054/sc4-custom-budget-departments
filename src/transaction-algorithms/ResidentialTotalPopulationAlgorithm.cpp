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

int64_t ResidentialTotalPopulationAlgorithm::Calculate(int64_t initialTotal) const
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

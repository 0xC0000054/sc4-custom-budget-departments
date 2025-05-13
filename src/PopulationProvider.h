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
#include "IPopulationProvider.h"

class cISC4DemandSimulator;
class cISC4Region;
class cISC4ResidentialSimulator;

class PopulationProvider : public IPopulationProvider
{
public:
	PopulationProvider();

	bool Init();
	bool Shutdown();

	int32_t GetCityResidentialPopulation() override;
	int32_t GetCityPopulation(uint32_t demandId) override;
	int64_t GetRegionResidentialPopulation() override;
	int64_t GetRegionPopulation(uint32_t demandId) override;

private:
	bool CalculateRegionalPopulation(
		cISC4Region* pRegion,
		int32_t currentCityX,
		int32_t currentCityZ);

	cISC4ResidentialSimulator* pResidentialSimulator;
	cISC4DemandSimulator* pDemandSimulator;
	int64_t regionResidentialPopulation;
	int64_t regionResidentialLowWealthPopulation;
	int64_t regionResidentialMediumWealthPopulation;
	int64_t regionResidentialHighWealthPopulation;
	bool initialized;
};


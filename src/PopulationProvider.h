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


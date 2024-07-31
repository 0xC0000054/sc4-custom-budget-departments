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
#include <cstdint>

class IPopulationProvider
{
public:
	virtual int32_t GetCityResidentialPopulation() = 0;
	virtual int32_t GetCityPopulation(uint32_t demandId) = 0;

	virtual int64_t GetRegionResidentialPopulation() = 0;
	virtual int64_t GetRegionPopulation(uint32_t demandId) = 0;
};
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

#include "PopulationProvider.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Demand.h"
#include "cISC4DemandSimulator.h"
#include "cISC4Region.h"
#include "cISC4RegionalCity.h"
#include "cISC4ResidentialSimulator.h"
#include "GZServPtrs.h"

PopulationProvider::PopulationProvider()
	: pResidentialSimulator(nullptr),
	  pDemandSimulator(nullptr),
	  regionResidentialPopulation(0),
	  regionResidentialLowWealthPopulation(0),
	  regionResidentialMediumWealthPopulation(0),
	  regionResidentialHighWealthPopulation(0),
	  initialized(false)
{
}

bool PopulationProvider::Init()
{
	bool result = true;

	if (!initialized)
	{
		initialized = true;
		result = false;

		cISC4AppPtr pSC4App;

		if (pSC4App)
		{
			cISC4City* pCity = pSC4App->GetCity();
			cISC4RegionalCity* pCurrentRegionalCity = pSC4App->GetRegionalCity();

			if (pCity && pCurrentRegionalCity)
			{
				pResidentialSimulator = pCity->GetResidentialSimulator();
				pDemandSimulator = pCity->GetDemandSimulator();

				if (pResidentialSimulator && pDemandSimulator)
				{
					int32_t x = 0;
					int32_t z = 0;

					pCurrentRegionalCity->GetPosition(x, z);

					result = CalculateRegionalPopulation(pSC4App->GetRegion(), x, z);
				}
			}
		}
	}

	return result;
}

bool PopulationProvider::Shutdown()
{
	pResidentialSimulator = nullptr;
	pDemandSimulator = nullptr;
	initialized = false;

	return true;
}

int32_t PopulationProvider::GetCityResidentialPopulation()
{
	return pResidentialSimulator ? pResidentialSimulator->GetPopulation() : 0;
}

int32_t PopulationProvider::GetCityPopulation(uint32_t demandId)
{
	int32_t value = 0;

	if (pDemandSimulator)
	{
		const cISC4Demand* pDemand = pDemandSimulator->GetDemand(demandId, 0);

		if (pDemand)
		{
			value = static_cast<int32_t>(pDemand->QuerySupplyValue());
		}
	}

	return value;
}

int64_t PopulationProvider::GetRegionResidentialPopulation()
{
	return regionResidentialPopulation;
}

int64_t PopulationProvider::GetRegionPopulation(uint32_t demandId)
{
	int64_t value = 0;

	switch (demandId)
	{
	case 0x1010:
		value = regionResidentialLowWealthPopulation;
		break;
	case 0x1020:
		value = regionResidentialMediumWealthPopulation;
		break;
	case 0x1030:
		value = regionResidentialHighWealthPopulation;
		break;
	}

	return value;
}

bool PopulationProvider::CalculateRegionalPopulation(cISC4Region* pRegion, int32_t currentCityX, int32_t currentCityZ)
{
	regionResidentialPopulation = 0;
	regionResidentialLowWealthPopulation = 0;
	regionResidentialMediumWealthPopulation = 0;
	regionResidentialHighWealthPopulation = 0;

	bool result = false;

	if (pRegion)
	{
		eastl::vector<cISC4Region::cLocation> cityLocations;

		pRegion->GetCityLocations(cityLocations);

		const size_t count = cityLocations.size();

		for (size_t i = 0; i < count; i++)
		{
			const cISC4Region::cLocation& location = cityLocations[i];

			// The current city is excluded from the regional totals
			// because its population will be queried from other sources.
			if (location.x == currentCityX && location.z == currentCityZ)
			{
				continue;
			}

			// The city pointer should not be released.

			cISC4RegionalCity** ppRegionalCity = pRegion->GetCity(location.x, location.z);

			if (ppRegionalCity && *ppRegionalCity)
			{
				cISC4RegionalCity* pRegionalCity = *ppRegionalCity;

				if (pRegionalCity->GetEstablished())
				{
					regionResidentialPopulation += pRegionalCity->GetPopulation();
					regionResidentialLowWealthPopulation += pRegionalCity->GetPopulation(0x1010);
					regionResidentialMediumWealthPopulation += pRegionalCity->GetPopulation(0x1020);
					regionResidentialHighWealthPopulation += pRegionalCity->GetPopulation(0x1030);
				}
			}
		}
		result = true;
	}

	return result;
}

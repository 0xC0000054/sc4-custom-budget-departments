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

#include "TransactionAlgorithmFactory.h"
#include "cIGZVariant.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "ResidentialTotalPopulationAlgorithm.h"

static constexpr uint32_t ResidentialTotalPopulationExpenseFactorPropertyId = 0x9EE12410;
static constexpr uint32_t ResidentialTotalPopulationIncomeFactorPropertyId = 0x9EE12411;

namespace
{
	bool GetPropertyValue(const cISCPropertyHolder* pPropertyHolder, uint32_t id, float& value)
	{
		if (pPropertyHolder)
		{
			const cISCProperty* property = pPropertyHolder->GetProperty(id);

			if (property)
			{
				const cIGZVariant* pVariant = property->GetPropertyValue();

				if (pVariant)
				{
					const uint16_t type = pVariant->GetType();

					if (type == cIGZVariant::Type::Float32)
					{
						return pVariant->GetValFloat32(value);
					}
					else if (type == cIGZVariant::Type::Float32Array && pVariant->GetCount() == 1)
					{
						value = *pVariant->RefFloat32();
						return true;
					}
				}
			}
		}

		return false;
	}
}

std::unique_ptr<ITransactionAlgorithm> TransactionAlgorithmFactory::Create(TransactionAlgorithmType type)
{
	switch (type)
	{
	case TransactionAlgorithmType::Fixed:
		// The Fixed algorithm type is represented by a null ITransactionAlgorithm.
		return std::unique_ptr<ITransactionAlgorithm>();
	case TransactionAlgorithmType::ResidentialTotalPopulation:
		return std::make_unique<ResidentialTotalPopulationAlgorithm>();
	default:
		throw CreateTransactionAlgorithmException("Unknown TransactionAlgorithmType value.");
	}
}

std::unique_ptr<ITransactionAlgorithm> TransactionAlgorithmFactory::Create(
	const cISCPropertyHolder* pPropertyHolder,
	TransactionAlgorithmType type,
	bool isIncome)
{
	std::unique_ptr<ITransactionAlgorithm> algorithm;
	// The Fixed algorithm type is represented by a null ITransactionAlgorithm.

	if (type == TransactionAlgorithmType::ResidentialTotalPopulation)
	{
		float factor = 0.0f;

		if (!GetPropertyValue(
			pPropertyHolder,
			isIncome ? ResidentialTotalPopulationIncomeFactorPropertyId : ResidentialTotalPopulationExpenseFactorPropertyId,
			factor))
		{
			throw CreateTransactionAlgorithmException("Failed to get the ResidentialTotalPopulation property value.");
		}

		algorithm = std::make_unique<ResidentialTotalPopulationAlgorithm>(factor);
	}

	return algorithm;
}

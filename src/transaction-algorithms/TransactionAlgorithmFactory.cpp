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
#include "SCPropertyUtil.h"
#include "ResidentialTotalPopulationAlgorithm.h"
#include "ResidentialWealthGroupPopulationAlgorithm.h"
#include "TourismAlgorithm.h"

#include <cstdarg>
#include <vector>

static constexpr uint32_t ResidentialTotalPopulationExpenseFactorPropertyId = 0x9EE12410;
static constexpr uint32_t ResidentialTotalPopulationIncomeFactorPropertyId = 0x9EE12411;
static constexpr uint32_t ResidentialWealthGroupPopulationExpenseFactorPropertyId = 0x9EE12412;
static constexpr uint32_t ResidentialWealthGroupPopulationIncomeFactorPropertyId = 0x9EE12413;
static constexpr uint32_t ResidentialTourismPopulationFactorsPropertyId = 0x9EE12414;

namespace
{
	void ThrowCreateImageExceptionFormatted(const char* const format, ...)
	{
		va_list args;
		va_start(args, format);

		va_list argsCopy;
		va_copy(argsCopy, args);

		int formattedStringLength = std::vsnprintf(nullptr, 0, format, argsCopy);

		va_end(argsCopy);

		if (formattedStringLength > 0)
		{
			size_t formattedStringLengthWithNull = static_cast<size_t>(formattedStringLength) + 1;

			constexpr size_t stackBufferSize = 1024;

			if (formattedStringLengthWithNull >= stackBufferSize)
			{
				std::unique_ptr<char[]> buffer = std::make_unique_for_overwrite<char[]>(formattedStringLengthWithNull);

				std::vsnprintf(buffer.get(), formattedStringLengthWithNull, format, args);

				va_end(args);

				throw CreateTransactionAlgorithmException(buffer.get());
			}
			else
			{
				char buffer[stackBufferSize]{};

				std::vsnprintf(buffer, stackBufferSize, format, args);

				va_end(args);

				throw CreateTransactionAlgorithmException(buffer);
			}
		}
	}

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

	bool GetPropertyValue(const cISCPropertyHolder* pPropertyHolder, uint32_t id, std::vector<float>& values)
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

					if (type == cIGZVariant::Type::Float32Array)
					{
						const uint32_t count = pVariant->GetCount();
						const float* pData = pVariant->RefFloat32();

						values.reserve(count);

						for (uint32_t i = 0; i < count; i++)
						{
							values.push_back(pData[i]);
						}

						return true;
					}
				}
			}
		}

		return false;
	}

	size_t FindLineItemDataStartIndex(
		const int64_t* pData,
		size_t count,
		int64_t lineNumber,
		size_t groupCountWithLineNumber)
	{
		// The collection must have enough data for at least one item group
		// and must be evenly divisible by the item group size.
		if (count >= groupCountWithLineNumber && (count % groupCountWithLineNumber) == 0)
		{
			for (size_t i = 0; i < count; i += groupCountWithLineNumber)
			{
				// The first item in the group is always the line item id.
				if (pData[i] == lineNumber)
				{
					return i;
				}
			}
		}

		// No matching item was found.
		return SIZE_MAX;
	}

	std::vector<int64_t> GetLineItemData(
		const cISCPropertyHolder* pPropertyHolder,
		uint32_t propertyID,
		int64_t lineNumber,
		size_t groupCountWithLineNumber,
		const char* const propertyName)
	{
		static const char* const GenericErrorFormat = "Failed to get the %s property value.";

		std::vector<int64_t> lineItemData;

		if (pPropertyHolder)
		{
			const cISCProperty* property = pPropertyHolder->GetProperty(propertyID);

			if (property)
			{
				const cIGZVariant* pVariant = property->GetPropertyValue();

				if (pVariant)
				{
					if (pVariant->GetType() == cIGZVariant::Type::Sint64Array)
					{
						const uint32_t count = pVariant->GetCount();
						const int64_t* pData = pVariant->RefSint64();

						const size_t lineItemStartIndex = FindLineItemDataStartIndex(
							pData,
							count,
							lineNumber,
							groupCountWithLineNumber);

						if (lineItemStartIndex == SIZE_MAX)
						{
							ThrowCreateImageExceptionFormatted(
								"The %s property does not contain line item 0x%08x.",
								lineNumber);
						}

						lineItemData.reserve(groupCountWithLineNumber - 1);

						for (size_t i = lineItemStartIndex + 1; i < groupCountWithLineNumber; i++)
						{
							lineItemData.push_back(pData[i]);
						}
					}
					else
					{
						ThrowCreateImageExceptionFormatted("The %s property type is not Sint64Array.", propertyName);
					}
				}
				else
				{
					ThrowCreateImageExceptionFormatted(GenericErrorFormat, propertyName);
				}
			}
			else
			{
				ThrowCreateImageExceptionFormatted(GenericErrorFormat, propertyName);
			}
		}
		else
		{
			ThrowCreateImageExceptionFormatted(GenericErrorFormat, propertyName);
		}

		return lineItemData;
	}

	float Rational64ToFloat(
		int64_t numerator,
		int64_t denominator,
		const char* const propertyName,
		const char* const valueName,
		uint32_t lineNumber)
	{
		// We limit the rational values to the range of int32_t.
		// This is done to ensure the values fit in a double.
		if (numerator < INT32_MIN || numerator > INT32_MAX)
		{
			ThrowCreateImageExceptionFormatted(
				"Error parsing the %s factor for %s property line item 0x%08x: "
				"The numerator must be in the range of -2,147,483,648 to 2,147,483,647.",
				valueName,
				propertyName,
				lineNumber);
			return 0.0f;
		}
		else if (denominator <= 0 || denominator > INT32_MAX)
		{
			ThrowCreateImageExceptionFormatted(
				"Error parsing the %s factor for %s property line item 0x%08x: "
				"The denominator must be in the range of 1 to 2,147,483,647.",
				valueName,
				propertyName,
				lineNumber);
			return 0.0f;
		}

		float value = 0.0f;

		if (numerator == 0)
		{
			value = 0.0f;
		}
		else
		{
			double temp = static_cast<double>(static_cast<int32_t>(numerator)) / static_cast<double>(static_cast<int32_t>(denominator));
			value = static_cast<float>(temp);
		}

		return value;
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
	case TransactionAlgorithmType::ResidentialWealthGroupPopulation:
		return std::make_unique<ResidentialWealthGroupPopulationAlgorithm>();
	case TransactionAlgorithmType::Tourism:
		return std::make_unique<TourismAlgorithm>();
	default:
		throw CreateTransactionAlgorithmException("Unknown TransactionAlgorithmType value.");
	}
}

std::unique_ptr<ITransactionAlgorithm> TransactionAlgorithmFactory::Create(
	const cISCPropertyHolder* pPropertyHolder,
	TransactionAlgorithmType type,
	uint32_t lineNumber,
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
	else if (type == TransactionAlgorithmType::ResidentialWealthGroupPopulation)
	{
		std::vector<float> values;

		if (!GetPropertyValue(
			pPropertyHolder,
			isIncome ? ResidentialWealthGroupPopulationIncomeFactorPropertyId : ResidentialWealthGroupPopulationExpenseFactorPropertyId,
			values))
		{
			throw CreateTransactionAlgorithmException("Failed to get the ResidentialWealthGroupPopulation property value.");
		}

		if (values.size() != 3)
		{
			throw CreateTransactionAlgorithmException("The ResidentialWealthGroupPopulation property must have 3 Float32 values.");
		}

		algorithm = std::make_unique<ResidentialWealthGroupPopulationAlgorithm>(values[0], values[1], values[2]);
	}
	else if (type == TransactionAlgorithmType::Tourism)
	{
		std::vector<int64_t> lineItemData = GetLineItemData(
			pPropertyHolder,
			ResidentialTourismPopulationFactorsPropertyId,
			lineNumber,
			4,
			"ResidentialTourismPopulation");

		float nationalAndInternationalTourismFactor = Rational64ToFloat(
			lineItemData[0],
			lineItemData[1],
			"ResidentialTourismPopulation",
			"national and international tourism",
			lineNumber);

		int64_t geopoliticsFactor = lineItemData[2];

		algorithm = std::make_unique<TourismAlgorithm>(nationalAndInternationalTourismFactor, geopoliticsFactor);
	}

	return algorithm;
}

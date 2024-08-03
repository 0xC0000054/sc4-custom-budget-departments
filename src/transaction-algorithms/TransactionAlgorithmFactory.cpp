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

static constexpr uint32_t ResidentialTotalPopulationFactorPropertyId = 0x9EE12410;
static constexpr uint32_t ResidentialWealthGroupPopulationFactorsPropertyId = 0x9EE12411;
static constexpr uint32_t ResidentialTourismPopulationFactorsPropertyId = 0x9EE12412;

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
	uint32_t lineNumber)
{
	std::unique_ptr<ITransactionAlgorithm> algorithm;
	// The Fixed algorithm type is represented by a null ITransactionAlgorithm.

	if (type == TransactionAlgorithmType::ResidentialTotalPopulation)
	{
		std::vector<int64_t> lineItemData = GetLineItemData(
			pPropertyHolder,
			ResidentialTotalPopulationFactorPropertyId,
			lineNumber,
			3,
			"ResidentialTotalPopulation");

		float factor = Rational64ToFloat(
			lineItemData[0],
			lineItemData[1],
			"ResidentialTotalPopulation",
			"total population",
			lineNumber);

		algorithm = std::make_unique<ResidentialTotalPopulationAlgorithm>(factor);
	}
	else if (type == TransactionAlgorithmType::ResidentialWealthGroupPopulation)
	{
		std::vector<int64_t> lineItemData = GetLineItemData(
			pPropertyHolder,
			ResidentialWealthGroupPopulationFactorsPropertyId,
			lineNumber,
			7,
			"ResidentialWealthGroupPopulation");

		float lowWealthFactor = Rational64ToFloat(
			lineItemData[0],
			lineItemData[1],
			"ResidentialWealthGroupPopulation",
			"low wealth",
			lineNumber);

		float mediumWealthFactor = Rational64ToFloat(
			lineItemData[2],
			lineItemData[3],
			"ResidentialWealthGroupPopulation",
			"medium wealth",
			lineNumber);

		float highWealthFactor = Rational64ToFloat(
			lineItemData[4],
			lineItemData[5],
			"ResidentialWealthGroupPopulation",
			"high wealth",
			lineNumber);

		algorithm = std::make_unique<ResidentialWealthGroupPopulationAlgorithm>(
			lowWealthFactor,
			mediumWealthFactor,
			highWealthFactor);
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

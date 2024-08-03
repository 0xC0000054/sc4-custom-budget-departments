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
#include "ITransactionAlgorithm.h"
#include <memory>
#include <stdexcept>

class cISCPropertyHolder;

class CreateTransactionAlgorithmException final : public std::runtime_error
{
public:
	CreateTransactionAlgorithmException(const char* const message) : std::runtime_error(message)
	{
	}

	CreateTransactionAlgorithmException(const std::string& message) : std::runtime_error(message)
	{
	}
};

namespace TransactionAlgorithmFactory
{
	std::unique_ptr<ITransactionAlgorithm> Create(TransactionAlgorithmType type);

	std::unique_ptr<ITransactionAlgorithm> Create(
		const cISCPropertyHolder* pPropertyHolder,
		TransactionAlgorithmType type,
		uint32_t lineNumber);
}
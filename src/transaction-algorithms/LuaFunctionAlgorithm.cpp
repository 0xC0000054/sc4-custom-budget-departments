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

#include "LuaFunctionAlgorithm.h"
#include "cIGZIStream.h"
#include "cIGZOStream.h"
#include "cISCLua.h"
#include "Logger.h"
#include "SafeInt.hpp"
#include "TransactionAlgorithmStaticPointers.h"

LuaFunctionAlgorithm::LuaFunctionAlgorithm()
	: functionName()
{
}

LuaFunctionAlgorithm::LuaFunctionAlgorithm(const cRZBaseString& functionName)
	: functionName(functionName)
{
}

TransactionAlgorithmType LuaFunctionAlgorithm::GetAlgorithmType() const
{
	return TransactionAlgorithmType::LuaFunction;
}

int64_t LuaFunctionAlgorithm::Calculate(int64_t perBuildingFixedCashFlow, int64_t buildingCount) const
{
	int64_t total = 0;

	if (spLua)
	{
		int32_t top = spLua->GetTop();
		spLua->GetGlobal(functionName.ToChar());

		if (spLua->GetTop() != top)
		{
			cIGZLua5Thread::LuaType type = spLua->Type(-1);

			if (type == cIGZLua5Thread::LuaTypeFunction)
			{
				// The Lua function we are calling has the following C++ signature:
				// double function_name(double perBuildingFixedCashFlow, double buildingCount)
				//
				// The calling sequence is:
				// 1. Push the parameters onto the Lua stack.
				// 2. Call the function.
				// 3. Read the returned number off the Lua stack.

				spLua->PushNumber(static_cast<double>(perBuildingFixedCashFlow));
				spLua->PushNumber(static_cast<double>(buildingCount));
				int32_t status = spLua->CallProtected(2, 1);

				if (status == 0)
				{
					if (spLua->IsNumber(-1))
					{
						double result = spLua->ToNumber(-1);

						if (!SafeCast(result, total))
						{
							total = 0;

							char errorMessageBuffer[1024]{};

							std::snprintf(
								errorMessageBuffer,
								sizeof(errorMessageBuffer),
								"%f cannot be represented as a signed 64-bit integer.",
								result);

							LogLuaFunctionCallError(errorMessageBuffer);
						}
					}
					else
					{
						LogLuaFunctionCallError("The function return value must be a Number.");
					}
				}
				else
				{
					LogLuaFunctionCallError("The game returned an error when calling the function.");
				}
			}
			else
			{
				LogLuaFunctionCallError("The function has the wrong type.");
			}

			spLua->SetTop(top);
		}
		else
		{
			LogLuaFunctionCallError("The function was not found.");
		}
	}

	return total;
}

bool LuaFunctionAlgorithm::Read(cIGZIStream& stream)
{
	return stream.GetGZStr(functionName);
}

bool LuaFunctionAlgorithm::Write(cIGZOStream& stream) const
{
	return stream.SetGZStr(functionName);
}

void LuaFunctionAlgorithm::LogLuaFunctionCallError(const char* message) const
{
	Logger::GetInstance().WriteLineFormatted(
		LogLevel::Error,
		"Error calling Lua function '%s': %s",
		functionName.ToChar(),
		message);
}

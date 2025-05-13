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

#include "DebugUtil.h"
#include "cIGZString.h"
#include "cIGZVariant.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISC4Occupant.h"
#include "cRZAutoRefCount.h"
#include "StringResourceKey.h"
#include "StringResourceManager.h"
#include <memory>
#include <Windows.h>

void DebugUtil::PrintLineToDebugOutput(const char* const line)
{
	OutputDebugStringA(line);
	OutputDebugStringA("\n");
}

void DebugUtil::PrintLineToDebugOutputFormatted(const char* const format, ...)
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

			PrintLineToDebugOutput(buffer.get());
		}
		else
		{
			char buffer[stackBufferSize]{};

			std::vsnprintf(buffer, stackBufferSize, format, args);

			PrintLineToDebugOutput(buffer);
		}
	}

	va_end(args);
}


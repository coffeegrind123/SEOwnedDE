#pragma once

#include <cstdint>
#include <Psapi.h>
#include <windows.h>

// Enhanced pattern scanning for 64-bit TF2 Steam overlay
// Based on learn_more implementation with 64-bit improvements
#define INRANGE(x, a, b) (x >= a && x <= b) 
#define getBits(x) (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xA): (INRANGE(x, '0', '9') ? x - '0': 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

/**
 * @brief Enhanced pattern finder with bounds checking for 64-bit addresses
 * @param start_address Starting address to search from
 * @param end_address Ending address to search to
 * @param target_pattern Pattern string (e.g., "48 8B ? 88 00 00 00 E8")
 * @return Address where pattern was found, or 0 if not found
 */
static uintptr_t FindPattern(const uintptr_t& start_address, const uintptr_t& end_address, const char* target_pattern) {
	if (!target_pattern || start_address >= end_address) {
		return 0;
	}

	const char* pattern = target_pattern;
	uintptr_t first_match = 0;

	for (uintptr_t position = start_address; position < end_address; position++) {
		if (!*pattern)
			return first_match;

		const uint8_t pattern_current = *reinterpret_cast<const uint8_t*>(pattern);
		const uint8_t memory_current = *reinterpret_cast<const uint8_t*>(position);

		if (pattern_current == '\?' || memory_current == getByte(pattern)) {
			if (!first_match)
				first_match = position;

			if (!pattern[2])
				return first_match;

			pattern += pattern_current != '\?' ? 3 : 2;
		}
		else {
			pattern = target_pattern;
			first_match = 0;
		}
	}

	return 0;
}

/**
 * @brief Find pattern in a specific module with enhanced validation
 * @param module Module name (e.g., "gameoverlayrenderer64.dll")
 * @param target_pattern Pattern to search for
 * @return Address where pattern was found, or 0 if not found
 */
static uintptr_t FindPattern(const char* module, const char* target_pattern) {
	if (!module || !target_pattern) {
		return 0;
	}

	HMODULE moduleHandle = GetModuleHandleA(module);
	if (!moduleHandle) {
		return 0;
	}

	MODULEINFO module_info = { 0 };
	if (!GetModuleInformation(GetCurrentProcess(), moduleHandle, &module_info, sizeof(MODULEINFO))) {
		return 0;
	}

	const uintptr_t start_address = reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll);
	const uintptr_t end_address = start_address + module_info.SizeOfImage;

	// Validate address range for 64-bit
	if (start_address < 0x10000 || end_address <= start_address) {
		return 0;
	}

	return FindPattern(start_address, end_address, target_pattern);
}

/**
 * @brief Validate if a memory address is executable and safe to use
 * @param address Address to validate
 * @return true if address is valid and executable
 */
static bool IsValidExecutableAddress(uintptr_t address) {
	if (!address || address < 0x10000) {
		return false;
	}

	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi))) {
		return false;
	}

	return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
}

/**
 * @brief Extract function address from LEA instruction pattern (64-bit)
 * Used to extract Steam overlay function addresses from call patterns
 * @param patternAddr Address where the call pattern was found
 * @param leaOffset Offset to the LEA instruction (typically -7 for Steam patterns)
 * @return Extracted function address, or 0 if extraction failed
 */
static uintptr_t ExtractFunctionFromLEA(uintptr_t patternAddr, int leaOffset = -7) {
	if (!patternAddr) {
		return 0;
	}

	uint8_t* leaAddr = reinterpret_cast<uint8_t*>(patternAddr + leaOffset);
	
	// Verify LEA instruction: 48 8D 15 (LEA RDX, [RIP+offset])
	if (leaAddr[0] == 0x48 && leaAddr[1] == 0x8D && leaAddr[2] == 0x15) {
		// Extract 32-bit relative offset
		int32_t relativeOffset = *reinterpret_cast<int32_t*>(leaAddr + 3);
		// Calculate absolute address: instruction_end + relative_offset
		uintptr_t functionAddr = reinterpret_cast<uintptr_t>(leaAddr + 7) + relativeOffset;
		
		if (IsValidExecutableAddress(functionAddr)) {
			return functionAddr;
		}
	}

	return 0;
}
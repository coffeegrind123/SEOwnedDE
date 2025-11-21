#include "CPredictionCopy.h"

// Exact Amalgam TransferData implementation using signature call
int CPredictionCopy::TransferData(const char* operation, int entindex, datamap_t* dmap)
{
	// Use SEOwnedDE's signature-based calling for exact 1:1 compatibility
	// SEOwnedDE uses Signatures:: namespace (like Amalgam uses S::)
	using TransferDataFn = int(__thiscall*)(CPredictionCopy*, const char*, int, datamap_t*);
	auto fn = reinterpret_cast<TransferDataFn>(Signatures::CPredictionCopy_TransferData.Get());
	return fn(this, operation, entindex, dmap);
}
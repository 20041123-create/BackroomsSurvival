#include "RandomRoomGenerationDataAssets.h"

FRandomRoomTemplateDefinition URandomRoomTemplateData::MakeDefinition() const
{
	FRandomRoomTemplateDefinition Result;
	Result.TemplateId = TemplateId;
	Result.Footprint = Footprint;
	Result.Connectors = Connectors;
	Result.AllowedRoomTypes = AllowedRoomTypes;
	Result.GenerationWeight = GenerationWeight;
	return Result;
}

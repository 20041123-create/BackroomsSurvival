#pragma once

#include "RandomRoomGenerationTypes.h"

namespace RandomRoomGeneration
{
 	class RANDOMROOMGENERATION_API FRandomRoomLayoutPlanner final
	{
	public:
		static FRandomRoomLayoutPlan Generate(const FRandomRoomGenerationRequest& Request);
		static bool VerifyPlan(const FRandomRoomLayoutPlan& Plan, FString& OutFailureReason);

	private:
		static bool ValidateRequest(const FRandomRoomGenerationRequest& Request, FString& OutFailureReason);
		static bool ValidateTemplate(const FRandomRoomTemplateDefinition& Template, FString& OutFailureReason);
	};
}

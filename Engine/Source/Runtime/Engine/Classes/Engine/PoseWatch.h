// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PoseWatch.generated.h"

struct FCompactHeapPose;

struct FAnimNodePoseWatch
{
	TSharedPtr<FCompactHeapPose>	PoseInfo;
	FColor							PoseDrawColour;
	int32							NodeID;
};

UCLASS()
class ENGINE_API UPoseWatch
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	// Node that we are trying to watch
	UPROPERTY()
	class UEdGraphNode* Node;

	UPROPERTY()
	FColor PoseWatchColour;
};
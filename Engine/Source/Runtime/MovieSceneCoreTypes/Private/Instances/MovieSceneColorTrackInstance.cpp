// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneColorTrackInstance.h"
#include "MatineeUtils.h"


FMovieSceneColorTrackInstance::FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack )
{
	ColorTrack = &InColorTrack;
}

void FMovieSceneColorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FLinearColor ColorValue;
	if( ColorTrack->Eval( Position, LastPosition, ColorValue ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			UObject* Object = RuntimeObjects[ObjIndex];

			UFunction* Function = RuntimeObjectToFunctionMap.FindRef( Object );
			if( Function )
			{
				Object->ProcessEvent( Function, &ColorValue );
			}
			else
			{
				// @todo Sequencer - remove this legacy method of setting the property directly
				UObject* PropertyOwner = NULL;
				UProperty* Property = NULL;
				//@todo Sequencer - Major performance problems here.  This needs to be initialized and stored (not serialized) somewhere
				void* Address = FMatineeUtils::GetPropertyAddress<void>( Object, ColorTrack->GetPropertyName(), Property, PropertyOwner );

				if (Address)
				{
					bool bIsFColor = false;
					const UStructProperty* StructProp = Cast<const UStructProperty>(Property);
					check(StructProp && StructProp->Struct);
					FName StructName = StructProp->Struct->GetFName();

					bIsFColor = StructName == NAME_Color;
					check(bIsFColor || StructName == NAME_LinearColor);

					if( bIsFColor )
					{
						*(FColor*)Address = ColorValue.ToFColor(false);
						// Let the property owner know that we changed one of its properties
						PropertyOwner->PostInterpChange( Property );
					}
					else
					{
						*(FLinearColor*)Address = ColorValue;
						// Let the property owner know that we changed one of its properties
						PropertyOwner->PostInterpChange( Property );
					}
				}
			}
		}
	}
}

void FMovieSceneColorTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	static const FString Set(TEXT("Set") );
	
	FString FunctionString = Set + ColorTrack->GetPropertyName().ToString();
	
	FName FunctionName = FName( *FunctionString );
	
	//Player.GetRuntimeObjects( )
	for( UObject* Object : RuntimeObjects )
	{
		// If the mapping for the object already exists we dont need to search for it again
		UFunction* Function = RuntimeObjectToFunctionMap.FindRef( Object );
		
		if( !Function )
		{
			Function = Object->FindFunction( FunctionName );
			if( Function )
			{
				RuntimeObjectToFunctionMap.Add( Object, Function );
			}
		}
	}
}

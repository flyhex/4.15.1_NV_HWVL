// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneColorTrack
 */
class FMovieSceneColorTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
private:
	/** The track being instanced */
	UMovieSceneColorTrack* ColorTrack;
	/** Mapping of objects to bound functions that will be called to update data on the track */
	TMap< TWeakObjectPtr<UObject>, UFunction* > RuntimeObjectToFunctionMap;
};

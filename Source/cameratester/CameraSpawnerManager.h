#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "CameraSpawnerManager.generated.h"

UCLASS(BlueprintType, Blueprintable)
class CAMERATESTER_API ACameraSpawnerManager : public AActor
{
	GENERATED_BODY()

public:    
	ACameraSpawnerManager();

protected:
	virtual void BeginPlay() override;

	// Blueprint class reference to your BP_CameraSpawner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<AActor> CameraSpawnerClass;

	// Number of instances to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	int32 SpawnCount = 5;

	// Offset between spawned instances (from first spawned camera)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	FVector SpawnOffset = FVector(400, 0, 0);

	// Array to store references to spawned cameras
	UPROPERTY(BlueprintReadOnly, Category = "Spawning")
	TArray<AActor*> SpawnedCameras;

public:
	// Function to spawn multiple camera spawners
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnMultipleCameras();

	// Get reference to specific spawned camera by index
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	AActor* GetSpawnedCamera(int32 Index);

	// Get all spawned cameras
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	TArray<AActor*> GetAllSpawnedCameras();
};

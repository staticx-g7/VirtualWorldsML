#include "CameraSpawnerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

ACameraSpawnerManager::ACameraSpawnerManager()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Initialize default values
    SpawnCount = 5;
    SpawnOffset = FVector(400, 0, 0);
}

void ACameraSpawnerManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Automatically spawn cameras when the game starts
    SpawnMultipleCameras();
}

void ACameraSpawnerManager::SpawnMultipleCameras()
{
    UE_LOG(LogTemp, Warning, TEXT("=== CameraSpawnerManager: Starting spawn process ==="));
    
    if (!CameraSpawnerClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CameraSpawnerClass is NULL! Please assign BP_CameraSpawner in the Details Panel."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) 
    {
        UE_LOG(LogTemp, Error, TEXT("World is NULL!"));
        return;
    }

    // Clear any existing spawned cameras array
    SpawnedCameras.Empty();

    FVector FirstCameraLocation;
    bool bFirstCameraSpawned = false;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d cameras with offset %s"), SpawnCount, *SpawnOffset.ToString());

    for (int32 i = 0; i < SpawnCount; i++)
    {
        FVector SpawnLocation;
        FRotator SpawnRotation = FRotator::ZeroRotator;

        if (i == 0)
        {
            // First camera spawns at the CameraSpawnerManager's location
            SpawnLocation = GetActorLocation();
            FirstCameraLocation = SpawnLocation;
            UE_LOG(LogTemp, Log, TEXT("First camera will spawn at manager location: %s"), *SpawnLocation.ToString());
        }
        else
        {
            // Subsequent cameras offset from the first camera's position
            SpawnLocation = FirstCameraLocation + (SpawnOffset * i);
            UE_LOG(LogTemp, Log, TEXT("Camera %d will spawn at: %s (offset: %s)"), 
                i + 1, *SpawnLocation.ToString(), *(SpawnOffset * i).ToString());
        }

        // Set up spawn parameters with unique name
        FActorSpawnParameters SpawnParams;
        FString UniqueName = FString::Printf(TEXT("BP_CameraSpawner_%d"), i + 1);
        SpawnParams.Name = FName(*UniqueName);
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Spawn the actor
        AActor* SpawnedActor = World->SpawnActor<AActor>(
            CameraSpawnerClass, 
            SpawnLocation, 
            SpawnRotation, 
            SpawnParams
        );

        if (SpawnedActor)
        {
            // Add to our array for tracking
            SpawnedCameras.Add(SpawnedActor);
            
            UE_LOG(LogTemp, Log, TEXT("✓ SUCCESS: Spawned %s at location %s"), 
                *SpawnedActor->GetName(), *SpawnLocation.ToString());
            
            // Optional: Set actor label in editor for better visibility
            #if WITH_EDITOR
                SpawnedActor->SetActorLabel(UniqueName);
            #endif

            if (i == 0)
            {
                bFirstCameraSpawned = true;
                UE_LOG(LogTemp, Warning, TEXT("First camera spawned successfully - using as reference point"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ FAILED: Could not spawn camera %d at location %s"), 
                i + 1, *SpawnLocation.ToString());
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Spawn process complete: %d/%d cameras spawned ==="), 
        SpawnedCameras.Num(), SpawnCount);
}

AActor* ACameraSpawnerManager::GetSpawnedCamera(int32 Index)
{
    if (SpawnedCameras.IsValidIndex(Index))
    {
        return SpawnedCameras[Index];
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Invalid camera index: %d (valid range: 0-%d)"), 
        Index, SpawnedCameras.Num() - 1);
    return nullptr;
}

TArray<AActor*> ACameraSpawnerManager::GetAllSpawnedCameras()
{
    return SpawnedCameras;
}

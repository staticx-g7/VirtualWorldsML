#include "RenderTargetManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ARenderTargetManager::ARenderTargetManager()
{
    PrimaryActorTick.bCanEverTick = false;
    RenderTargetWidth = 512;
    RenderTargetHeight = 512;
    bForceFrameUpdates = true;
    UpdateFrequency = 0.0f;
    MaxDepthDistance = 10000.0f;
}

void ARenderTargetManager::BeginPlay()
{
    Super::BeginPlay();

    FTimerHandle InitTimer;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(InitTimer, this, &ARenderTargetManager::DetectAndCreateRenderTargets, 5.0f, false);
        
        if (bForceFrameUpdates && UpdateFrequency > 0.0f)
        {
            StartPeriodicUpdates();
        }
    }
}

void ARenderTargetManager::DetectAndCreateRenderTargets()
{
    UE_LOG(LogTemp, Warning, TEXT("=== RenderTargetManager: Starting dynamic detection ==="));

    if (!IsValid(this) || !GetWorld() || !TargetActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid prerequisites for render target detection."));
        return;
    }

    CreatedRenderTargets.Reset();

    TArray<AActor*> FoundActors = GetAllInstancesOfTargetActor();
    for (int32 i = 0; i < FoundActors.Num(); i++)
    {
        AActor* Camera = FoundActors[i];
        if (!IsValid(Camera))
            continue;

        UTextureRenderTarget2D* RT = CreateRenderTargetForActor(Camera, i + 1);
        if (IsValid(RT))
        {
            CreatedRenderTargets.Add(RT);
            USceneCaptureComponent2D* SceneCap = Camera->FindComponentByClass<USceneCaptureComponent2D>();
            if (IsValid(SceneCap))
            {
                SceneCap->TextureTarget = RT;
                SetupSceneCaptureComponent(SceneCap, ESceneCaptureSource::SCS_FinalColorLDR);
            }
        }
    }

    ConfigureCaptureSettings();
    UE_LOG(LogTemp, Warning, TEXT("=== Detection complete: %d render targets created/assigned ==="), CreatedRenderTargets.Num());
}

UTextureRenderTarget2D* ARenderTargetManager::CreateRenderTargetForActor(AActor* Actor, int32 Index, ETextureRenderTargetFormat Format)
{
    if (!IsValid(Actor) || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid parameters for render target creation"));
        return nullptr;
    }

    UTextureRenderTarget2D* NewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    if (IsValid(NewRenderTarget))
    {
        NewRenderTarget->RenderTargetFormat = Format;
        NewRenderTarget->InitAutoFormat(RenderTargetWidth, RenderTargetHeight);
    }

    return NewRenderTarget;
}

TArray<AActor*> ARenderTargetManager::GetAllInstancesOfTargetActor()
{
    TArray<AActor*> FoundActors;
    if (GetWorld() && TargetActorClass)
    {
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), TargetActorClass, FoundActors);
    }
    return FoundActors;
}

TArray<UTextureRenderTarget2D*> ARenderTargetManager::GetAllRenderTargets()
{
    return CreatedRenderTargets;
}

void ARenderTargetManager::ForceUpdateAllRenderTargets()
{
    TArray<AActor*> Cameras = GetAllInstancesOfTargetActor();
    int32 UpdateCount = 0;
    
    for (AActor* Camera : Cameras)
    {
        if (!IsValid(Camera))
            continue;
            
        TArray<USceneCaptureComponent2D*> SceneCaptures;
        Camera->GetComponents<USceneCaptureComponent2D>(SceneCaptures);
        
        for (USceneCaptureComponent2D* SceneCap : SceneCaptures)
        {
            if (IsValid(SceneCap) && IsValid(SceneCap->TextureTarget))
            {
                if (!SceneCap->bCaptureEveryFrame)
                {
                    SceneCap->CaptureScene();
                    UpdateCount++;
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Force updated %d scene capture components"), UpdateCount);
}

void ARenderTargetManager::StartPeriodicUpdates()
{
    if (GetWorld() && UpdateFrequency > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ARenderTargetManager::UpdateRenderTargets, UpdateFrequency, true);
        UE_LOG(LogTemp, Log, TEXT("Started periodic render target updates every %.2f seconds"), UpdateFrequency);
    }
}

void ARenderTargetManager::StopPeriodicUpdates()
{
    if (GetWorld() && UpdateTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
        UE_LOG(LogTemp, Log, TEXT("Stopped periodic render target updates"));
    }
}

void ARenderTargetManager::ConfigureCaptureSettings()
{
    TArray<AActor*> Cameras = GetAllInstancesOfTargetActor();
    
    for (AActor* Camera : Cameras)
    {
        if (!IsValid(Camera))
            continue;
            
        TArray<USceneCaptureComponent2D*> SceneCaptures;
        Camera->GetComponents<USceneCaptureComponent2D>(SceneCaptures);
        
        for (USceneCaptureComponent2D* SceneCap : SceneCaptures)
        {
            if (IsValid(SceneCap))
            {
                SceneCap->bCaptureEveryFrame = bForceFrameUpdates;
                SceneCap->bCaptureOnMovement = true;
                SceneCap->SetActive(true);
                SceneCap->MaxViewDistanceOverride = 100000.0f;
                SceneCap->LODDistanceFactor = 1.0f;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Configured capture settings: CaptureEveryFrame=%s"), 
        bForceFrameUpdates ? TEXT("True") : TEXT("False"));
}

void ARenderTargetManager::UpdateRenderTargets()
{
    ForceUpdateAllRenderTargets();
}

void ARenderTargetManager::SetupSceneCaptureComponent(USceneCaptureComponent2D* SceneCapture, ESceneCaptureSource CaptureSource)
{
    if (!IsValid(SceneCapture))
        return;
        
    SceneCapture->CaptureSource = CaptureSource;
    SceneCapture->bCaptureEveryFrame = bForceFrameUpdates;
    SceneCapture->bCaptureOnMovement = true;
    SceneCapture->SetActive(true);
    SceneCapture->MaxViewDistanceOverride = 100000.0f;
    SceneCapture->LODDistanceFactor = 1.0f;
}

#if WITH_EDITOR
void ARenderTargetManager::CreatePersistentCameraRenderTargets()
{
    FString OutputFolder = TEXT("/Game/RenderTargets");
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectContentDir() + "RenderTargets"), true);

    // Clear existing arrays
    RGBRenderTargets.Empty();
    DepthRenderTargets.Empty();
    MLDepthRenderTargets.Empty(); // NEW
    NormalRenderTargets.Empty();

    TArray<AActor*> Cameras = GetAllInstancesOfTargetActor();

    for (int32 i = 0; i < Cameras.Num(); ++i)
    {
        AActor* Camera = Cameras[i];
        if (!IsValid(Camera))
            continue;

        TArray<USceneCaptureComponent2D*> SceneCaptures;
        Camera->GetComponents<USceneCaptureComponent2D>(SceneCaptures);

        // RGB Buffer
        if (bCreateRGBBuffer && SceneCaptures.Num() > 0)
        {
            UTextureRenderTarget2D* RGBRT = CreateRenderTargetAsset(
                FString::Printf(TEXT("RT_RGB_Camera_%d"), i + 1),
                RTF_RGBA8
            );
            SceneCaptures[0]->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
            SceneCaptures[0]->TextureTarget = RGBRT;
            SetupSceneCaptureComponent(SceneCaptures[0], ESceneCaptureSource::SCS_FinalColorLDR);
            RGBRenderTargets.Add(RGBRT);
            UE_LOG(LogTemp, Log, TEXT("✓ Created RGB render target for camera %d"), i + 1);
        }

        // Raw Depth Buffer
        if (bCreateDepthBuffer && SceneCaptures.Num() > 1)
        {
            UTextureRenderTarget2D* DepthRT = CreateRenderTargetAsset(
                FString::Printf(TEXT("RT_Depth_Camera_%d"), i + 1),
                RTF_R32f
            );
            SceneCaptures[1]->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
            SceneCaptures[1]->TextureTarget = DepthRT;
            SetupSceneCaptureComponent(SceneCaptures[1], ESceneCaptureSource::SCS_SceneDepth);
            DepthRenderTargets.Add(DepthRT);
            UE_LOG(LogTemp, Log, TEXT("✓ Created Raw Depth render target for camera %d (red debug view)"), i + 1);
        }

        // NEW: ML Depth Buffer (Normalized for Machine Learning)
        if (bCreateMLDepthBuffer)
        {
            CreateMLDepthCaptureForCamera(Camera, i);
        }

        // Normal Buffer
        if (bCreateNormalBuffer && SceneCaptures.Num() > 2)
        {
            UTextureRenderTarget2D* NormalRT = CreateRenderTargetAsset(
                FString::Printf(TEXT("RT_Normal_Camera_%d"), i + 1),
                RTF_RGBA8
            );
            SceneCaptures[2]->CaptureSource = ESceneCaptureSource::SCS_Normal;
            SceneCaptures[2]->TextureTarget = NormalRT;
            SetupSceneCaptureComponent(SceneCaptures[2], ESceneCaptureSource::SCS_Normal);
            NormalRenderTargets.Add(NormalRT);
            UE_LOG(LogTemp, Log, TEXT("✓ Created Normal render target for camera %d"), i + 1);
        }
    }

    ConfigureAllSceneCaptureSettings();

    UE_LOG(LogTemp, Warning, TEXT("=== ML Render Target Creation Complete ==="));
    UE_LOG(LogTemp, Warning, TEXT("Created %d RGB, %d Raw Depth, %d ML Depth, %d Normal render targets"), 
        RGBRenderTargets.Num(), DepthRenderTargets.Num(), MLDepthRenderTargets.Num(), NormalRenderTargets.Num());
}

void ARenderTargetManager::CreateMLDepthCaptureForCamera(AActor* Camera, int32 CameraIndex)
{
    // Create ML-ready normalized depth render target
    UTextureRenderTarget2D* MLDepthRT = CreateRenderTargetAsset(
        FString::Printf(TEXT("RT_DepthML_Camera_%d"), CameraIndex + 1),
        RTF_R8  // 8-bit grayscale for ML
    );

    // Create additional Scene Capture Component for normalized depth
    USceneCaptureComponent2D* MLDepthCapture = NewObject<USceneCaptureComponent2D>(Camera);
    MLDepthCapture->AttachToComponent(Camera->GetRootComponent(), 
        FAttachmentTransformRules::KeepRelativeTransform);

    // Configure ML depth capture
    MLDepthCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    MLDepthCapture->TextureTarget = MLDepthRT;
    
    // TODO: Apply depth normalization post-process material here
    // This would contain a material that samples SceneDepth, divides by MaxDepthDistance, 
    // and outputs as grayscale
    
    SetupSceneCaptureComponent(MLDepthCapture, ESceneCaptureSource::SCS_FinalColorLDR);
    
    // Register the new component
    Camera->AddInstanceComponent(MLDepthCapture);
    MLDepthCapture->RegisterComponent();

    MLDepthRenderTargets.Add(MLDepthRT);
    UE_LOG(LogTemp, Log, TEXT("✓ Created ML Depth render target for camera %d (grayscale, ML-ready)"), CameraIndex + 1);
}

void ARenderTargetManager::ConfigureAllSceneCaptureSettings()
{
    ConfigureCaptureSettings();
    
    FTimerHandle ForceUpdateTimer;
    GetWorld()->GetTimerManager().SetTimer(ForceUpdateTimer, this, &ARenderTargetManager::ForceUpdateAllRenderTargets, 1.0f, false);
    
    UE_LOG(LogTemp, Log, TEXT("Configured all Scene Capture settings for live updates"));
}

UTextureRenderTarget2D* ARenderTargetManager::CreateRenderTargetAsset(const FString& AssetName, ETextureRenderTargetFormat Format)
{
    FString PackageName = TEXT("/Game/RenderTargets/") + AssetName;
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(
        Package, *AssetName, RF_Public | RF_Standalone
    );
    RT->RenderTargetFormat = Format;
    RT->InitAutoFormat(RenderTargetWidth, RenderTargetHeight);
    RT->UpdateResourceImmediate(true);

    FAssetRegistryModule::AssetCreated(RT);
    Package->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_None;
    UPackage::SavePackage(Package, RT, *PackageFileName, SaveArgs);

    return RT;
}
#endif

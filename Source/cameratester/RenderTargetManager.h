#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RenderTargetManager.generated.h"

UCLASS(BlueprintType, Blueprintable)
class CAMERATESTER_API ARenderTargetManager : public AActor
{
    GENERATED_BODY()

public:
    ARenderTargetManager();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
    TSubclassOf<AActor> TargetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Targets")
    int32 RenderTargetWidth = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Targets")
    int32 RenderTargetHeight = 512;

    // ML Buffer Options
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML Buffers")
    bool bCreateRGBBuffer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML Buffers")
    bool bCreateDepthBuffer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML Buffers")
    bool bCreateMLDepthBuffer = true; // NEW: Normalized depth for ML

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML Buffers")
    bool bCreateNormalBuffer = false;

    // Force Update Options
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Settings")
    bool bForceFrameUpdates = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Update Settings")
    float UpdateFrequency = 0.0f;

    // Depth Normalization Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML Settings", meta = (ClampMin = "100", ClampMax = "100000"))
    float MaxDepthDistance = 10000.0f; // 100 meters in cm

    // Store render targets by type
    UPROPERTY(BlueprintReadOnly, Category = "Render Targets")
    TArray<UTextureRenderTarget2D*> CreatedRenderTargets;

    UPROPERTY(BlueprintReadOnly, Category = "ML Buffers")
    TArray<UTextureRenderTarget2D*> RGBRenderTargets;

    UPROPERTY(BlueprintReadOnly, Category = "ML Buffers")
    TArray<UTextureRenderTarget2D*> DepthRenderTargets;

    UPROPERTY(BlueprintReadOnly, Category = "ML Buffers")
    TArray<UTextureRenderTarget2D*> MLDepthRenderTargets; // NEW: Normalized depth

    UPROPERTY(BlueprintReadOnly, Category = "ML Buffers")
    TArray<UTextureRenderTarget2D*> NormalRenderTargets;

    // Timer for updates
    FTimerHandle UpdateTimer;

public:
    UFUNCTION(BlueprintCallable, Category = "Detection")
    void DetectAndCreateRenderTargets();

    UFUNCTION(BlueprintCallable, Category = "Detection")
    TArray<AActor*> GetAllInstancesOfTargetActor();

    UFUNCTION(BlueprintCallable, Category = "Render Targets")
    TArray<UTextureRenderTarget2D*> GetAllRenderTargets();

    UFUNCTION(BlueprintCallable, Category = "Render Targets")
    UTextureRenderTarget2D* CreateRenderTargetForActor(AActor* Actor, int32 Index, ETextureRenderTargetFormat Format = RTF_RGBA8);

    // Force Update Functions
    UFUNCTION(BlueprintCallable, Category = "Updates")
    void ForceUpdateAllRenderTargets();

    UFUNCTION(BlueprintCallable, Category = "Updates")
    void StartPeriodicUpdates();

    UFUNCTION(BlueprintCallable, Category = "Updates")
    void StopPeriodicUpdates();

    UFUNCTION(BlueprintCallable, Category = "Updates")
    void ConfigureCaptureSettings();

#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category = "Editor Tools")
    void CreatePersistentCameraRenderTargets();

    UFUNCTION(CallInEditor, Category = "Editor Tools")
    void ConfigureAllSceneCaptureSettings();

private:
    UTextureRenderTarget2D* CreateRenderTargetAsset(const FString& AssetName, ETextureRenderTargetFormat Format);
    void CreateMLDepthCaptureForCamera(AActor* Camera, int32 CameraIndex); // NEW
#endif

private:
    void UpdateRenderTargets();
    void SetupSceneCaptureComponent(USceneCaptureComponent2D* SceneCapture, ESceneCaptureSource CaptureSource);
};

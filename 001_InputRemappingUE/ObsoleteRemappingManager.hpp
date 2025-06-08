#pragma once

#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInputLibrary.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
////////
#include "EnhancedInput/Public/InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
////////
#include "ObsoleteRemappingManager.generated.h"


UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Rebind Setting"))
class UInputModifierCustomData : public UInputModifier
{
    GENERATED_BODY()

    friend struct FKeyMappingPack;

private:

    UPROPERTY(EditInstanceOnly, Category=Settings)
    FText CustomDisplayName;

private:

    // This modifier, while being processed in the modifier chain, does not affect anything
    FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* /*PlayerInput*/, const FInputActionValue CurrentValue, float /*DeltaTime*/) override
    {
        return CurrentValue;
    }

    /////////////////////

    [[nodiscard]] const FText& GetCustomDisplayName() const
    {
        return CustomDisplayName;
    }

};


//////////////////////////////////////////
//////////////////////////////////////////


USTRUCT(BlueprintType, Blueprintable, Category = "Rebind Setting")
struct FKeyMappingPack
{
    GENERATED_BODY();

    friend class URebindSettingController;

public:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    UInputMappingContext* MappingContext = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    const UInputAction* MappingAction = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    FKey DefaultKey{NAME_None};

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    FKey CustomKey{NAME_None};

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    FText MappingDisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rebind Setting")
    int32 MappingIndex = -1;

public:

    // This constructor is for Editor only
    // It must never be used by anybody else
    explicit FKeyMappingPack() = default;

    /////////////////////

    FORCEINLINE bool operator==(const FKeyMappingPack& Other) const
    {
        return true
        && Other.MappingContext == MappingContext
        && Other.MappingAction == MappingAction
        && Other.CustomKey == CustomKey
        && Other.MappingDisplayName.EqualTo(MappingDisplayName)
        && Other.MappingIndex == MappingIndex;
    }

    FORCEINLINE bool operator!=(const FKeyMappingPack& Other) const
    {
        return !(*this == Other);
    }

private:

    /////////////////////

    FKeyMappingPack(UInputMappingContext* Context, const UInputAction* Action, const FKey& KeyToStore, const int32 KeyIndex, const FText& MappingName) : 
        MappingContext(Context),
        MappingAction(Action),
        DefaultKey(KeyToStore),
        CustomKey(KeyToStore),
        MappingDisplayName(MappingName),
        MappingIndex(KeyIndex)
    {
        check(MappingContext);
        check(MappingAction);
        check(DefaultKey.IsValid());
        check(CustomKey.IsValid());
        check(!MappingDisplayName.IsEmpty());
        check(KeyIndex >= 0);
    }

    /////////////////////

    static FText GetMappingDisplayName(const FEnhancedActionKeyMapping& Mapping)
    {
        if (!Mapping.IsPlayerMappable()) return FText::GetEmpty();

        const TArray<TObjectPtr<UInputModifier>>& Modifiers = Mapping.Modifiers;
        for (const UInputModifier* OneModifier : Modifiers)
        {
            const auto* CustomData = Cast<const UInputModifierCustomData>(OneModifier);
            if (!static_cast<bool>(CustomData)) continue;

            const FText& NameToReturn = CustomData->GetCustomDisplayName();
            if (!NameToReturn.IsEmptyOrWhitespace()) return NameToReturn;
        }

        return FText::FromName(Mapping.GetMappingName());
    }

    /////////////////////

    // Searches the context for an input mapping, with identical name and action
    FEnhancedActionKeyMapping* ExtractSimilarMapping(UInputMappingContext* OtherContext) const
    {
        const bool bIsWrongContext =
            // Contexts are just different
            OtherContext != MappingContext
            // Current mapping was changed, so as this pack is invalid now
            || OtherContext->GetMappings().Num() <= MappingIndex;

        if (bIsWrongContext) return nullptr;

        // We assume, the mapping under the stored number is the same mapping we created this pack with
        // So, in case of any inconsistencies, this pack is deemed invalid
        FEnhancedActionKeyMapping& MappingUnderThisNumber = OtherContext->GetMapping(MappingIndex);

        // Do not consider non-editable mappings
        if (!MappingUnderThisNumber.IsPlayerMappable()) return nullptr;

        // If the action, default key or its display name are different, this pack is deemed invalid
        const bool bIsPackValid = true
            && MappingUnderThisNumber.Action == MappingAction
            && GetMappingDisplayName(MappingUnderThisNumber).EqualTo(MappingDisplayName);
        if (!bIsPackValid) return nullptr;

        // Return a pointer to the editable mapping of the same pack profile
        return &MappingUnderThisNumber;
    }
};


//////////////////////////////////////////


UCLASS(NotBlueprintType, NotBlueprintable, Category = "Rebind Setting")
class URebindSettingController : public UObject
{
    GENERATED_BODY()

private:

    // Input User settings, stored between game sessions
    // The settings are stored only in shipping builds for debug reasons!
    UPROPERTY()
    TArray<FKeyMappingPack> StableKeyMappingPacks;

private:

    // In Shipping build no keys are restored on game's end!
    #if !UE_BUILD_SHIPPING
    void BeginDestroy() override
    {
        Super::BeginDestroy();

        // Restores all
        for (FKeyMappingPack& OnePack : StableKeyMappingPacks)
        {
            RestoreDefaultKey(OnePack);
        }
    }
    #endif // !UE_BUILD_SHIPPING

////////////////////////////

    static bool IsCorrectControlMode(const FKeyMappingPack& RebindPack, const FText& ControlMode)
    {
        const FKey KeyToSelect = RebindPack.CustomKey;
        const FString& ControlModeString = ControlMode.ToString();

        if (KeyToSelect.IsTouch() && ControlModeString.Equals("Touch", ESearchCase::IgnoreCase))
        {
            return true;
        }

        if (KeyToSelect.IsGesture() && ControlModeString.Equals("VR", ESearchCase::IgnoreCase))
        {
            return true;
        }

        if (KeyToSelect.IsGamepadKey() && ControlModeString.Equals("Gamepad", ESearchCase::IgnoreCase))
        {
            return true;
        }

        const bool bIsKeyboardOrMouse = KeyToSelect.IsMouseButton() || UKismetInputLibrary::Key_IsKeyboardKey(KeyToSelect);

        return bIsKeyboardOrMouse && ControlModeString.Equals("KeyboardAndMouse", ESearchCase::IgnoreCase);
    }

////////////////////////////

    static void CollectContextsWithMappableKeys(TArray<UInputMappingContext*>& CurrentContexts)
    {
        CurrentContexts.Empty();

        const TArray<UInputMappingContext*> AllContexts = FindAllInputMappingContexts();

        // KeyMappingPack are formed from the MappingContexts
        check(!AllContexts.IsEmpty());

        for (UInputMappingContext* OneContext : AllContexts)
        {
            const TArray<FEnhancedActionKeyMapping> Mappings = OneContext->GetMappings();
            if (Mappings.IsEmpty()) continue;

            for (int Index = 0; Index < Mappings.Num(); ++Index)
            {
                const FEnhancedActionKeyMapping& OneMapping = Mappings[Index];

                // Skip, if this mapping is not editable
                if (!OneMapping.IsPlayerMappable()) continue;

                CurrentContexts.Emplace(OneContext);
            }
        }

        // Something went wrong is no context had at least one mappable mapping
        check(!CurrentContexts.IsEmpty());
    }

    void RestoreStoredKeysAndRemoveObsoleteMappings(const TArray<UInputMappingContext*>& CurrentContexts)
    {
        if (StableKeyMappingPacks.IsEmpty() || CurrentContexts.IsEmpty()) return;

        const auto RemoveObsoletePacksAndResetStoredKeys = [&CurrentContexts](const FKeyMappingPack& OneStablePack)->bool
        {
            for (UInputMappingContext* Context : CurrentContexts)
            {
                // Look for a mapping with similar name and input action
                FEnhancedActionKeyMapping* Mapping = OneStablePack.ExtractSimilarMapping(Context);

                // Skip, if this context does not contain the respective mapping OR it is not editable
                if (!Mapping || !Mapping->IsPlayerMappable()) continue;

                // If this context has the similar mapping, assign this the stored key
                Mapping->Key = OneStablePack.CustomKey;

                // This pack has proved to be still valid and must persist for this game session
                return true;
            }

            // This pack lost its respective mapping and must be removed
            return false;
        };
        StableKeyMappingPacks = StableKeyMappingPacks.FilterByPredicate(RemoveObsoletePacksAndResetStoredKeys);
    }

    void CollectNewControlSettings(const TArray<UInputMappingContext*>& CurrentContexts)
    {
        for (UInputMappingContext* OneContext : CurrentContexts)
        {
            const TArray<FEnhancedActionKeyMapping> Mappings = OneContext->GetMappings();
            if (Mappings.IsEmpty()) continue;

            for (int KeyIndex = 0; KeyIndex < Mappings.Num(); ++KeyIndex)
            {
                const FEnhancedActionKeyMapping& OneMapping = Mappings[KeyIndex];

                // Skip, if this mapping is not editable
                if (!OneMapping.IsPlayerMappable()) continue;

                const FText& DisplayName = FKeyMappingPack::GetMappingDisplayName(OneMapping);
                check(!DisplayName.IsEmptyOrWhitespace());

                const UInputAction* OneAction = OneMapping.Action.Get();
                check(OneAction);

                const FKey DefaultKey = OneMapping.Key;
                check(DefaultKey.IsValid());

                FKeyMappingPack PackToAdd{OneContext, OneAction, DefaultKey, KeyIndex, DisplayName};

                //  Skip, if this mapping has already been stored
                const auto HasAlreadyPacked = [&PackToAdd](const FKeyMappingPack& Pack)
                {
                    return (Pack == PackToAdd);
                };
                if (static_cast<bool>(StableKeyMappingPacks.FindByPredicate(HasAlreadyPacked))) continue;

                StableKeyMappingPacks.Emplace(MoveTemp(PackToAdd));
            }
        }
    }

    // This function must be called once at the start of every game session
    // This function restores valid control settings, removes obsolete ones and adds new ones
    void RecalculatePlayerMappingSettings()
    {
        // The settings are stored between game sessions only in shipping builds for debug reasons!
        #if !UE_BUILD_SHIPPING
        StableKeyMappingPacks.Empty();
        #endif // !UE_BUILD_SHIPPING

        // Collect contexts with mappable keys
        TArray<UInputMappingContext*> CurrentContexts;
        CollectContextsWithMappableKeys(CurrentContexts);

        // Restore keys from settings in valid mappings and remove obsolete ones
        RestoreStoredKeysAndRemoveObsoleteMappings(CurrentContexts);

        // Collect new control setting
        CollectNewControlSettings(CurrentContexts);
    }

////////////////////////////

    // Discovers assets of class UInputMappingContext within the project's Content folder (/Game)
    // Collects loaded and unloaded assets!
    static TArray<UInputMappingContext*> FindAllInputMappingContexts()
    {
        const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        const FString RootPath = TEXT("/Game"); // project's "Content" folder

        AssetRegistry.ScanPathsSynchronous({ RootPath }, /*bForceRescan*/ false);

        FARFilter Filter;
        Filter.bRecursivePaths = true;
        Filter.bRecursiveClasses = true;
        Filter.PackagePaths.Emplace(*RootPath);
        Filter.ClassPaths.Emplace(UInputMappingContext::StaticClass()->GetClassPathName());

        TArray<FAssetData> AssetDataArray;
        AssetRegistry.GetAssets(Filter, AssetDataArray);

        // Returned array
        TArray<UInputMappingContext*> Result;

        for (const FAssetData& Data : AssetDataArray)
        {
            if (UInputMappingContext* IMC = Cast<UInputMappingContext>(Data.GetAsset()))
            {
                Result.Emplace(IMC);
            }
        }

        return Result;
    }

////////////////////////////

    // The following functions relate to the custom Furnish Master classes
    void ApplyRebindSettings() const
    {
        //OnSettingsChange.Broadcast();
        //GetSettingsManager()->SaveSettings();
    }

////////////////////////////

    // This function is not used. In theory, we need to call it after any mapping change.
    // But everything (including Shipping) works fine without it
    void RebuildControlMappings() const
    {
        const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

        // This prevents crash, when this function is called at PIE stop
        if (!static_cast<bool>(PlayerController)) return;

        const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
        check(LocalPlayer);

        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
        check(Subsystem);

        Subsystem->RequestRebuildControlMappings();
    }

////////////////////////////

    static void UpdateKeyInContextByIndex(UInputMappingContext* MappingContext, const int32 MappingIndex, const FKey& KeyToSet)
    {
        check(MappingContext);

        // This pack is invalid, if this check fires
        check(MappingContext->GetMappings().Num() > MappingIndex);

        // Restore default key in the respective mapping context
        FEnhancedActionKeyMapping& MappingToRestore = MappingContext->GetMapping(MappingIndex);
        MappingToRestore.Key = KeyToSet;
    }

////////////////////////////

    void UpdateCustomKeyInPackAndSettings(FKeyMappingPack& PackParam, const FKey& KeyToSet)
    {
        // Restore default key in the settings
        for (FKeyMappingPack& StablePack : StableKeyMappingPacks)
        {
            if (StablePack != PackParam) continue;
            StablePack.CustomKey = KeyToSet;

            break;
        }

        // Restore default key in the pack (after settings were changed!)
        PackParam.CustomKey = KeyToSet;

        // Save settings immediately after remapping
        ApplyRebindSettings();
    }

////////////////////////////

public:

////////////////////////////

    UFUNCTION(BlueprintPure=false, meta = (ExpandBoolAsExecs = "ReturnValue"), Category = "Rebind Setting")
    bool IsCustomKeySetForThisPack(const FKeyMappingPack& Pack, bool& bIsCustomKeySet) const
    {
        bIsCustomKeySet = Pack.DefaultKey != Pack.CustomKey;
        return bIsCustomKeySet;
    }

////////////////////////////

    UFUNCTION(BlueprintPure, Category = "Rebind Setting")
    void UnpackRebindPack(const FKeyMappingPack& Pack, FText& ActionName, FKey& PackedKey) const
    {
        check(Pack.CustomKey.IsValid());
        check(!Pack.MappingDisplayName.IsEmptyOrWhitespace());

        PackedKey = Pack.CustomKey;
        ActionName = Pack.MappingDisplayName;
    }

////////////////////////////

    UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
    TArray<FKeyMappingPack> GetMappingPacksForControlMode(const FText& ControlMode) const
    {
        // This array must never be empty at this point
        // If it is, most likely we did not mark any input action for remapping
        check(!StableKeyMappingPacks.IsEmpty());

        TArray<FKeyMappingPack> ReturnPacks;

        for (const FKeyMappingPack& Pack : StableKeyMappingPacks)
        {
            if (ReturnPacks.Contains(Pack) || !IsCorrectControlMode(Pack, ControlMode)) continue;

            ReturnPacks.Emplace(Pack);
        }

        // This array also must never be empty
        check(!ReturnPacks.IsEmpty());

        return ReturnPacks;
    }

////////////////////////////

    UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
    void RemapControlKey(UPARAM(ref) FKeyMappingPack& PackParam, const FKey& KeyToSet)
    {
        // We have nothing to do, if we change no key
        //if (PackParam.CustomKey == KeyToSet) return;

        UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, KeyToSet);

        UpdateCustomKeyInPackAndSettings(PackParam, KeyToSet);
    }

////////////////////////////

    UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
    void RestoreDefaultKey(UPARAM(ref) FKeyMappingPack& PackParam)
    {
        // We have nothing to do, if we change no key
        //if (PackParam.CustomKey == PackParam.DefaultKey) return;

        UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, PackParam.DefaultKey);

        UpdateCustomKeyInPackAndSettings(PackParam, PackParam.DefaultKey);
    }

};

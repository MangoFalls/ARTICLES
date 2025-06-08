# UE Enhanced Input System: In-Game Remapping Before and After UE 5.3

We'll cover the basic approaches to creating an input remapping system in Unreal Engine 5 using the `Enhanced Input System` plugin. For UE 5.3 and newer versions, we can rely on the built-in settings system. However, for earlier versions, a custom solution is required.

# Introduction

In Unreal Engine 5, the `Enhanced Input System (EIS)` plugin handles input. It’s a more powerful tool compared to the `Input System` in previous engine versions.

Currently, the EIS supports at least two approaches to building a remappable controls system in a game.

- **Approach 1: Built-in logic**. This approach works in projects developed on engine version 5.3 or newer. It’s fairly straightforward and leverages the tools already available in EIS.

- **Approach 2: Custom logic**. This approach was relevant for projects on engine versions up to 5.2 and has, for the most part, become obsolete. However, it may still be useful for projects running on older engine versions.

## Scope of the Article

This article isn’t intended as a comprehensive tutorial. Rather, it’s an outline of my experience creating input remapping systems.

I’ve deliberately left out questions specific to individual projects, such as working with Blueprints, saving input settings within Approach 2, or integrating the system into custom widgets. These topics fall outside the scope of this article.

The goal here is to provide foundational knowledge for those encountering the need to create an input remapping system using the `Enhanced Input System` plugin for the first time.

---

# Main Classes of the Enhanced Input System

In this section, we'll overview the key entities of the Enhanced Input System that we are going to use while building an input remapping system.

1. The `UInputMappingContext` class forms the core of the Enhanced Input System. A context defines an active set of input bindings depending on the game world's state, the player's mode, and so on. For example, we'll need different sets of keys to choose menu options, move around a level, or perform actions in a mini-game. What's more, even a single game mode might require multiple contexts, like separate mappings for a keyboard-and-mouse setup versus a gamepad.

2. Each `UInputAction` object represents a specific player action - jumping, shooting, using an ability - that can be tied to one or several physical inputs (buttons, sensors). This class doesn't describe which key executes the action; that's handled by `UInputMappingContext`. `UInputAction` just defines the input's handling for an action, its type, triggers, modifiers, and so on.

<br><br>![[EIS_DataAssetTypes.png]](/001_InputRemappingUE/EIS_DataAssetTypes.png) <br>(_fig.1 UInputAction & UInputMappingContext files in Content Browser_)<br><br>

3. The `FKey` class is an abstract representation of a particular input. It encapsulates the input's identifier, which might be a keyboard key, a mouse button, a gamepad axis, a touch input, a gesture, or a specialized device.

4. It's the job of `UInputMappingContext` to hold `UInputAction & FKey` pairs, bundled together with additional parameters into `FEnhancedActionKeyMapping` objects (mappings). Besides the core pair itself, these mappings also contain `UInputTrigger` objects, which define the conditions for the action to activate with the given input, and `UInputModifier` objects, which somehow modify (filter, scale, etc.) the data from the input.

<br><br> ![[EIS_FEnhancedActionKeyMapping.png]](/001_InputRemappingUE/EIS_FEnhancedActionKeyMapping.png) <br>(_fig.2 FEnhancedActionKeyMapping entries in an UInputMappingContext file_) <br><br>

5. In addition to the general EIS classes above, the library logic for remapping input and saving those mappings relies on several classes specific to the first approach. For example, `UEnhancedInputUserSettings` manages the player's input settings, including persisting them between game sessions. The saved settings are tied to a player's profile - `UEnhancedPlayerMappableKeyProfile`. In the first approach, as in most games, we'll work with just one profile.

---

# Approach 1: Rebinding in UE 5.3 and Later

I implemented the approach outlined below while setting up the key remapping system in the game [Furnish Master](https://store.steampowered.com/app/2004080/Furnish_Master/).

<br><br> ![[EIS_FinalScreen.png]](/001_InputRemappingUE/EIS_FinalScreen.png)<br><br>

The final codebase is tightly coupled with the game’s existing architecture, so only fragments that use the relevant APIs are shown here. Methods unrelated to the remapping system have been excluded from the examples.

---

## Step 1. Enable User Input Settings

The first method for building a key remapping system relies entirely on the built-in user input settings functionality provided by EIS. So, our first step is to enable this system.

Head over to `Project Settings` → `Engine` section → `Enhanced Input` → `User Settings`.

Check the `Enable User Settings` option. If this flag is not enabled, we won't be able to access the `UEnhancedInputUserSettings` object at runtime, and any pointers to it will be null.

<br><br> ![[EIS_EnableUserSettings.png]](/001_InputRemappingUE/EIS_EnableUserSettings.png) <br>(_fig. 3 Where to find Enable User Settings in Project Settings_) <br><br>

---

## Step 2. Gather the Necessary Pointers

Most of what we’ll be doing in this approach involves the user settings object and the player's profile. Here's a function to get a pointer to both of these objects:

```cpp
UPROPERTY()
TObjectPtr<UEnhancedInputUserSettings> UserSettings = nullptr;

UPROPERTY()
TObjectPtr<UEnhancedPlayerMappableKeyProfile> CurrentProfile = nullptr;

///////////////

void InitUserSettingsAndProfile()
{
    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

    UserSettings = InputSubsystem->GetUserSettings();
    CurrentProfile = UserSettings->GetCurrentKeyProfile();
}
```

---

## Step 3. Enabling Remappability for Input Actions

To let the user settings system work with the existing input setup, we need to grant it access to the relevant `Input Action` assets.

We should leave all `Input Actions` that aren't supposed to be remappable unchanged.

For all other `Input Actions`, we need to bind them to the input settings system. There are two ways we can achieve this:

---

### **Method 3.1. Editing the `UInputAction` Asset**

In the **Content Browser**, locate and open the `Input Action` asset whose bindings we want to make remappable.

Inside the **User Settings** section, set a valid class under the `Player Mappable Key Settings` field. At this point, there's typically just one valid class available by default: `Player Mappable Key Settings`.

<br><br> ![[EIS_Settings_in_UInputAction.png]](/001_InputRemappingUE/EIS_Settings_in_UInputAction.png) <br>(_fig.4 Remapping settings in UInputAction_) <br><br>

It’s also a good idea to fill out the `Name` and `Display Name` fields.

The first one is the system-internal name for the action - it maps to the `FName MappingName` variable in the `FPlayerKeyMapping` struct.

The second one is the user-facing, localizable name - it maps to the `FText DisplayName` variable in the same struct.

This method effectively exposes the `UInputAction` for remapping in all contexts where the asset is used.

> **Note:** This approach only works reliably if a single `FKey` is assigned to a single `UInputAction` in exactly one `UInputMappingContext`.

If, for example, we register (see Step 4) two contexts that both use the same `UInputAction`, the settings system will only keep the `FPlayerKeyMapping` from the most recently registered context.

This happens because all actions exposed using this method share the same name, and the system doesn’t track which context a remapping came from. As a result, previously registered mappings tied to that `UInputAction` get overwritten.

To understand this behavior and how the mappings get overridden, take a look at `UEnhancedInputUserSettings::RegisterKeyMappingsToProfile()`.

---

### **Method 3.2. Editing the `UInputMappingContext` Asset**

In the **Content Browser**, locate and open the `Input Mapping Context` asset whose bindings (all or just some) we want to make remappable.

Within the mapping entry, find the `Setting Behavior` field. We can choose from several options here:

<br><br> ![[EIS_Settings_in_UInputMappingContext.png]](/001_InputRemappingUE/EIS_Settings_in_UInputMappingContext.png) <br>(_fig.5 Remapping settings in UInputMappingContext_) <br><br>

1. **`InheritSettingsFromAction`**: As the name suggests, the settings from the associated `UInputAction` will be inherited. This is the default setting for mappings inside an `Input Mapping Context`, which is why the logic described in Method 3.1 works. We can’t customize the mapping individually with this mode.

2. **`OverrideSettings`**: This option allows - and actually _requires_ - us to provide a unique name for the mapping. Whether we’re remapping something like the classic WASD layout (four keys for one movement `UInputAction`), or something more exotic, this will usually be the most practical choice when building a flexible in-game settings system.

3. **`IgnoreSettings`**: A somewhat niche but occasionally useful option. This lets us explicitly exclude a mapping from the remapping system. If we use this, mappings based on the same `UInputAction` - even if they have remapping settings enabled - will be registered, but this particular one will be ignored.

## Step 4. Registering Input Mapping Contexts

All preparations are complete, so we can now proceed to writing code.

At this point, we need to specify which `UInputMappingContext` objects our user settings system will work with.

To do this, we register the appropriate contexts using the `RegisterInputMappingContexts()` function - or more precisely, the `RegisterInputMappingContext()` function that it calls internally - within the `UEnhancedInputUserSettings` class.

For example:

```cpp
void RegisterAllContextsToRemap()
{
    TArray<UInputMappingContext*> MappingContexts;
    CollectContextsWithMappableKeys(MappingContexts);

    for (const UInputMappingContext* Context : MappingContexts)
    {
        UserSettings->RegisterInputMappingContext(Context);
    }
}
```

The registration functions iterate over all mappings (`FPlayerKeyMapping`) contained in each context and collect only those where the associated `Input Action` is marked as remappable (see Step 3 above). All other mappings are ignored.

Eligible mappings are saved into the input settings profile (`UEnhancedPlayerMappableKeyProfile`), which is where we'll be working with them from now on.

All settings we collect - and any further changes we make - are saved into the project's `Saved` directory. As a result, deleting the `Saved` folder will completely remove all profiles along with every associated input configuration.

You don't need to worry about re-registering contexts that were already registered - doing so won't affect the state of any existing `FPlayerKeyMapping` objects.

To reset any input settings created this way back to their original defaults, you can use `UEnhancedInputUserSettings::ResetKeyProfileToDefault()`:

```cpp
FGameplayTagContainer FailureReason;
UserSettings->ResetKeyProfileToDefault(UserSettings->GetCurrentKeyProfileIdentifier(), FailureReason);
```

## Step 5. Reading the Current Input Settings State

Next, we’ll define a structure for storing and working with input data at the Blueprint level.

Our structure, `FKeyMappingPack`, is similar to the built-in `FPlayerKeyMapping` structure, but only includes the fields and methods we actually need.

```cpp
USTRUCT(BlueprintType, Blueprintable, Category = "Rebind Setting")
struct FKeyMappingPack
{
    GENERATED_BODY();
private:
    FText DisplayName;
    FName MappingName;
    FKey DefaultKey{NAME_None};
    FKey CustomKey{NAME_None};
    EPlayerMappableKeySlot Slot;

public:
    explicit FKeyMappingPack() = default;
    explicit FKeyMappingPack(const FPlayerKeyMapping& Mapping);

/////////////////////////

    bool IsCustomized() const { return (CustomKey != DefaultKey); }
    FKey GetDefaultKey() const { return DefaultKey; }
    FMapPlayerKeyArgs GetKeyArgs(const FKey& NewKey = FKey{}) const;
    void SetCustomKey(const FKey& NewKey);
}
```

To directly work with input settings, we'll first need to gather the relevant data from the profile. Once we've done that, we can pass this information into the Blueprint system for reading and displaying it in the input settings menu - as well as for directly handling remapping operations there.

In the `IteratePlayerKeyMappings()` function, we loop through the mappings stored in the profile and perform a specific operation on each, as defined by the `Callback` object.

The `GetMappingPacksForControlMode()` function creates a lambda to filter mappings by input type - an aspect determined by the control system used in _Furnish Master_. The `IteratePlayerKeyMappings()` function takes this lambda and returns the filtered array.

```cpp
void IteratePlayerKeyMappings(const TFunction<void(const FPlayerKeyMapping&)>& Callback) const
{
    const TMap<FName, FKeyMappingRow>& MappingRows = CurrentProfile->GetPlayerMappingRows();

    for (const auto& [RowName, OneRow] : MappingRows)
    {
        const FKeyMappingRow* MutableRow = CurrentProfile->FindKeyMappingRow(RowName);
        const TSet<FPlayerKeyMapping>& RowMappings = MutableRow->Mappings;
        for (const FPlayerKeyMapping& Mapping : RowMappings)
        {
            Callback(Mapping);
        }
    }
}

//////////////////////////

UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
TArray<FKeyMappingPack> GetMappingPacksForControlMode(const FName& ControlMode) const
{
    TArray<FKeyMappingPack> OutKeyMapping;
    const auto PackCollector = [&OutKeyMapping, &ControlMode](const FPlayerKeyMapping& Mapping)
    {
        if (!IsCorrectControlType(Mapping.GetCurrentKey(), ControlMode)) return;
        OutKeyMapping.Emplace(FKeyMappingPack{Mapping});
    };

    IteratePlayerKeyMappings(PackCollector);
    return OutKeyMapping;
}
```

---

## Step 6. Remapping Input and Saving It

We've gathered all the data we need to work with input settings. Now there's just one final step.

To actually remap input keys, we'll use the existing `MapPlayerKey()` function.

This function takes an `FMapPlayerKeyArgs` object as its first parameter. Before calling it, we'll need to populate this struct with the name of the `Input Action`, the `FKey` we want to assign to that action, and the `EPlayerMappableKeySlot` - based on the assumption that a single action within one context can have multiple input options.

To finalize the input change in the profile, we call the existing `SaveSettings()` function.

As soon as this function completes, the new configuration is saved and will persist even after restarting the game. The actual saving is handled by the EIS plugin itself.

```cpp
UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
void RemapControlKey(FKeyMappingPack& PackParam, const FKey& KeyToSet) const
{
    MapNewKeyInSettings(PackParam, KeyToSet);
}

//////////////////////////

UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
void RestoreDefaultKey(FKeyMappingPack& PackParam) const
{
    MapNewKeyInSettings(PackParam, PackParam.GetDefaultKey());
}

//////////////////////////

void MapNewKeyInSettings(FKeyMappingPack& PackParam, const FKey& KeyToSet) const
{
    FGameplayTagContainer FailureReason;
    UserSettings->MapPlayerKey(PackParam.GetKeyArgs(KeyToSet), FailureReason);
    UserSettings->SaveSettings();
    PackParam.SetCustomKey(KeyToSet);
}

//////////////////////////

FMapPlayerKeyArgs FKeyMappingPack::GetKeyArgs(const FKey& NewKey) const
{
    FMapPlayerKeyArgs Args;
    (NewKey.IsValid())? (Args.NewKey = NewKey) : (Args.NewKey = DefaultKey);
    Args.MappingName = MappingName;
    Args.Slot = Slot;
    return Args;
}
```

# Approach 2: Remapping Prior to UE 5.3

## Step 1. Mark Input Actions for Mapping

The beginning of this custom approach fully repeats Step 3 of the previous section.

<br><br> ![[EIS_PlayerMappableKeySettingsUE5Old.png]](/001_InputRemappingUE/EIS_PlayerMappableKeySettingsUE5Old.png) <br><br>

In the settings of our `Input Action` assets, under the `Player Mappable Key Settings` field, we assign any valid class (by default, only one is available).

If we want unique mapping names for the same `Input Action`, we assign a valid setting class to mappings in the `UInputMappingContext` assets - just as we did earlier.

```cpp
USTRUCT(BlueprintType, Blueprintable, Category = "Rebind Setting")
struct FKeyMappingPack
{
    GENERATED_BODY();

public:
    TObjectPtr<UInputMappingContext> MappingContext = nullptr;
    TObjectPtr<const UInputAction> MappingAction = nullptr;
    FKey DefaultKey{NAME_None};
    FKey CustomKey{NAME_None};
    FText MappingDisplayName;
    int32 MappingIndex = -1;
};
```

## Step 2. Create a Structure for Transferring Mapping Data

Our key rebinding system needs a structure for transferring data about both default and currently active mappings - something similar to what we built in Step 5 of the first approach.

The `FKeyMappingPack` struct works similarly to `FPlayerKeyMapping`, but only includes the fields we actually need, most importantly a reference to the owning `UInputMappingContext`.

## Step 3. Gather the Mappings

This step is similar to how we registered input contexts in Step 4 of the first approach, with one key difference: this time, we perform that 'registration' ourselves and store the output locally.

Our goal here is to extract editable action-key pairs from the available input contexts - in particular, from the `FEnhancedActionKeyMapping` entries they contain.

Besides `UInputAction` and `FKey`, we'll also find other useful data, such as the mapping index.

The `CollectNewControlSettings()` function might look bulky at first glance, but in essence, all it does is collect mapping data and package it into an array of `FKeyMappingPack` objects.

```cpp
UPROPERTY()
TArray<FKeyMappingPack> StableKeyMappingPacks;

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
                
                const FText& DisplayName = OneMapping.GetDisplayName();
                const UInputAction* OneAction = OneMapping.Action.Get();
                const FKey DefaultKey = OneMapping.Key;
                FKeyMappingPack PackToAdd{OneContext, OneAction, DefaultKey, KeyIndex, DisplayName};

                //  Skip, if this mapping has already been stored
                const auto HasAlreadyPacked = [&PackToAdd](const FKeyMappingPack& Pack)
                {
                    return (Pack == PackToAdd);
                };
                if (static_cast<bool>(StableKeyMappingPacks.FindByPredicate(HasAlreadyPacked))) continue;

                StableKeyMappingPacks.Emplace(PackToAdd);
            }
        }
    }
```

## Step 4. Reassigning Keys

Now we need to change a specific key in a particular mapping within a given context. We can solve this in two different ways.

> **Warning!** The actions described below involve direct modification of the context files (`UInputMappingContext`) located in the project’s `Content` folder.
> Any changes we make to these contexts during an editor play session will persist even after the session ends!

### Option 2.1: Completely Removing and Recreating the Mapping

In the first method, we remove the original mapping from the context and create a new one in its place.

To do this, we use the built-in library functions `UInputMappingContext::UnmapKey()` and `UInputMappingContext::MapKey()`.

As you'll see in the code below, in addition to swapping one key for another, we also need to manually transfer at least the _triggers_ and _modifiers_ from the removed mapping to the new one.

That said, some fields - specifically `SettingBehavior` and `PlayerMappableKeySettings` - are marked as `protected`. Because of this, we can’t configure them directly, and they’ll fall back to default values when the new mapping is created.

While this behavior may seem counterintuitive, in my experience, removing and recreating mappings this way doesn’t affect the validity of their indices as stored in `FKeyMappingPack`. The newly created mapping is still accessible via the same index as the one that was removed, even though its position within the `UInputMappingContext` asset may shift visually. Still, this should be treated as an unpredictable detail, and we strongly advise against relying on it.

Overall, I _don’t_ recommend this method. It’s included here strictly for completeness - and as a cautionary example of what _not_ to do.

```cpp
static void ReplaceKeyInContext(UInputMappingContext* MappingContext, const UInputAction* ActionToRebind, const FKey& OldKey, const FKey& NewKey)
{
    // Cache the properties of the key, being rebound in the target MappingContext
    const auto [CachedTriggers, CachedModifiers] = CacheTriggersAndModifiers(MappingContext, OldKey, ActionToRebind);

    // Remove the old bound key from its mapping context
    MappingContext->UnmapKey(ActionToRebind, OldKey);

    // Add a new key for the action to the mapping context
    FEnhancedActionKeyMapping& ActionKeyMapping = MappingContext->MapKey(ActionToRebind, NewKey);

    // Restore the properties of the newly rebound key
    ActionKeyMapping.Modifiers = CachedModifiers;
    ActionKeyMapping.Triggers = CachedTriggers;
}

//////////////////////////

static TPair<TArray<UInputTrigger*>, TArray<UInputModifier*>> CacheTriggersAndModifiers(const UInputMappingContext* MappingContext, const FKey& KeyToUnmap, const UInputAction* ActionToRebind)
{
    // Get all mapped keys from the mapping context
    const TArray<FEnhancedActionKeyMapping>& CurrentMappings = MappingContext->GetMappings();

    // There might be several keys bound to one action in one MappingContext
    // We need to get the exact binding from the target MappingContext
    const auto MappingLookup = [ActionToRebind, &KeyToUnmap](const FEnhancedActionKeyMapping& OneMapping)
    {
        return OneMapping.Action == ActionToRebind && OneMapping.Key == KeyToUnmap;
    };
    const FEnhancedActionKeyMapping* FoundMapping = CurrentMappings.FindByPredicate(MappingLookup);

    // Cache the properties of the key, being rebound in the target MappingContext
    return {FoundMapping->Triggers, FoundMapping->Modifiers};
}
```

### Option 2.2: Directly Changing the `FKey` by Reference

The second method is simpler and much safer: we locate the relevant mapping by its index and directly change its `Key` field to the desired value. All other mapping properties remain untouched.

There are several important points to keep in mind:

1. As seen in the parameters for our `UpdateKeyInContextByIndex()` function, it’s called using the data we’ve stored in `FKeyMappingPack`.

2. The function `GetMapping()` returns an `FEnhancedActionKeyMapping&`, which is a non-const reference to the mapping. If we forget to use `&` in the return type, we’ll get a copy of the structure - and any subsequent modifications will have no effect whatsoever.

3. Some sources claim that we should call `RequestRebuildControlMappingsUsingContext()` after making any changes to mappings. It’s true that the EIS does trigger such a rebuild internally when mappings are added or removed. However, based on my experience, calling this function has no noticeable effect: key reassignment works just as reliably without it - even in the project’s Shipping build.

4. The `UpdateKeyInContextByIndex()` function works equally well for assigning a custom key to a mapping or restoring its original value. To do the latter, simply pass `FKeyMappingPack::DefaultKey` as the `KeyToSet` parameter when calling the function.

```cpp
static void UpdateKeyInContextByIndex(UInputMappingContext* MappingContext, const int32 MappingIndex, const FKey& KeyToSet)
{
    // Restore default key in the respective mapping context
    FEnhancedActionKeyMapping& MappingToRestore = MappingContext->GetMapping(MappingIndex);
    MappingToRestore.Key = KeyToSet;

    UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(MappingContext);
}

//////////////////////////

UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
void RemapControlKey(UPARAM(ref) FKeyMappingPack& PackParam, const FKey& KeyToSet)
{
    // Do nothing, if we change no key
    if (PackParam.CustomKey == KeyToSet) return;

    UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, KeyToSet);

    UpdateCustomKeyInPackAndSettings(PackParam, KeyToSet);
}

//////////////////////////

UFUNCTION(BlueprintPure=false, Category = "Rebind Setting")
void RestoreDefaultKey(UPARAM(ref) FKeyMappingPack& PackParam)
{
    // Do nothing, if we change no key
    if (PackParam.CustomKey == PackParam.DefaultKey) return;

    UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, PackParam.DefaultKey);

    UpdateCustomKeyInPackAndSettings(PackParam, PackParam.DefaultKey);
}
```

5. As we've already discovered, Approach 2 modifies actual assets. Leaving such changes in place after testing your game in-editor is risky. Fortunately, the solution is straightforward: just roll back any changes made to mappings when the in-editor session ends. For instance, like this:

```cpp
#if WITH_EDITOR
void BeginDestroy() override
{
    Super::BeginDestroy();

    // Restore all mappings to default
    for (FKeyMappingPack& OnePack : StableKeyMappingPacks)
    {
        RestoreDefaultKey(OnePack);
    }
}
#endif // #if WITH_EDITOR
```

# References

1. https://dev.epicgames.com/community/learning/tutorials/Vp69/unreal-engine-player-mappable-keys-using-enhanced-input
2. https://zomgmoz.tv/unreal/Enhanced-Input-System/How-to-set-up-a-player-customizable-keybinding-system-using-Enhanced-Input
3. https://forums.unrealengine.com/t/enhanced-input-remapping-delete-triggers-modifiers-action-etc/1241165
4. https://forums.unrealengine.com/t/remapping-enhanced-input-actions-with-multiple-keys/739508
5. https://forums.unrealengine.com/t/enhanced-input-losing-modifiers-on-remap-c/1319341

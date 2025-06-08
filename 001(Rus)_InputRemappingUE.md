# UE Enhanced Input System: настраиваем управление в игре (до и после версии 5.3)

Мы рассмотрим два базовых подхода к созданию системы переназначения ввода в Unreal Engine 5 с плагином Enhanced Input System.
Для версий UE 5.3 и выше можно использовать встроенную систему настроек, а для более ранних версий - разработать собственную реализацию.

# Введение

В Unreal Engine 5 за работу с вводом отвечает плагин `Enhanced Input System (EIS)` - более мощный инструмент по сравнению с `Input System` в предыдущей версии движка.

В данный момент Enhanced Input System допускает по меньшей мере два подхода к созданию системы переназначения управления в игре.

- **Подход 1: Решение на основе встроенной логики**. Первый подход возможен в проектах, реализуемых на движке версии 5.3 или более поздних. Подход достаточно прост и основан на применении существующих в EIS инструментов.

- **Подход 2: Решение на основе кастомной логики**. Второй подход был актуален на движке до версии 5.2 включительно, и по большей части утратил актуальность к настоящему времени. Он тем не менее все еще может быть полезен для проектов на старых версиях движка.

## Рамки темы

Статья не претендует на статус исчерпывающего туториала. Скорее, она изложение моего опыта по созданию систем переназначения управления.

За кадром остались вопросы, индивидуальные для проектов: работа в блупринтах, сохранение настроек ввода в рамках Подхода 2, интеграция системы в пользовательские виджеты... Эти аспекты к теме статьи не относятся.

Цель статьи - снабдить базовыми сведениями тех, кто впервые столкнулся с необходимостью сделать систему переназначения управления на основе плагина Enhanced Input System.

# Основные классы EIS

В этом разделе мы кратко вспомним основные сущности Enhanced Input System, которые будем использовать при создании системы переназначения клавиш.

1) Класс `UInputMappingContext` лежит в основе Enhanced Input System. Контекст определяет активный набор вариантов ввода в зависимости от состояния игрового мира, режима игрока и т.д. К примеру, для выбора опций в меню, перемещения по уровню и выполнения действий в мини-игре нам нужны разные наборы клавиш. Более того, даже один режим игры может требовать разных контекстов, например, для клавамыши и для геймпада.

2) Каждый объект класса `UInputAction` отвечает за определенное действие игрока - прыжок, выстрел, использование способности - которое может быть связано с одним или несколькими физическими вводами (кнопками, осями, сенсорами). Этот класс не описывает, какая клавиша выполняет действие: это задача `UInputMappingContext`. `UInputAction` лишь задаёт обработку ввода, его тип, триггеры, модификаторы и т.д.

*(рис.1 файлы UInputAction и UInputMappingContext в Content Browser)*

![[EIS_DataAssetTypes.png]]

3) Класс `FKey` - абстрактное представление определенного ввода. Он инкапсулирует идентификатор ввода, который может быть кнопкой клавиатуры, мыши, осью геймпада, сенсорным вводом, жестом или специализированным устройством.

4) Задача `UInputMappingContext` - содержать пары `UInputAction`-`FKey`, которые, вместе с дополнительными параметрами объединены в объекты структуры `FEnhancedActionKeyMapping` (маппинги). Помимо собственно этой пары, маппинги несут в себе, среди прочего, объекты `UInputTrigger`, определяющие, при каких условиях действие активируется данным вводом, и объекты `UInputModifier`, которые как-либо меняют (фильтруют, масштабируют и т.д.) данные, получаемые от ввода.

![[EIS_FEnhancedActionKeyMapping.png]]

*(рис.2 записи FEnhancedActionKeyMapping в файле UInputMappingContext)*

5) Помимо общих для рассматриваемых ниже EIS-классов, библиотечная логика переназначения ввода и его сохранения основывается на ряде специфичных для первого подхода классов. Так, `UEnhancedInputUserSettings` управляет пользовательскими настройками ввода, в том числе, сохраняет их между запусками игры. Сохраненные настройки привязаны к профилю игрока - `UEnhancedPlayerMappableKeyProfile`. В рамках первого подхода, как и в большинстве игр, мы будет работать лишь с одним профилем.

# Подход 1: Переназначение в UE 5.3 и последующих версиях

Описываемый ниже подход я применил при подготовке системы ремаппинга в игре [Furnish Master](https://store.steampowered.com/app/2004080/Furnish_Master/).

![[EIS_FinalScreen.png]]

Итоговый код проекта достаточно плотно увязан с существующей архитектурой игры, так что здесь только фрагменты, использующие интересующие нас функции и классы. Методы, не имеющие отношения к системе ремаппинга, были исключены из примеров.

## Шаг 1. Активируем пользовательские настройки ввода

Первый подход в создании механизма переназначения клавиш полностью основан на использовании встроенной в плагин Enhanced Input System системы пользовательских настроек ввода. Потому в первом шаге нам требуется активировать эту систему.

Перейдем в `Project Settings` -> раздел `Engine` -> пункт `Enhanced Input` -> раздел `User Settings`.

Ставим галочку в пункте `Enable User Settings`. Если этот флаг не отметить, у нас не будет доступа к объекту `UEnhancedInputUserSettings` в рантайм, а указатели на него будут пустыми.

![[EIS_EnableUserSettings.png]]

*(рис. 3 Где найти `Enable User Settings` в `Project Settings`)*

## Шаг 2. Собираем нужные указатели

Большинство наших действий в рамках первого подхода связано c объектом пользовательских настроек и профилем игрока. Вот функция для получения указателя на оба этих объекта:

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

## Шаг 3. Активируем переназначаемость у Input Action

Чтобы система пользовательских настроек могла начать работать с управлением, которое у нас уже есть, нам нужно открыть доступ этой системы к файлам `Input Action`.

Действия ввода, которые мы желаем оставить недоступными для переназначения, оставляем без изменений.

Для всех остальных `Input Action` нам надо выполнить их привязку к настройкам ввода. Эту задачу мы можем выполнить двумя способами:

### Способ 3.1. Редактирование `UInputAction`

В `Content Browser` находим и открываем файл `Input Action`, клавиши которого мы хотим сделать переназначаемыми.

В разделе `User Settings` в графе `Player Mappable Key Settings` выставляем валидный класс настроек. В данный момент, по умолчанию там есть всего один такой класс, `Player Mappable Key Settings`.

![[EIS_Settings_in_UInputAction.png]]

*(рис.4 Настройки переназначения в `UInputAction`)*

Там же будет нелишним заполнить поля `Name` и `Display Name`. Первое поле это внутрисистемное название действия - переменная `FName MappingName` в классе `FPlayerKeyMapping`. Второе поле это внутриигровое, доступное для локализации название действия - переменная `FText DisplayName` в классе `FPlayerKeyMapping`.

Способ предполагает, что мы открываем для ремаппинга `UInputAction` как таковой во всех контекстах, где этот ассет используется.

>**Внимание**: Представленный способ на практике допустим, только когда у нас лишь одна `FKey` назначена для одного `UInputAction` в единственном `UInputMappingContext`.

Если же, к примеру, мы проведем регистрацию (см. далее, Шаг 4) двух контекстов с одним и тем же `UInputAction`, система настроек учтет существование только одного `FPlayerKeyMapping` с этим `UInputAction` из контекста, прошедшего регистрацию последним.

Дело в том, что все экшены, открытые данным способом, будут иметь одно имя, а при регистрации их контексты учитываться никак не будут. Потому и будет происходить затирание сформированных ранее маппингов с уже учтенными `UInputAction`.

Изучить логику регистрации `FPlayerKeyMapping` и понять, почему маппинги затирают друг друга, можно в функции `UEnhancedInputUserSettings::RegisterKeyMappingsToProfile()`.

### Способ 3.2. Редактирование `UInputMappingContext`

В `Content Browser` находим и открываем файл `Input Mapping Context`, все (или отдельные) клавиши которого мы желаем сделать переназначаемыми.

В интересующем нас маппинге находим пункт `Setting Behavior`. Здесь нам доступно несколько опций:

![[EIS_Settings_in_UInputMappingContext.png]]

*(рис.5 Настройки переназначения в `UInputMappingContext`)*

1) `InheritSettingsFromAction`: Как и явствует из названия, настройки, примененные в `UInputAction`, будут перенесены в данный маппинг. Это значение выставляется у маппингов внутри `Input Mapping Context` по умолчанию, потому и работает логика, описанная в Способе 3.1. Никак индивидуализировать такой маппинг мы не можем.

2) `OverrideSettings`: Эта опция позволяет (и даже требует) обеспечить для маппинга уникальное имя. Будь то переназначение движения классической раскладки WASD (сразу четыре клавиши для единичного `UInputAction` движения) или что-то более необычное - именно этот вариант нам будет полезен в большинстве случаев, когда мы решим конструировать систему настроек для своей игры.

3) `IgnoreSettings`: Необычная, но все же полезная в теории опция. С ней мы можем явно исключить маппинг из системы переназначения. Соответственно, все маппинги, основанные на одном `UInputAction` с активированными настройками, будут регистрироваться системой при обработке контекста, а маппинг с этой опцией проигнорируют.

## Шаг 4. Регистрируем контексты ввода

Все приготовления завершены, и мы можем приступить к написанию кода.

Сейчас надо указать, с какими объектами класса `UInputMappingContext` будет работать наша система пользовательских настроек.

Для этого соответствующие контексты требуется зарегистрировать через функцию `RegisterInputMappingContexts()` или вызываемую в ней `RegisterInputMappingContext()` в рамках класса `UEnhancedInputUserSettings`.

Например, так:

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

Метод `CollectContextsWithMappableKeys()` самописный и здесь не представлен. Он собирает из все маппинг-контексты, которые могут нам пригодиться в рамках `RegisterAllContextsToRemap()`.

Функции регистрации итерируют через все содержащиеся в контексте маппинги (`FPlayerKeyMapping`) и собирают те, у которых `Input Action` открыты для переназначения (см. ранее Шаг 3). Все остальные маппинги игнорируются.

Подходящие маппинги записываются в профиль маппинг-настроек (`UEnhancedPlayerMappableKeyProfile`), откуда мы и будем с ними работать.

Собранные нами и в дальнейшем изменяемые настройки сохраняются в папку `Saved` проекта. Соответственно, удаление папки `Saved` полностью удалит все профили со всеми настройками ввода.

Не стоит опасаться повторной регистрации ранее зарегистрированных контекстов: это никак не повлияет на состояние уже созданных `FPlayerKeyMapping`.

В сбросе созданных таким образом настроек ввода к исходным поможет `UEnhancedInputUserSettings::ResetKeyProfileToDefault()`:

```cpp
FGameplayTagContainer FailureReason;
UserSettings->ResetKeyProfileToDefault(UserSettings->GetCurrentKeyProfileIdentifier(), FailureReason);
```

## Шаг 5. Читаем текущее состояние настроек ввода

Далее создаем структуру, для хранения и использования данных ввода на уровне блупринтов.

Наша структура `FKeyMappingPack` во многом похожа на системную структуру `FPlayerKeyMapping`, но содержит только поля и методы, которые потребуются нам в дальнейшем.

```cpp
USTRUCT(BlueprintType, Blueprintable, Category = "Rebind Settings")
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

Для непосредственной работы с настройками ввода нам придется собрать данные об этих настройках из профиля. Затем можно передать их в блупринт для чтения и вывода в меню настроек ввода, как и использовать там для непосредственного переназначения.

В функции `IteratePlayerKeyMappings()` мы обходим маппинги, содержащиеся в профиле, и выполняем с каждым определённую операцию, задаваемую объектом `Callback`.

Функция `GetMappingPacksForControlMode()` создает лямбду-функцию для фильтрации маппингов по типу ввода (аспект, обусловленный системой управления в Furnish Master). Функция `IteratePlayerKeyMappings()` принимает лямбду и возвращает собранный массив.

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

UFUNCTION(BlueprintPure=false, Category = "Rebind Settings")
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

## Шаг 6. Переназначаем ввод и сохраняем его

Все данные, необходимые для работы с настройками, мы собрали. Остался последний шаг.

Для непосредственного переназначения клавиш мы используем уже существующую функцию `MapPlayerKey()`.

Первым параметром функция принимает объект `FMapPlayerKeyArgs`, в который нам нужно предварительно записать имя `Input Action`, `FKey` ввода, который мы назначаем для этого маппинга, и `EPlayerMappableKeySlot` маппинга, из допущения, что на одно действие в рамках одного контекста может быть назначено несколько вариантов ввода.

Для фиксации смены ввода в профиле вызываем существующую функцию `SaveSettings()`.

Сразу после завершения функции новая конфигурация сохраняется и будет оставаться в таком состоянии даже при следующем запуске игры. Сохранение настроек выполняем сам плагин EIS.

```cpp
UFUNCTION(BlueprintPure=false, Category = "Rebind Settings")
void RemapControlKey(FKeyMappingPack& PackParam, const FKey& KeyToSet) const
{
    MapNewKeyInSettings(PackParam, KeyToSet);
}

//////////////////////////

UFUNCTION(BlueprintPure=false, Category = "Rebind Settings")
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

# Подход 2: Переназначение до UE 5.3

## Шаг 1. Отмечаем инпуты для маппинга

Начало реализации этого "кастомного" подхода полностью повторяет шаг 3 предыдущего раздела.

![[EIS_Player Mappable Key Settings (UE 5.2).png]]

В настройках объектов `Input Action` в графе `Player Mappable Key Settings` выставляем любой валидный класс (по умолчанию, там доступен лишь один).

Если нам нужны уникальные названия для маппингов с одним и тем же `Input Action`, выставляем валидные классы у маппингов в объектах `UInputMappingContext` - все, как было описано ранее.

## Шаг 2. Создаем структуру для переноса данных

В нашей системе переназначения клавиш нам нужна структура для переноса данных о дефолтных и ныне действующих маппингах, аналогичная той, что описана в шаге 5 первого подхода.

Структура `FKeyMappingPack` функционально схожа с `FPlayerKeyMapping`, но содержит только поля, которые нам понадобятся, включая, что немаловажно, указатель на `UInputMappingContext`.

```cpp
USTRUCT(BlueprintType, Blueprintable, Category = "Rebind Settings")
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

## Шаг 3. Собираем маппинги

Данный шаг схож с регистрацией инпут-контекстов, проведенной нами в шаге 4 первого подхода, с тем отличием, что мы выполняем эту "регистрацию" самостоятельно и сохраняем вывод у себя.

Наша задача здесь - извлечь редактируемые пары "действие-ключ" из доступных инпут-контекстов, а конкретно, из содержащихся там `FEnhancedActionKeyMapping`.

Помимо `UInputAction` и `FKey` нам будут полезны и иные данные, вроде индекса маппинга.

Функция `CollectNewControlSettings()` может казаться массивной на первый взгляд, но в конечном итоге она просто собирает данные маппингов и упаковывает в массив объектов `FKeyMappingPack`.

```cpp
UPROPERTY()
TArray<FKeyMappingPack> StableKeyMappingPacks;

//////////////////////////

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

## Шаг 4. Переназначаем клавиши

Нам осталось поменять конкретную клавишу у определенного маппинга в рамках заданного контекста. Эту задачу мы можем решить двумя путями.

> **Внимание**! Описанные в обоих вариантах действия - это прямая манипуляция файлами контекстов (`UInputMappingContext`) в папке `Content` проекта.
> Изменения, внесенные в контексты во время игры из редактора, останутся там даже после окончания игровой сессии!

### Вариант 2.1: Полное удаление и пересоздание маппинга

В рамках первого способа нам надо убрать из контекста исходный маппинг и на его месте создать новый маппинг.

Для этих задач мы используем существующие библиотечные функции `UInputMappingContext::UnmapKey()` и `UInputMappingContext::MapKey()`.

Как мы видим из кода ниже, помимо замены одной кнопки на другую нам требуется еще обеспечить перенос, по крайней мере, триггеров и модификаторов из удаляемого маппинга в новосозданный.

Между тем, настройку остальных полей, в частности, `SettingBehavior` и `PlayerMappableKeySettings` мы также легко выполнить не можем из-за их `protected` статуса. Как следствие, они инициализируются данными по умолчанию.

Хотя это может показаться контринтуитивным, но на моей практике создание и удаление маппингов при таком подходе никак не влияло на действительность их индексов, сохраненных в `FKeyMappingPack`. Новый маппинг в дальнейшем был доступен по тому же номеру, что и удаленный маппинг, хотя визуально положение таких маппингов внутри `UInputMappingContext` менялось. Лучше рассматривать это как случайность и не полагаться на нее.

В целом, я не рекомендую данный подход и привожу его здесь лишь для полноты изложения и как предостережение от подобных ошибок.

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

### Вариант 2.2: Простая смена `FKey` по ссылке на маппинг

Механизм второго подхода проще и безопаснее - мы находим по индексу нужный нам маппинг и меняем значение его поля `Key` на нужное нам. Все остальные части маппинга остаются нетронутыми.

Есть несколько аспектов, о которых следует помнить:

1) Как мы видим из списка параметров нашей функции `UpdateKeyInContextByIndex()`, она вызывается с данными, которые мы собрали в `FKeyMappingPack`.

2) Функция `GetMapping()` возвращает `FEnhancedActionKeyMapping&`, то есть неконстантную ссылку на маппинг. Если мы забудем поставить `&` в нужном месте, произойдет копирование структуры и дальнейшая операция потеряет всякий смысл.

3) Некоторые источники утверждают, что после всякого изменения маппингов следует запросить их пересборку (`RequestRebuildControlMappingsUsingContext()`). Действительно, система производит такую пересборку, в частности, при добавлении и удалении маппингов. Между тем, исходя из моей практики, вызов этой функции ни на что не влияет: с ней маппинги переназначаются столь же надежно, как и без нее даже в Shipping сборке проекта.

4) Функция `UpdateKeyInContextByIndex()` отлично подходит как для назначения в маппинг собственной клавиши, так и для восстановления исходной. Для последнего при вызове в параметр `KeyToSet` достаточно передать `FKeyMappingPack::DefaultKey`.

```cpp
static void UpdateKeyInContextByIndex(UInputMappingContext* MappingContext, const int32 MappingIndex, const FKey& KeyToSet)
{
    // Restore default key in the respective mapping context
    FEnhancedActionKeyMapping& MappingToRestore = MappingContext->GetMapping(MappingIndex);
    MappingToRestore.Key = KeyToSet;

    UEnhancedInputLibrary::RequestRebuildControlMappingsUsingContext(MappingContext);
}

//////////////////////////

UFUNCTION(BlueprintCallable, Category = "Rebind Settings")
void RemapControlKey(UPARAM(ref) FKeyMappingPack& PackParam, const FKey& KeyToSet)
{
    // Do nothing, if we change no key
    if (PackParam.CustomKey == KeyToSet) return;

    UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, KeyToSet);

    UpdateCustomKeyInPackAndSettings(PackParam, KeyToSet);
}

//////////////////////////

UFUNCTION(BlueprintCallable, Category = "Rebind Settings")
void RestoreDefaultKey(UPARAM(ref) FKeyMappingPack& PackParam)
{
    // Do nothing, if we change no key
    if (PackParam.CustomKey == PackParam.DefaultKey) return;

    UpdateKeyInContextByIndex(PackParam.MappingContext, PackParam.MappingIndex, PackParam.DefaultKey);

    UpdateCustomKeyInPackAndSettings(PackParam, PackParam.DefaultKey);
}
```

5) Как мы выяснили ранее, Подход 2 предполагает изменение ассетов. Оставлять это в процессе тестирования игры в редакторе просто так опасно. Решение лежит на поверхности. Достаточно откатывать произведенные изменения для каждого маппинга, когда игра в редакторе завершается. Например, так:

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

# Заключение

Итак, мы рассмотрели два подхода к созданию системы переназначения управления в игре на основе плагина Enhanced Input System.

В целом, первый подход, предполагающий использование готового функционала EIS, значительно более эффективен и безопасен, нежели второй подход, и именно его рекомендуется по возможности придерживаться.

# Источники

1. https://dev.epicgames.com/community/learning/tutorials/Vp69/unreal-engine-player-mappable-keys-using-enhanced-input
2. https://zomgmoz.tv/unreal/Enhanced-Input-System/How-to-set-up-a-player-customizable-keybinding-system-using-Enhanced-Input
3. https://forums.unrealengine.com/t/enhanced-input-remapping-delete-triggers-modifiers-action-etc/1241165
4. https://forums.unrealengine.com/t/remapping-enhanced-input-actions-with-multiple-keys/739508
5. https://forums.unrealengine.com/t/enhanced-input-losing-modifiers-on-remap-c/1319341

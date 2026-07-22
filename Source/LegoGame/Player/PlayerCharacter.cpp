// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LegoGame/LegoGame.h"
#include "LegoGame/Components/PackageComponent.h"
#include "LegoGame/GamePlay/GameMenu/KeySettingUserWidget.h"
#include "LegoGame/GamePlay/MainGame/LgHUD.h"
#include "LegoGame/SaveGame/CustomKeySaveGame.h"
#include "LegoGame/Scene/SceneItemActor.h"
#include "LegoGame/Settings/LgGameUserSettings.h"

#define INSERT_ACTION_KEY(KeyEventName) if (DT_KeyMapping->GetRowMap().Contains(KeyEventName))\
		{\
			FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*>(DT_KeyMapping->GetRowMap()[KeyEventName]);\
			FKey Key = GetUserCustomKey(KeyEventName);\
			if (!Key.IsValid())\
			{\
				Key = KeyInfoHeader->Key;\
			}\
			IMC_Player->MapKey(KeyInfoHeader->InputAction,Key);\
		}

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;//开启挂臂跟随相机控制器旋转
	//SpringArmComponent->bDoCollisionTest = false;
	CameraComponent->SetupAttachment(SpringArmComponent);
	
	SphereComponent->SetupAttachment(RootComponent);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAll"));
	SphereComponent->SetSphereRadius(150.f);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetUpPlayerInputMappingContext();
	
	//绑定sphere的代理通知
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this,&ThisClass::OnComponentBeginOverlapEvent);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this,&ThisClass::OnComponentEndOverlapEvent);
}

void APlayerCharacter::SetUpPlayerInputMappingContext()
{
	//将输入关系配置给当前角色使用，让引擎知道响应对象是谁
	//Character是玩家在虚拟世界的抽象皮囊，他不是真正的角色
	if (APlayerController* Pc = Cast<APlayerController>(GetController()))
	{
		//Pc->GetLocalPlayer();//这个才是虚幻引擎中抽象的玩家角色
		//获取增强输入子系统
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Pc->GetLocalPlayer());
		//告知引擎的输入系统哪一个映射会被使用
		if (!IMC_Player)
		{
			IMC_Player = LoadObject<UInputMappingContext>(this,TEXT("/Script/EnhancedInput.InputMappingContext'/Game/LegoGame/EnhancedInput/Mappings/IMC_Player.IMC_Player'"));
		}
		InputSubsystem->AddMappingContext(IMC_Player,0);
		//添加一个断言
		ensure(IMC_Player);//如果是空指针，输出错误日志，并终止程序执行
		//清理映射表格数
		IMC_Player->UnmapAll();
		//根据表格动态创建映射关系
		if (!DT_KeyMapping)
		{
			DT_KeyMapping = LoadObject<UDataTable>(this,TEXT("/Script/Engine.DataTable'/Game/LegoGame/Data/DT_KeyMapping.DT_KeyMapping'"));
		}
		//读取按键自定义存档
		if (UGameplayStatics::DoesSaveGameExist(CUSTOM_KEY_SLOT,0))
		{
			CustomKeySaveGame = Cast<UCustomKeySaveGame>(UGameplayStatics::LoadGameFromSlot(CUSTOM_KEY_SLOT,0));
		}
		 // if (DT_KeyMapping->GetRowMap().Contains(TEXT("Jump")))
		 // {
		 // 	FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*>(DT_KeyMapping->GetRowMap()[TEXT("Jump")]);
		 // 	//查一下有没有该按键
		 // 	FKey Key = GetUserCustomKey(TEXT("Jump"));
		 // 	if (!Key.IsValid())
		 // 	{
		 // 		Key = KeyInfoHeader->Key;
		 // 	}
			// IMC_Player->MapKey(KeyInfoHeader->InputAction,Key);
		 // }
		
		//动态注入输入映射上下文
		INSERT_ACTION_KEY("Jump");
		INSERT_ACTION_KEY("Fire");
		INSERT_ACTION_KEY("Look");
		INSERT_ACTION_KEY("Sprint");
		INSERT_ACTION_KEY("Crouch");
		INSERT_ACTION_KEY("Package");
		INSERT_ACTION_KEY("IronSight");
		INSERT_ACTION_KEY("Reload");
		
		
		
		//添加轴映射关系
		AddAxisActionKey(TEXT("MoveForward"),EAxis::Y);
		AddAxisActionKey(TEXT("MoveBack"),EAxis::Y,true);
		AddAxisActionKey(TEXT("MoveRight"),EAxis::X);
		AddAxisActionKey(TEXT("MoveLeft"),EAxis::X,true);
	}
}

FKey APlayerCharacter::GetUserCustomKey(FName KeyEventName)
{
	FKey Key;
	if (CustomKeySaveGame&&CustomKeySaveGame->CustomKey.Contains(KeyEventName))
	{
		Key = CustomKeySaveGame->CustomKey[KeyEventName];
	}
	return Key;
}

UInputAction* APlayerCharacter::GetInputAction(FName KeyEventName)
{
	if (!DT_KeyMapping)
	{
		DT_KeyMapping = LoadObject<UDataTable>(this,TEXT("/Script/Engine.DataTable'/Game/LegoGame/Data/DT_KeyMapping.DT_KeyMapping'"));
	}
	if (DT_KeyMapping&&DT_KeyMapping->GetRowMap().Contains(KeyEventName))
	{
		FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*>(DT_KeyMapping->GetRowMap()[KeyEventName]);
		return KeyInfoHeader->InputAction;
	}
	return nullptr;
}

void APlayerCharacter::Move(FInputActionValue const& InputActionValue)
{
	// FVector2D AxisValue = InputActionValue.Get<FVector2D>();
	// UE_LOG(LogTemp, Warning, TEXT("%s"),*AxisValue.ToString());
	// AddMovementInput(GetActorForwardVector(),AxisValue.Y);
	// AddMovementInput(GetActorRightVector(),AxisValue.X);
	FVector2D AxisValue = InputActionValue.Get<FVector2D>();
    
	// 确保控制器存在
	if (Controller != nullptr)
	{
		// 1. 获取控制器的旋转方向（这通常就是摄像机的朝向）
		const FRotator Rotation = Controller->GetControlRotation();
        
		// 2. 关键：抹去俯仰角（Pitch）和横滚角（Roll），只保留偏航角（Yaw）
		// 这样可以防止玩家在“看天”或“看地”时，按 W 键导致角色往天上飞或往地下钻
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
		// 3. 计算基于相机偏航角的前方和右方向量
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		// 4. 将输入值应用到这两个新的方向上
		AddMovementInput(ForwardDirection, AxisValue.Y);
		AddMovementInput(RightDirection, AxisValue.X);
		
		//飞行，游泳视角使用四元数表达式
		// FQuat ControllerQuat = Controller->GetControlRotation().Quaternion();
		// AddMovementInput(ControllerQuat.GetAxisX(), AxisValue.Y);
		// AddMovementInput(ControllerQuat.GetRightVector(), AxisValue.X);
	}
}

void APlayerCharacter::AddAxisActionKey(FName KeyEventName, EAxis::Type Axis, bool bNegate)
{
	if (DT_KeyMapping->GetRowMap().Contains(KeyEventName))
	{
		FKsyInfoHeader* KeyInfoHeader = reinterpret_cast<FKsyInfoHeader*>(DT_KeyMapping->GetRowMap()[KeyEventName]);
		//查一下有没有该按键
		FKey Key = GetUserCustomKey(KeyEventName);
		if (!Key.IsValid())
		{
			Key = KeyInfoHeader->Key;
		}
		FEnhancedActionKeyMapping& KeyMapping = IMC_Player->MapKey(KeyInfoHeader->InputAction,Key);
		//添加一个modifer
		if (bNegate)
		{
			KeyMapping.Modifiers.Add(NewObject<UInputModifierNegate>(IMC_Player));//如何构建一个U类指针对象
		}
		if (Axis==EAxis::Y)
		{
			KeyMapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(IMC_Player));//加反转轴
			
		}
	}
}

void APlayerCharacter::Look(FInputActionValue const& InputActionValue)
{
	//因为绑定的是Trigger响应，所有此函数每一个逻辑帧都会被调用，性能不同电脑调用次数不同*GetWorld()->GetDeltaSeconds()
	FVector2D AxisValue = InputActionValue.Get<FVector2D>();
	//UE_LOG(LogTemp, Warning, TEXT("%s"),*AxisValue.ToString());
	
	//读取配置信息获取鼠标速度
	if (MouseSpeed == 0)
	{
		if (ULgGameUserSettings* GameUserSettings = Cast<ULgGameUserSettings>(UGameUserSettings::GetGameUserSettings()))
		{
			MouseSpeed = GameUserSettings->GetMouseSpeedRate();
		}else
		{
			
			MouseSpeed = 40.f;
		}
	}
	
	//将输入鼠标的偏移添加到相机的偏移上
	AddControllerPitchInput(AxisValue.Y *GetWorld()->GetDeltaSeconds()*MouseSpeed*-1);
	AddControllerYawInput(AxisValue.X *GetWorld()->GetDeltaSeconds()*MouseSpeed);
}

void APlayerCharacter::TogglePackageUI()
{
	//执行显示或关闭背包ui
	if (APlayerController* Pc = Cast<APlayerController>(GetController()))
	{
		if (ALgHUD* HUD = Cast<ALgHUD>(Pc->GetHUD()))
		{
			HUD->TogglePackageUI();
		}
	}
}

void APlayerCharacter::OnComponentBeginOverlapEvent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// if (OtherActor)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Enter %s"),*OtherActor->GetName());
	// }
	if (PackageComponent)
	{
		if (ASceneItemActor* SceneItemActor = Cast<ASceneItemActor>(OtherActor))
		{
			PackageComponent->AddNearSceneItem(SceneItemActor);
		}
	}
}

void APlayerCharacter::OnComponentEndOverlapEvent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// if (OtherActor)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Leave %s"),*OtherActor->GetName());
	// }
	if (PackageComponent)
	{
		if (ASceneItemActor* SceneItemActor = Cast<ASceneItemActor>(OtherActor))
		{
			PackageComponent->RemoveNearSceneItem(SceneItemActor);
		}
	}
}


// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	//设置Action触发后关联哪些逻辑
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//绑定Action
		if (UInputAction* InputAction = GetInputAction("Jump"))
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::Jump);
		}
		if (UInputAction* InputAction = GetInputAction("Reload"))
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::ReloadWeapon);
		}
		if (UInputAction* InputAction = GetInputAction("Fire"))
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::StartFire);
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Completed,this,&ThisClass::StopFire);
		}
		if (UInputAction* InputAction = GetInputAction("IronSight"))
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::StartIronSight);
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Completed,this,&ThisClass::StopIronSight);
		}
		if (UInputAction* InputAction = GetInputAction("Sprint"))
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::StartSprint);
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Completed,this,&ThisClass::StopSprint);
		}
		if (UInputAction* InputAction = GetInputAction("MoveForward"))//只用绑定一个
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Triggered,this,&ThisClass::Move);
		}
		if (UInputAction* InputAction = GetInputAction("Look"))//只用绑定一个
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
		}
		if (UInputAction* InputAction = GetInputAction("Crouch"))//只用绑定一个
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::DoCrouch);
		}
		if (UInputAction* InputAction = GetInputAction("Package"))//只用绑定一个
		{
			EnhancedInputComponent->BindAction(InputAction,ETriggerEvent::Started,this,&ThisClass::TogglePackageUI);
		}
	}
}


#undef INSERT_ACTION_KEY

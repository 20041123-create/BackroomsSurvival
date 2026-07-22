// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CustomKeySaveGame.generated.h"

/**
 * 存储玩家自定义key数据
 * 存档数据类型必须是虚幻引擎支持的序列化类型
 * 结构型数据：FVector,FRotator,FString,FName
 * 类结构指针：无法直接存储指针，因为指针只是一个地址，引擎不会帮你遍历地址存储
 * 结构容器：TArray,TMap,可以存储，但是必须保证是上述允许类型
 */


UCLASS()
class LEGOGAME_API UCustomKeySaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FName,FKey> CustomKey;
	
};

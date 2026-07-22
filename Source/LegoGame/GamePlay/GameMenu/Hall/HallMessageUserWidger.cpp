// Fill out your copyright notice in the Description page of Project Settings.


#include "HallMessageUserWidger.h"

#include "Components/TextBlock.h"

void UHallMessageUserWidger::ShowMessage(FText Title, FText Message)
{
	TitleTextBlock->SetText(Title);
	MessageTextBlock->SetText(Message);
	
	AddToViewport();
}

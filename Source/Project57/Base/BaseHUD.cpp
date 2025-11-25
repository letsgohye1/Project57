// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseHUD.h"
#include "Engine/Canvas.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

/*
화면 중앙 좌표를 계산한다.

HUD를 그리기 위한 기본 단위(Unit)을 화면 해상도 기준으로 잡는다.

현재 캐릭터 속도와 최대 이동속도를 가져온다.

현재 속도를 비율로 계산해서 조준선이 얼마나 벌어질지 계산한다.

그 값을 이용해
조준선의 4개 선(좌/우/상/하)을 화면 중앙에서 바깥쪽으로 그린다.

속도가 빠르면 멀어지고, 멈추면 다시 좁아진다.*/


void ABaseHUD::DrawHUD()
{
	Super::DrawHUD();

	//HUD 크기 기준 단위값. 화면 가로 해상도를 100등분
	int32 Unit = Canvas->SizeX / 100;
	
	int32 CenterX = Canvas->SizeX / 2;
	int32 CenterY = Canvas->SizeY / 2;
	int32 DrawSize = Unit * 2;

	float CurrentSpeed = 0.f; //캐릭터의 현재 이동속도
	float MaxSpeed = 0.0f;	 //캐릭터가 낼 수 있는 최대 속도
	float GapRatio = 0.0f;  //현재속도 / 최대속도
	int32 Gap = Unit * 3;	//조준선 기본 간격

	//현재 HUD를 소유한 캐릭터(Pawn)를 가져온다.
	ACharacter* Pawn = Cast<ACharacter>(GetOwningPawn());

	if (Pawn) //캐릭터가 존재한다면,
	{
		MaxSpeed = Pawn->GetCharacterMovement()->GetMaxSpeed();			//최대 이동속도(MaxSpeed) 와
		CurrentSpeed = Pawn->GetCharacterMovement()->Velocity.Size2D(); //현재 속도(CurrentSpeed) 를 가져온다.
		GapRatio = CurrentSpeed / MaxSpeed; //현재 속도 비율		예) 속도가 최대의 절반이면 0.5
	}
	
	Gap = (int32)((float)Gap * GapRatio);
	//조준선 벌어짐 정도를 속도 비율로 계산.
	//Gap = Gap * GapRatio;


	//Draw2DLine(X1, Y1, X2, Y2, Color)
	//→ 화면 좌표(X1, Y1) 에서(X2, Y2) 까지 Color 색상의 직선을 그린다.
	Draw2DLine(CenterX - Unit - Gap, 
		CenterY,
		CenterX - Gap,
		CenterY,
		FColor::Red);

	Draw2DLine(CenterX + Gap,
		CenterY,
		CenterX + Unit + Gap,
		CenterY,
		FColor::Red);

	Draw2DLine(CenterX,
		CenterY - Unit - Gap,
		CenterX,
		CenterY - Gap,
		FColor::Red);

	Draw2DLine(CenterX,
		CenterY + Gap,
		CenterX,
		CenterY + Unit + Gap,
		FColor::Red);
}


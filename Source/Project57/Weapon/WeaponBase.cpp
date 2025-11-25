// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BaseDamageType.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"



// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


//Reload(): 탄약을 최대치로 채우고 로그 출력.
void AWeaponBase::Reload()
{
	CurrentBulletCount = MaxBulletCount;
	UE_LOG(LogTemp, Warning, TEXT("Reload %d"), CurrentBulletCount);
}


/*발사 처리
(발사 속도 검사, 
자동 발사 타이머 등록, 
화면 중심을 기반으로 한 선형 트레이스, 
히트 시 데미지 적용, 
탄약 감소, 
사운드 재생, 
발사 시각 갱신).*/
void AWeaponBase::Fire()
{
	float CurrentTimeofShoot = GetWorld()->TimeSeconds - TimeofLastShoot; //TimeofLastShoot로부터 지난 시간

	//1) 연속발사(발사 간격) 검사
	if (CurrentTimeofShoot < RefireRate) //CurrentTimeofShoot이 RefireRate보다 작으면 발사 무시(재발사 전 대기).
	{
		return;
	}

	//2) 풀오토 처리 - 타이머 설정
	if (bFullAuto) //풀오토면 RefireTimer를 설정해 RefireRate 후 Fire()를 다시 호출.
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AWeaponBase::Fire, RefireRate, false);
	}

	//3) 소유자(캐릭터) 및 컨트롤러 얻기
	ACharacter* Character = Cast<ACharacter>(GetOwner());

	ensure(Character);
	//check(Character);
	if (!Character)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());

	if (PC)
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		int32 CenterX = 0;
		int32 CenterY = 0;
		FVector WorldDirection;
		FVector WorldLocation;
		FVector CameraLocation;
		FRotator CameraRotation;

		//4) 화면 중심 → 월드 방향으로 변환(Deproject)
		PC->GetViewportSize(SizeX, SizeY);
		CenterX = SizeX / 2;
		CenterY = SizeY / 2;

		//화면 중심 픽셀을 월드 위치(WorldLocation)와 방향(WorldDirection)으로 변환.
		PC->DeprojectScreenPositionToWorld((float)CenterX, (float)CenterY,WorldLocation, WorldDirection);

		//그 후 GetPlayerViewPoint로 카메라 위치/회전 얻음.
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

		//Start를 카메라 위치로 하고 End를 카메라 + 방향 * 큰 거리로 설정 → 카메라에서 쭉 뻗는 레이.
		FVector Start = CameraLocation;
		FVector End = CameraLocation + WorldDirection * 100000.0f;


		//5) 라인 트레이스(히트 검사)
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

		//LineTraceSingleForObjects 를 사용해서 충돌 객체 타입 목록(ObjectTypes)에 해당하는 오브젝트와 충돌 검사.
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_PhysicsBody));

		TArray<AActor*> IngnoreActors;
		FHitResult HitResult;

		bool bResult = UKismetSystemLibrary::LineTraceSingleForObjects(
			GetWorld(),
			Start,
			End,
			ObjectTypes,
			true,
			IngnoreActors,
			EDrawDebugTrace::ForDuration, //디버그 라인 시각화(게임 실행 중 보임)
			HitResult,
			true
		);


		// 6) 히트 시 데미지 적용
		if (bResult)
		{
			//RPG 
			//UGameplayStatics::ApplyDamage(HitResult.GetActor(),
			//	50,
			//	PC,
			//	this,
			//	UBaseDamageType::StaticClass()
			//);

			//총쏘는 데미지
			UGameplayStatics::ApplyPointDamage(
				HitResult.GetActor(),			//히트된 액터에 ApplyPointDamage 호출:
				10,								//데미지 10
				-HitResult.ImpactNormal,		//ShotDirection 으로 -HitResult.ImpactNormal 사용(이건 충돌면의 법선의 반대방향)
				HitResult,
				PC,								//EventInstigator로 PC 전달(OK)
				this,							//DamageCauser로 this (무기 액터)
				UBaseDamageType::StaticClass()	//DageType으로 UBaseDamageType
			);

			////범위 공격, 폭탄
			//UGameplayStatics::ApplyRadialDamage(HitResult.GetActor(),
			//	10,
			//	HitResult.ImpactPoint,
			//	300.0f,
			//	UBaseDamageType::StaticClass(),
			//	IngnoreActors,
			//	this,
			//	PC,
			//	true
			//);
		}

	}

	//7) 탄약감소·사운드·타임스탬프
	CurrentBulletCount--;
	UE_LOG(LogTemp, Warning, TEXT("Fire %d"), CurrentBulletCount);
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), FireSound, GetActorLocation());

	//마지막 발사 시각 갱신
	TimeofLastShoot = GetWorld()->TimeSeconds;

}

void AWeaponBase::FireProjectile()
{
}

//자동사격 타이머 취소.
void AWeaponBase::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}


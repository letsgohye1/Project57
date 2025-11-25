ㄹ// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
#include "GameframeWork/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "../Weapon/WeaponBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "../Weapon/BaseDamageType.h"
#include "Engine/DamageEvents.h"



// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	GetMesh()->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));

	GetMesh()->SetRelativeRotation(FRotator(0, -90.0f, 0));

	Weapon = CreateDefaultSubobject<UChildActorComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh());

}

// Called when the game starts or when spawned
//BeginPlay() – 무기 실제 장착 처리
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	//무기 집으면 잡게 이동
	AWeaponBase* ChildWeapon = Cast<AWeaponBase>(Weapon->GetChildActor()); //ChildActorComponent 안에 있는 무기를 가져옴
	if (ChildWeapon)
	{
		ChildWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, ChildWeapon->SocketName);//캐릭터 메쉬의 손 소켓 등에 실제 장착
		WeaponState = EWeaponState::Pistol; //현재 무기 상태 설정
		ChildWeapon->SetOwner(this); //데미지를 줄 때 공격 주체 표시용
	}

}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//4. 입력 바인딩
// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* UIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (UIC)
	{
		//재장전 키 입력
		UIC->BindAction(IA_Reload, ETriggerEvent::Completed, this,&ABaseCharacter::Reload);

		//공격 키 누르면 발사
		UIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &ABaseCharacter::StartFire);

		//떼면 멈춤
		UIC->BindAction(IA_Fire, ETriggerEvent::Completed, this,&ABaseCharacter::StopFire);
	}

}

void ABaseCharacter::Move(float Forward, float Right)
{
	const FRotator CameraRotation =  GetController()->GetControlRotation();
	const FRotator YawRotation = FRotator(0, CameraRotation.Yaw, 0);
	const FRotator YawRollRotation = FRotator(0, CameraRotation.Yaw, CameraRotation.Roll);


	const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(YawRotation);
	AddMovementInput(ForwardVector, Forward);

	const FVector RightVector = UKismetMathLibrary::GetRightVector(YawRollRotation);
	AddMovementInput(RightVector, Right);


//	AddMovementInput(FVector(Forward, Right, 0));
}

void ABaseCharacter::Look(float Pitch, float Yaw)
{
	AddControllerPitchInput(Pitch);
	AddControllerYawInput(Yaw);
}

//🔷 7. 무기 동작
void ABaseCharacter::Reload()
{
	AWeaponBase* ChildWeapon = Cast<AWeaponBase>(Weapon->GetChildActor());
	if (ChildWeapon)
	{
		//무기가 들고 있는 재장전 몽타주 실행
		PlayAnimMontage(ChildWeapon->ReloadMontage);
	}
}

void ABaseCharacter::DoFire()
{
	AWeaponBase* ChildWeapon = Cast<AWeaponBase>(Weapon->GetChildActor());
	if (ChildWeapon)
	{
		ChildWeapon->Fire(); //실제 무기 Fire() 호출
	}
}

void ABaseCharacter::StartFire()
{
	bIsFire = true;
	DoFire();
}

void ABaseCharacter::StopFire()
{
	bIsFire = false;
	AWeaponBase* ChildWeapon = Cast<AWeaponBase>(Weapon->GetChildActor());
	if (ChildWeapon)
	{
		ChildWeapon->StopFire();
	}
}

//피격 시 피격 애니메이션(몽타주)에서 랜덤 섹션을 골라서 재생한다. 여러 종류의 피격 모션을 섞어서 자연스럽게 보이게 함.
void ABaseCharacter::HitReaction()
{
	FString SectionName = FString::Printf(TEXT("%d"), FMath::RandRange(1, 8));

	PlayAnimMontage(HitMontage, 1.0, FName(*SectionName) );
}

void ABaseCharacter::ReloadWeapon()
{
	AWeaponBase* ChildWeapon = Cast<AWeaponBase>(Weapon->GetChildActor());
	if (ChildWeapon)
	{
		ChildWeapon->Reload();
	}
}


//언리얼의 데미지 이벤트를 받아서 HP를 깎고 피격/사망 처리를 수행한다.
float ABaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (CurrentHP <= 0)
	{
		return DamageAmount;
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent* Event = (FPointDamageEvent*)(&DamageEvent); //(총알 같은 포인트 데미지)
		if (Event)
		{
			CurrentHP -= DamageAmount;

			 UE_LOG(LogTemp, Warning, TEXT("Point Damage %f %s"), DamageAmount, *(Event->HitInfo.BoneName.ToString()));
		}
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent* Event = (FRadialDamageEvent*)(&DamageEvent);//(폭발 같은 범위 데미지)
		if (Event)
		{
			CurrentHP -= DamageAmount;

			UE_LOG(LogTemp, Warning, TEXT("Radial Damage %f %s"), DamageAmount, *Event->DamageTypeClass->GetName());
		}
	}
	else //(DamageEvent.IsOfType(FDamageEvent::ClassID))
	{
		CurrentHP -= DamageAmount;
		UE_LOG(LogTemp, Warning, TEXT("Damage %f"), DamageAmount);
	}

	DoHitReact(); //DoHitReact() 호출하여 피격 애니메이션 재생



	if (CurrentHP <= 0)
	{
		//죽는다. 애님 몽타주 재생
		//네트워크 할려면 다 RPC로 작업해 됨
		DoDead();
	}

	return DamageAmount;
}

void ABaseCharacter::DoDeadEnd()//사망 처리 완료 후 물리(래그돌)로 전환해 자연스럽게 쓰러지게 함.
{
	GetController()->SetActorEnableCollision(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
}

void ABaseCharacter::DoDead() //사망 몽타주(여러 섹션 중 랜덤) 재생.
{
	FName SectionName = FName(FString::Printf(TEXT("%d"), FMath::RandRange(1, 6)));
	PlayAnimMontage(DeathMontage, 1.0f, SectionName);
}

void ABaseCharacter::DoHitReact()
{
	FName SectionName = FName(FString::Printf(TEXT("%d"), FMath::RandRange(1, 8)));
	PlayAnimMontage(HitMontage, 1.0f, SectionName);
}

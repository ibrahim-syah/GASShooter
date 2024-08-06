// Copyright 2020 Dan Kestranek.


#include "UI/GSHUDDamageIndicator.h"
#include "Kismet/KismetMathLibrary.h"

float UGSHUDDamageIndicator::UpdateAngle(FVector ForwardVector, FVector ActorLocationVector)
{
    FVector normalizedDifference = ActorLocationVector - HitLocation;
    UKismetMathLibrary::Vector_Normalize(normalizedDifference, 0.001f);
    float dotProduct = FVector::DotProduct(ForwardVector, normalizedDifference);

    float operandA = ForwardVector.X * normalizedDifference.Y;
    float operandB = ForwardVector.Y * normalizedDifference.X;

    float atan2 = UKismetMathLibrary::DegAtan2(operandA - operandB, dotProduct);

    return UKismetMathLibrary::MapRangeClamped(atan2, 180.f, -180.f, 0.f, 1.f);
}

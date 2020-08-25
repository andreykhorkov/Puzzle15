// Fill out your copyright notice in the Description page of Project Settings.


#include "GameboardActor.h"

#include "Kismet/GameplayStatics.h"

static FVector tileScale;
static FVector boardPosition;
static float boardWidth = 0;
static float boardExtentsZ = 0;
static FTransform transform;
static float initialOffset;
static float tileSize;
static int lastTileCount;
static int numTiles;

// Sets default values
AGameboardActor::AGameboardActor()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    boardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    boardMesh->SetupAttachment(RootComponent);
    boardPosition = GetActorLocation();

    transform = GetTransform();
    UE_LOG(LogTemp, Warning, TEXT("%s"), *boardPosition.ToString());
    UE_LOG(LogTemp, Warning, TEXT("%s"), "1");
}

// Called when the game starts or when spawned
void AGameboardActor::BeginPlay()
{
    Super::BeginPlay();
    FVector boardExt;
    GetActorBounds(false, boardPosition, boardExt);
    boardWidth = boardExt.Y * 2;
    boardExtentsZ = boardExt.X;

    this->OnClicked.AddDynamic(this, &AGameboardActor::OnSelected);
}

void GetCoordinates(const FVector* hitPos)
{
    int column = ((*hitPos).Y + tileSize * (initialOffset + 0.5f)) / tileSize;
    int raw = numTiles - ((*hitPos).X + tileSize * (initialOffset + 0.5f)) / tileSize;
    UE_LOG(LogTemp, Warning, TEXT("%d:%d"), column, raw);
}

void AGameboardActor::OnSelected(AActor* Target, FKey ButtonPressed)
{
    FHitResult hit(ForceInit);
    UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHitResultUnderCursor(ECollisionChannel::ECC_WorldDynamic, false, hit);
    auto location = GetTransform().InverseTransformPositionNoScale(hit.ImpactPoint);
    GetCoordinates(&location);
}

static FVector GetTileSpawnPosition(float zPos, int raw, int col)
{
    auto pos = FVector::UpVector * (zPos + 3);
    pos.Y -= tileSize * (initialOffset - col);
    pos.X += tileSize * (initialOffset - raw);
    return pos;
}

void AGameboardActor::SpawnTiles(int num)
{
    UE_LOG(LogTemp, Warning, TEXT("suka, %f, %f"), pool.size(), sizeof(int));
    if(grid != nullptr)
    {
        for (int i = 0; i < lastTileCount; ++i)
        {
            RecycleTile(grid[i]);
        }

        delete[] grid;
    }

    lastTileCount = num * num;
    grid = new ATile *[lastTileCount]{nullptr};
    numTiles = num;

    auto ratio = static_cast<float>(1) / static_cast<float>(num) * 0.90f; // * (boardWidth / tileWidth)
    tileScale = FVector(ratio, ratio, ratio);
    tileSize = boardWidth / num;
    initialOffset = num % 2 == 0 ? num / 2 - 0.5 : num / 2;
    FVector tileExtents = FVector::ZeroVector;
    
    for (int i = 0; i < num * num - 1; ++i)
    {
        auto raw = i / num;
        auto col = i % num;
        
        auto t = GetTile();
        t->AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true)); 
        t->SetActorRelativeScale3D(tileScale);

        if(tileExtents.IsZero())
        {
            FVector pos = t->GetActorLocation();
            t->GetActorBounds(false, pos, tileExtents);
        }

        const auto tilePos = GetTileSpawnPosition(tileExtents.X, raw, col);
        t->SetActorRelativeLocation(tilePos);
        t->SetNum(i + 1);
        grid[i] = t;
    }
}

ATile* AGameboardActor::GetTile()
{
    if(pool.empty())
    {
        return  GetWorld()->SpawnActor<ATile>(tile, transform);
    }

    const auto t = pool.front();
    pool.pop();
    t->SetActive(true);
    UE_LOG(LogTemp, Warning, TEXT("%d, pool size"), pool.size());
    return t;
}

void AGameboardActor::RecycleTile(ATile* t)
{
    if(t == nullptr)
    {
        return;
    }
    
    pool.emplace(t);
    t->SetActive(false);
}

// Called every frame
void AGameboardActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

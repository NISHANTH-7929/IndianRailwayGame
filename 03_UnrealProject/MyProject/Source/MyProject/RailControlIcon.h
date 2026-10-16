#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "RailControlIcon.generated.h"

// Small vector marks, drawn locally; no font glyph or texture dependency.
UCLASS()
class MYPROJECT_API URailControlIcon : public UWidget {
 GENERATED_BODY()
public:
 FName Kind;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
};

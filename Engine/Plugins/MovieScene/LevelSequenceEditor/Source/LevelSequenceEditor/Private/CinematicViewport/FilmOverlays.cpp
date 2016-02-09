// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "FilmOverlays.h"
#include "LevelSequenceEditorStyle.h"
#include "Widgets/Colors/SColorPicker.h"
#include "NumericTypeInterface.h"

#define LOCTEXT_NAMESPACE "LevelSequenceEditorFilmOverlays"

struct FFilmOverlay_None : IFilmOverlay
{
	FText GetDisplayName() const { return LOCTEXT("OverlayDisabled", "Disabled"); }
	
	const FSlateBrush* GetThumbnail() const { return FLevelSequenceEditorStyle::Get()->GetBrush("FilmOverlay.Disabled"); }

	void Paint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
	}
};

struct FFilmOverlay_Grid : IFilmOverlay
{
	FFilmOverlay_Grid(int32 InNumDivsH, int32 InNumDivsV)
		: NumDivsH(InNumDivsH)
		, NumDivsV(InNumDivsV)
	{
		BrushName = *FString::Printf(TEXT("FilmOverlay.%dx%dGrid"), NumDivsH, NumDivsV);
	}

	FText GetDisplayName() const
	{
		return FText::Format(
			LOCTEXT("GridNameFormat", "Grid ({0}x{1})"),
			FText::AsNumber(NumDivsH),
			FText::AsNumber(NumDivsV)
		);
	}
	
	const FSlateBrush* GetThumbnail() const
	{
		return FLevelSequenceEditorStyle::Get()->GetBrush(BrushName);
	}

	void Paint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
		const int32 IntervalH = AllottedGeometry.Size.X / NumDivsH;
		const int32 IntervalV = AllottedGeometry.Size.Y / NumDivsV;

		TArray<FVector2D> LinePoints;
		LinePoints.SetNum(2);

		for (int32 OffsetH = 1; OffsetH < NumDivsH; ++OffsetH)
		{
			LinePoints[0] = FVector2D(IntervalH*OffsetH, 0.f);
			LinePoints[1] = FVector2D(IntervalH*OffsetH, AllottedGeometry.Size.Y);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				Tint,
				false
				);
		}

		for (int32 OffsetV = 1; OffsetV < NumDivsV; ++OffsetV)
		{
			LinePoints[0] = FVector2D(0.f, IntervalV*OffsetV);
			LinePoints[1] = FVector2D(AllottedGeometry.Size.X, IntervalV*OffsetV);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				MyClippingRect,
				ESlateDrawEffect::None,
				Tint,
				false
				);
		}

	}
private:
	FName BrushName;
	int32 NumDivsH;
	int32 NumDivsV;
};

struct FFilmOverlay_Rabatment : IFilmOverlay
{
	FText GetDisplayName() const { return LOCTEXT("RabatmentName", "Rabatment"); }
	
	const FSlateBrush* GetThumbnail() const { return FLevelSequenceEditorStyle::Get()->GetBrush("FilmOverlay.Rabatment"); }

	void Paint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
		FVector2D Position(AllottedGeometry.Size.X / 2, AllottedGeometry.Size.Y / 2);
		float Size = FMath::Min(AllottedGeometry.Size.X, AllottedGeometry.Size.Y) * .1f;

		TArray<FVector2D> LinePoints;
		LinePoints.SetNum(2);

		LinePoints[0] = FVector2D(AllottedGeometry.Size.Y, 0.f);
		LinePoints[1] = FVector2D(AllottedGeometry.Size.Y, AllottedGeometry.Size.Y);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint,
			false
			);

		LinePoints[0] = FVector2D(AllottedGeometry.Size.X - AllottedGeometry.Size.Y, 0.f);
		LinePoints[1] = FVector2D(AllottedGeometry.Size.X - AllottedGeometry.Size.Y, AllottedGeometry.Size.Y);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint,
			false
			);

	}
};

struct FFilmOverlay_Crosshair : IFilmOverlay
{
	FText GetDisplayName() const { return LOCTEXT("CrosshairName", "Crosshair"); }
	const FSlateBrush* GetThumbnail() const { return FLevelSequenceEditorStyle::Get()->GetBrush("FilmOverlay.Crosshair"); }
	void Paint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
		FVector2D Position(AllottedGeometry.Size.X / 2, AllottedGeometry.Size.Y / 2);
		float Size = FMath::Min(AllottedGeometry.Size.X, AllottedGeometry.Size.Y) * .1f;

		TArray<FVector2D> LinePoints;
		LinePoints.SetNum(2);

		LinePoints[0] = FVector2D(Position.X, Position.Y - Size);
		LinePoints[1] = FVector2D(Position.X, Position.Y - Size * .25f);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint
			);
		LinePoints[0] = FVector2D(Position.X, Position.Y + Size);
		LinePoints[1] = FVector2D(Position.X, Position.Y + Size * .25f);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint,
			false
			);

		LinePoints[0] = FVector2D(Position.X - Size, Position.Y);
		LinePoints[1] = FVector2D(Position.X - Size * .25f, Position.Y);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint
			);
		LinePoints[0] = FVector2D(Position.X + Size, Position.Y);
		LinePoints[1] = FVector2D(Position.X + Size * .25f, Position.Y);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint,
			false
			);
	}
};

struct FFilmOverlay_SafeFrame : IFilmOverlay
{
	FFilmOverlay_SafeFrame(FText InDisplayName, float InSizePercentage, const FLinearColor& InColor)
	{
		DisplayName = InDisplayName;
		SizePercentage = InSizePercentage;

		Tint = InColor;
	}

	FText GetDisplayName() const { return DisplayName; }

	const FSlateBrush* GetThumbnail() const { return nullptr; }

	void Paint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
		FVector2D TopLeft = AllottedGeometry.Size * ((100.f - SizePercentage) * .5f) / 100.f;
		FVector2D BottomRight = AllottedGeometry.Size - TopLeft;

		TArray<FVector2D> LinePoints;
		LinePoints.Add(FVector2D(TopLeft.X,			TopLeft.Y));
		LinePoints.Add(FVector2D(BottomRight.X,		TopLeft.Y));
		LinePoints.Add(FVector2D(BottomRight.X,		BottomRight.Y));
		LinePoints.Add(FVector2D(TopLeft.X,			BottomRight.Y));
		LinePoints.Add(FVector2D(TopLeft.X-1,		TopLeft.Y-1));

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			MyClippingRect,
			ESlateDrawEffect::None,
			Tint,
			false
			);
	}

	virtual TSharedPtr<SWidget> ConstructSettingsWidget() override
	{
		struct FPercentageInterface : TDefaultNumericTypeInterface<float>
		{
			virtual FString ToString(const float& Value) const override
			{
				return TDefaultNumericTypeInterface::ToString(Value) + TEXT("%");
			}
			virtual TOptional<float> FromString(const FString& InString, const float& InExistingValue) override
			{
				return TDefaultNumericTypeInterface::FromString(InString.Replace(TEXT("%"), TEXT("")), InExistingValue);
			}
			virtual bool IsCharacterValid(TCHAR InChar) const override
			{
				return InChar == '%' || TDefaultNumericTypeInterface::IsCharacterValid(InChar);
			}
		};

		auto GetValue = [this]{ return SizePercentage; };
		auto SetValue = [this](float NewValue){ SizePercentage = NewValue; };
		auto SetValueCommitted = [this](float NewValue, ETextCommit::Type){ SizePercentage = NewValue; };

		return SNew(SHorizontalBox)

			// + SHorizontalBox::Slot()
			// .AutoWidth()
			// [
			// 	SNew(STextBlock)
			// 	.Text(LOCTEXT("SizePercentage", "Screen Percentage:"))
			// ]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SSpinBox<float>)
				.MinValue(1)
				.MaxValue(99)
				.Value_Lambda(GetValue)
				.OnValueChanged_Lambda(SetValue)
				.OnValueCommitted_Lambda(SetValueCommitted)
				.TypeInterface(MakeShareable(new FPercentageInterface))
			];
	}

private:
	float SizePercentage;
	FText DisplayName;
};

void SFilmOverlay::Construct(const FArguments& InArgs)
{
	FilmOverlays = InArgs._FilmOverlays;
}

int32 SFilmOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	TArray<IFilmOverlay*> Overlays = FilmOverlays.Get();
	for (IFilmOverlay* Overlay : Overlays)
	{
		Overlay->Paint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
		++LayerId;
	}

	return LayerId;
}

void SFilmOverlayOptions::Construct(const FArguments& InArgs)
{
	MasterColorTint = FLinearColor(1.f, 1.f, 1.f, .5f);

	MasterFilmOverlays.Add(NAME_None, TUniquePtr<IFilmOverlay>(new FFilmOverlay_None));
	MasterFilmOverlays.Add("3x3Grid", TUniquePtr<IFilmOverlay>(new FFilmOverlay_Grid(3, 3)));
	MasterFilmOverlays.Add("2x2Grid", TUniquePtr<IFilmOverlay>(new FFilmOverlay_Grid(2, 2)));
	MasterFilmOverlays.Add("Crosshair", TUniquePtr<IFilmOverlay>(new FFilmOverlay_Crosshair));
	MasterFilmOverlays.Add("Rabatment", TUniquePtr<IFilmOverlay>(new FFilmOverlay_Rabatment));

	ToggleableOverlays.Add("ActionSafeFrame",	TUniquePtr<IFilmOverlay>(new FFilmOverlay_SafeFrame(LOCTEXT("ActionSafeFrame", "Action Safe"), 95.f, FLinearColor::Red)));
	ToggleableOverlays.Add("TitleSafeFrame",	TUniquePtr<IFilmOverlay>(new FFilmOverlay_SafeFrame(LOCTEXT("TitleSafeFrame", "Title Safe"), 90.f, FLinearColor::Yellow)));
	ToggleableOverlays.Add("CustomSafeFrame",	TUniquePtr<IFilmOverlay>(new FFilmOverlay_SafeFrame(LOCTEXT("CustomSafeFrame", "Custom Safe"), 85.f, FLinearColor::Green)));

	OverlayWidget = SNew(SFilmOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		.FilmOverlays(this, &SFilmOverlayOptions::GetActiveFilmOverlays);

	ChildSlot
	[
		SNew(SComboButton)
		.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
		.ForegroundColor(FEditorStyle::GetColor("InvertedForeground"))
		.OnGetMenuContent(this, &SFilmOverlayOptions::GetMenuContent)
		.ButtonContent()
		[
			SNew(SBox)
			.WidthOverride(36)
			.HeightOverride(24)
			.ToolTipText(LOCTEXT("FilmOverlaysToolTip", "Displays a list of available film overlays to apply to this viewport."))
			[
				SNew(SImage)
				.Image(this, &SFilmOverlayOptions::GetCurrentThumbnail)
			]
		]
	];
}

FLinearColor SFilmOverlayOptions::GetMasterColorTint() const
{
	return MasterColorTint;
}

void SFilmOverlayOptions::SetMasterColorTint(FLinearColor Tint)
{
	MasterColorTint = Tint;

	IFilmOverlay* Overlay = GetMasterFilmOverlay();
	if (Overlay)
	{
		Overlay->SetTint(MasterColorTint);
	}
}

TSharedRef<SWidget> SFilmOverlayOptions::GetMenuContent()
{
	return SNew(SGridPanel)

		+ SGridPanel::Slot(0, 0)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 5.f)
			[
				SNew(SHeader)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OverlaysHeader", "Composition Overlays"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				ConstructMasterOverlaysMenu()
			]
		]

		+ SGridPanel::Slot(0, 1)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHeader)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ColorPickerHeader", "Overlay Color Tint"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			.HAlign(HAlign_Center)
			[
				SNew(SColorPicker)
				.TargetColorAttribute(this, &SFilmOverlayOptions::GetMasterColorTint)
				.UseAlpha(true)
				.DisplayInlineVersion(true)
				.OnColorCommitted(this, &SFilmOverlayOptions::SetMasterColorTint)
			]
		]

		// @todo: specific overlay settings to go here
		// + SGridPanel::Slot(1, 0)
		// [
		// 
		// ]

		+ SGridPanel::Slot(1, 0)
		//+ SGridPanel::Slot(1, 1)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 5.f)
			[
				SNew(SHeader)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SafeFrameHeader", "Safe Frames"))
				]
			]
			
			+ SVerticalBox::Slot()
			[
				ConstructToggleableOverlaysMenu()
			]
		];
}

TSharedRef<SWidget> SFilmOverlayOptions::ConstructMasterOverlaysMenu()
{
	TSharedRef<SUniformGridPanel> OverlaysPanel = SNew(SUniformGridPanel).SlotPadding(10.f);

	TArray<FName> OverlayNames;
	MasterFilmOverlays.GenerateKeyArray(OverlayNames);

	const int32 NumColumns = FMath::Log2(OverlayNames.Num() - 1);

	int32 ColumnIndex = 0, RowIndex = 0;
	for (int32 OverlayIndex = 0; OverlayIndex < OverlayNames.Num(); ++OverlayIndex)
	{
		IFilmOverlay& Overlay = *MasterFilmOverlays[OverlayNames[OverlayIndex]].Get();
		OverlaysPanel->AddSlot(ColumnIndex, RowIndex)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.OnClicked(this, &SFilmOverlayOptions::SetMasterFilmOverlay, OverlayNames[OverlayIndex])
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.Padding(0.f, 4.f)
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(36)
					.HeightOverride(24)
					[
						SNew(SImage)
						.Image(Overlay.GetThumbnail())
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FEditorStyle::GetColor("DefaultForeground"))
					.Text(Overlay.GetDisplayName())
				]
			]
		];

		if (++ColumnIndex >= NumColumns)
		{
			ColumnIndex = 0;
			++RowIndex;
		}
	}

	return OverlaysPanel;
}

TSharedRef<SWidget> SFilmOverlayOptions::ConstructToggleableOverlaysMenu()
{
	TSharedRef<SGridPanel> GridPanel = SNew(SGridPanel);

	int32 Row = 0;
	for (auto& Pair : ToggleableOverlays)
	{
		TSharedPtr<SWidget> Settings = Pair.Value->ConstructSettingsWidget();
		if (!Settings.IsValid())
		{
			continue;
		}

		TUniquePtr<IFilmOverlay>* Ptr = &Pair.Value;

		auto OnCheckStateChanged = [=](ECheckBoxState InNewState){ (*Ptr)->SetEnabled(InNewState == ECheckBoxState::Checked); };
		auto IsChecked = [=]{ return (*Ptr)->IsEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };
		auto IsEnabled = [=]{ return (*Ptr)->IsEnabled(); };

		Settings->SetEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(IsEnabled)));

		GridPanel->AddSlot(0, Row)
		.VAlign(VAlign_Center)
		.Padding(10.f, 5.f)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged_Lambda(OnCheckStateChanged)
			.IsChecked_Lambda(IsChecked)
			[
				SNew(STextBlock)
				.Text(Pair.Value->GetDisplayName())
			]
		];

		GridPanel->AddSlot(1, Row)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.Padding(10.f, 5.f)
		[
			Settings.ToSharedRef()
		];

		++Row;
	}

	return GridPanel;
}

FReply SFilmOverlayOptions::SetMasterFilmOverlay(FName InName)
{
	CurrentMasterOverlay = InName;
	IFilmOverlay* Overlay = GetMasterFilmOverlay();
	if (Overlay)
	{
		Overlay->SetTint(MasterColorTint);
	}
	return FReply::Unhandled();
}

TSharedRef<SFilmOverlay> SFilmOverlayOptions::GetFilmOverlayWidget() const
{
	return OverlayWidget.ToSharedRef();
}

TArray<IFilmOverlay*> SFilmOverlayOptions::GetActiveFilmOverlays() const
{
	TArray<IFilmOverlay*> Overlays;

	if (IFilmOverlay* Overlay = GetMasterFilmOverlay())
	{
		Overlays.Add(Overlay);
	}

	for (auto& Pair: ToggleableOverlays)
	{
		if (Pair.Value->IsEnabled())
		{
			Overlays.Add(Pair.Value.Get());
		}
	}

	return Overlays;
}

const FSlateBrush* SFilmOverlayOptions::GetCurrentThumbnail() const
{
	if (!CurrentMasterOverlay.IsNone())
	{
		return MasterFilmOverlays[CurrentMasterOverlay]->GetThumbnail();
	}

	return FLevelSequenceEditorStyle::Get()->GetBrush("FilmOverlay.DefaultThumbnail");
}

IFilmOverlay* SFilmOverlayOptions::GetMasterFilmOverlay() const
{
	if (!CurrentMasterOverlay.IsNone())
	{
		return MasterFilmOverlays[CurrentMasterOverlay].Get();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
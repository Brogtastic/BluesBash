#include "BluesBash_UI.h"

bool IsActive(ui_id Id) {
	return (Id.OwnerFunc == UIContext.Active.OwnerFunc) && (Id.Index == UIContext.Active.Index);
}

bool IsHot(ui_id Id) {
	return (Id.OwnerFunc == UIContext.Hot.OwnerFunc) && (Id.Index == UIContext.Hot.Index);
}

// NOTE(Roskuski): If HitRect.width or HitRect.height are 0, those properties are set to the width of the texture of the button.
ui_result DoUIButton(ui_id Id, Rectangle HitRect, animation_state State) {
	ui_result Result = {false, false};

	Texture2D *CurrentFrame = GetCurrentFrame(State);
	if ((HitRect.width == 0) || (HitRect.height == 0)) {
		HitRect.width = CurrentFrame->width;
		HitRect.height = CurrentFrame->height;
	}
	
	DrawTexture(*CurrentFrame, HitRect.x, HitRect.y, WHITE);
	
	const Vector2 MousePos = GetMousePosition();

	if (CheckCollisionPointRec(MousePos, HitRect)) {
		UIContext.Hot = Id;
		Result.Hot = true;
	}
	else if (IsHot(Id)) {
		UIContext.Hot = {0, 0};
	}
	
	if (IsActive(Id)) {
		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			if (IsHot(Id)) {
				Result.PerformAction = true;
			}
			UIContext.Active = {0, 0};
		}
	}
	else if (IsHot(Id)) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			UIContext.Active = Id;
		}
	}
	
	// NOTE(Roskuski): Debug display of the button HitRect
	{
		Color Temp;
		if (IsActive(Id)) {
			Temp = VIOLET;
		}
		else if (IsHot(Id)) {
			Temp = GREEN;
		}
		else {
			Temp = RED;
		}
		
		Temp.a = 0xff * 0.25;
		DrawRectangleRec(HitRect, Temp);
	}
	
	return Result; 
}

// NOTE(Roskuski): I decided that the lowest friction way to do this right now, is to use line number as our "index" for UI elements that are made in the same function.
// NOTE(Roskuski): As long as we don't make two different UI elements on the same line, this should be a fine assumption...
#define DoUIButtonAutoId(...) DoUIButton({CurrentFunction, __LINE__}, __VA_ARGS__)

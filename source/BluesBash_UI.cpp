#include "BluesBash_UI.h"

bool IsActive(ui_id Id) {
	return (Id.OwnerFunc == UIContext.Active.OwnerFunc) && (Id.Index == UIContext.Active.Index);
}

bool IsHot(ui_id Id) {
	return (Id.OwnerFunc == UIContext.Hot.OwnerFunc) && (Id.Index == UIContext.Hot.Index);
}

ui_result DoUIButton(ui_id Id, Rectangle GraphicsRect, Rectangle HitRect, animation_state State) {
	ui_result Result = {false, false};

	Texture2D *CurrentFrame = GetCurrentFrame(State);
	
	DrawTextureQuad(*CurrentFrame, {1, 1}, {0, 0}, GraphicsRect, WHITE);
	
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
	// @TODO(Roskuski): We should disable this in non-debug builds.
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

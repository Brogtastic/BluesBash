#include "BluesBash_UI.h"

void LoadUim(const char *Path) {
	FILE *File = fopen(Path, "rb");
	if (File) {
		int BaseKeyLength = 0;
		fread(&BaseKeyLength, sizeof(BaseKeyLength), 1, File);

		char *BaseKey = (char*)malloc(BaseKeyLength + 1);
		fread(BaseKey, BaseKeyLength, 1, File);
		BaseKey[BaseKeyLength] = 0;

		int ButtonNum = 0;
		fread(&ButtonNum, sizeof(ButtonNum), 1, File);
		for (int Index = 0; Index < ButtonNum; Index++) {
			button_def Button = {};

			int ButtonKeyLength = 0; 
			fread(&ButtonKeyLength, sizeof(ButtonKeyLength), 1, File);
			Button.Entry.Key = (char*)malloc(ButtonKeyLength + BaseKeyLength + 2);
			memcpy(Button.Entry.Key, BaseKey, BaseKeyLength);
			Button.Entry.Key[BaseKeyLength] = '_'; 
			fread(&Button.Entry.Key[BaseKeyLength + 1], ButtonKeyLength, 1, File);
			Button.Entry.Key[ButtonKeyLength + BaseKeyLength + 1] = 0;

			int GraphicKeyLength = 0;
			fread(&GraphicKeyLength, sizeof(GraphicKeyLength), 1, File);
			Button.AniState.Key = (char*)malloc(GraphicKeyLength);
			Button.AniState.CurrentFrameMajor = 0;
			Button.AniState.CurrentFrameMinor = 0;
			Button.AniState.CurrentTime = 0;
			fread(Button.AniState.Key, GraphicKeyLength, 1, File);
			Button.AniState.Key[GraphicKeyLength] = 0;

			fread(&Button.HitRect, sizeof(Button.HitRect), 1, File);
			fread(&Button.HitRotation, sizeof(Button.HitRotation), 1, File);
			fread(&Button.GraphicRect, sizeof(Button.GraphicRect), 1, File);
			fread(&Button.GraphicRotation, sizeof(Button.GraphicRotation), 1, File);
			ButtonMap_Insert(Button.Entry.Key, Button);
		}

		free(BaseKey);
		fclose(File);
	}
	else {
		printf("[LoadUim]: Failed to load \"%s\"\n", Path);
	}
}

bool IsActive(ui_id Id) {
	return Id.KeyPointer == UIContext.Active.KeyPointer;
}

bool IsHot(ui_id Id) {
	return Id.KeyPointer == UIContext.Hot.KeyPointer;
}

bool CheckCollisionPointRotatedRec(Vector2 Point, float Rotation, Rectangle Rect) {
	Rotation *= DEG2RAD;
	// mat2x2 for rotation.
	float MatA = cos(Rotation); float MatB = -sin(Rotation);
	float MatC = sin(Rotation); float MatD = cos(Rotation);

	// Rectangle points that are centered in the centered of the rect
	Vector2 RectPoints[4] = {
		{-Rect.width/2, -Rect.height/2},
		{Rect.width - (Rect.width/2), -Rect.height/2},
		{-Rect.width/2, Rect.height - (Rect.height/2)},
		{Rect.width - (Rect.width/2), Rect.height - (Rect.height/2)},
	};

	for (int Index = 0; Index < ArrayCount(RectPoints); Index++) {
		Vector2 Temp = RectPoints[Index];
		RectPoints[Index].x = MatA * Temp.x + MatB * Temp.y;
		RectPoints[Index].y = MatC * Temp.x + MatD * Temp.y;
		// Add in the offset after we're done rotating.
		RectPoints[Index].x += Rect.x;
		RectPoints[Index].y += Rect.y;
	}

	bool Result = CheckCollisionPointTriangle(Point, RectPoints[2], RectPoints[1], RectPoints[0]) || CheckCollisionPointTriangle(Point, RectPoints[1], RectPoints[2], RectPoints[3]);

	return Result;
}

ui_result DoUIButtonFromMap(char *Key) {
	button_def *Button = ButtonMap_Get(Key);
	ui_result Result = {false, false};
	if (Button) {
		ui_id Id = {(uintptr_t)Button->Entry.Key};
		Result = DoUIButton(Button, Id);
	}
	else {
		printf("[DoUIButtonFromMap]: Key \"%s\" does not exist!\n", Key);
		// @TODO(Roskuski): create a visual warning as well?
	}
	return Result;
}

ui_result DoUIButton(button_def *Button, ui_id Id) {
	ui_result Result = {false, false};
	Texture2D *CurrentFrame = GetCurrentFrame(Button->AniState);
	const Vector2 MousePos = GetMousePosition();

	if (CurrentFrame) {
		DrawTexturePro(*CurrentFrame, {0, 0, (float)CurrentFrame->width, (float)CurrentFrame->height}, Button->GraphicRect, {Button->GraphicRect.width/2, Button->GraphicRect.height/2}, Button->GraphicRotation, WHITE);
	}
	else { // fallback if the graphic isn't valid.
		DrawRectanglePro(Button->GraphicRect, {Button->GraphicRect.width/2, Button->GraphicRect.height/2}, Button->GraphicRotation, PURPLE);
	}

	if (CheckCollisionPointRotatedRec(MousePos, Button->HitRotation, Button->HitRect)) {
		UIContext.Hot = Id;
		Result.Hot = true;
	}
	else if (IsHot(Id)) {
		UIContext.Hot = Nouiid;
	}
	
	if (IsActive(Id)) {
		if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			if (IsHot(Id)) {
				Result.PerformAction = true;
			}
			UIContext.Active = Nouiid;
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
		DrawRectanglePro(Button->HitRect, {Button->HitRect.width/2, Button->HitRect.height/2}, Button->HitRotation, Temp);
	}
	
	return Result; 
}

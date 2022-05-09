#include "BluesBash_UI.h"

void LoadUim_Buttons(FILE *File) {
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
		Button.AniState.Key = (char*)malloc(GraphicKeyLength + 1);
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

void LoadUim_TextAreas(FILE *File) {
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
		Button.AniState.Key = (char*)malloc(GraphicKeyLength + 1);
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

	int TextAreaNum = 0;
	fread(&TextAreaNum, sizeof(TextAreaNum), 1, File);
	for (int Index = 0; Index < TextAreaNum; Index++) {
		text_area_def TextArea = {};

		int TextAreaKeyLength = 0;
		fread(&TextAreaKeyLength, sizeof(TextAreaKeyLength), 1, File);
		TextArea.Entry.Key = (char*)malloc(TextAreaKeyLength + BaseKeyLength + 2); // NOTE(Roskuski): + 2 for '_' and '\0'.
		memcpy(TextArea.Entry.Key, BaseKey, BaseKeyLength);
		TextArea.Entry.Key[BaseKeyLength] = '_';
		fread(&(TextArea.Entry.Key[BaseKeyLength + 1]), TextAreaKeyLength, 1, File);
		TextArea.Entry.Key[TextAreaKeyLength + BaseKeyLength + 1] = 0;

		int GraphicKeyLength = 0;
		fread(&GraphicKeyLength, sizeof(GraphicKeyLength), 1, File);
		TextArea.AniState.Key = (char*)malloc(GraphicKeyLength + 1);
		TextArea.AniState.CurrentFrameMajor = 0;
		TextArea.AniState.CurrentFrameMinor = 0;
		TextArea.AniState.CurrentTime = 0;
		fread(TextArea.AniState.Key, GraphicKeyLength, 1, File);
		TextArea.AniState.Key[GraphicKeyLength] = 0;

		fread(&TextArea.BufferMaxSize, sizeof(TextArea.BufferMaxSize), 1, File);
		fread(&TextArea.FontSize, sizeof(TextArea.FontSize), 1, File);
		fread(&TextArea.HitRect, sizeof(TextArea.HitRect), 1, File);
		fread(&TextArea.GraphicRect, sizeof(TextArea.GraphicRect), 1, File);

		// NOTE(Roskuski) Text areas are actually able to hold one less character than expected
		// This is because of the null term
		TextArea.Buffer = (char*)malloc(TextArea.BufferMaxSize);
		memset(TextArea.Buffer, 0, TextArea.BufferMaxSize);
		TextAreaMap_Insert(TextArea.Entry.Key, TextArea);
	}

	free(BaseKey);
	fclose(File);
}

void LoadUim(const char *Path) {
	FILE *File = fopen(Path, "rb");
	if (File) {
		char MagicNumber[4];
		fread(MagicNumber, ArrayCount(MagicNumber), 1, File);
		if (MagicNumber[0] == 'U' &&
		    MagicNumber[1] == 'I' &&
		    MagicNumber[2] == 'M' &&
		    MagicNumber[3] == 0) {
			unsigned int FileVersion = 0;
			fread(&FileVersion, sizeof(FileVersion), 1, File);
			switch (FileVersion) {
				case Uim_Buttons: {
					LoadUim_Buttons(File);
				} break;
				case Uim_TextAreas: {
					LoadUim_TextAreas(File);
				} break;

				case Uim_LatestPlusOne:
				default: {
					printf("[LoadUim] The version of \"%s\" was %d, which is past the latest version %d.\n", Path, FileVersion, Uim_LatestPlusOne - 1);
				} break;
			}
		}
		else { printf("[LoadUim] The FourCC of \"%s\" did not match \"UIM\\x00\"!\n", Path); }
	}
	else { printf("[LoadUim] There was an error opening \"%s\"\n", Path); }
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

	return Result;
}

ui_result DoUITextArea(text_area_def *TextArea, ui_id Id) {
	ui_result Result = {false, false};
	animation *Visual = AnimationMap_Get(TextArea->AniState.Key);
	const Vector2 MousePos = GetMousePosition();

	Rectangle CenteredRect = {TextArea->HitRect.x - TextArea->HitRect.width/2, TextArea->HitRect.y - TextArea->HitRect.height/2, TextArea->HitRect.width, TextArea->HitRect.height};
	Rectangle TextBoxRect = CenteredRect;
	TextBoxRect.y += TextArea->HitRect.height/2 - ((float)TextArea->FontSize/2);

	if (CheckCollisionPointRec(MousePos, CenteredRect)) {
		UIContext.Hot =  Id;
		Result.Hot = true;
	}
	else if (IsHot(Id)) {
		UIContext.Hot = Nouiid;
	}

	if (IsHot(Id) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		UIContext.Active = Id;
		Result.PerformAction = true;
	}

	if (IsActive(Id)) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(MousePos, CenteredRect)) {
			UIContext.Active = Nouiid;
		}
		else {
			Result.PerformAction = true;
		}
	}

	if (Visual) {
		Texture2D *Frame = &Visual->Frames[0].Graphic;
		DrawTexturePro(*Frame, {0, 0, (float)Frame->width, (float)Frame->height}, TextArea->GraphicRect, {TextArea->GraphicRect.width/2, TextArea->GraphicRect.height/2}, 0, WHITE);
	}
	else {
		DrawRectanglePro(TextArea->HitRect, {TextArea->HitRect.width/2, TextArea->HitRect.height/2}, 0, WHITE);
		DrawRectanglePro({TextArea->HitRect.x - 2, TextArea->HitRect.y - 2, TextArea->HitRect.width + 2, TextArea->HitRect.height + 2}, {TextArea->HitRect.width/2, TextArea->HitRect.height/2}, 0, BLACK);
	}

	BeginScissorMode(TextBoxRect.x, TextBoxRect.y, TextBoxRect.width, TextBoxRect.height);
	int TextLength = MeasureText(TextArea->Buffer, TextArea->FontSize);
	int XOffset = 0;
	// @TODO(Roskuski): Make sure that this offset code works properly
	while (TextLength - XOffset > TextArea->HitRect.width) {
		XOffset += (TextArea->HitRect.width * 1)/4;
	}
	DrawText(TextArea->Buffer, TextBoxRect.x - XOffset, TextBoxRect.y, TextArea->FontSize, BLACK);
	if (Result.PerformAction) {
		DrawRectangle(TextBoxRect.x - XOffset + TextLength + 2, TextBoxRect.y, 3, TextArea->FontSize, BLACK);
	}
	EndScissorMode();

	return Result;
}

ui_result DoUITextAreaFromMap(char *Key) {
	text_area_def *TextArea = TextAreaMap_Get(Key);
	ui_result Result = {false, false};
	if (TextArea) {
		ui_id Id = {(uintptr_t)TextArea->Entry.Key};
		Result = DoUITextArea(TextArea, Id);
	}
	else {
		printf("[DoUITextAreaFromMap]: Key \"%s\" does not exist!\n", Key);
		// @TODO(Roskuski): create a visual warning as well?
	}
	return Result;
}

bool DoTextInput(text_area_def *TextArea) {
	bool RanOutOfSpace = false;
	if (IsKeyPressed(KEY_BACKSPACE)) {
		TextArea->BufferCurrentLength -= 1;
		TextArea->Buffer[TextArea->BufferCurrentLength] = 0;
	}

	for (int UniChar = GetCharPressed(); UniChar != 0; UniChar = GetCharPressed()) {
		if (UniChar > 255) { printf("[DoTextInput]: I got a unicode char! discarding it..."); }
		else if (TextArea->BufferCurrentLength <= TextArea->BufferMaxSize - 2) { // Do we have space for another character?
			TextArea->Buffer[TextArea->BufferCurrentLength] = (char)UniChar;
			TextArea->BufferCurrentLength += 1;
			TextArea->Buffer[TextArea->BufferCurrentLength] = 0;
		}
		else { // We didn't have space for it.
			RanOutOfSpace = true;
			break;
		}
	}
	return RanOutOfSpace;
}

bool DoTextInputFromMap(char *Key) {
	text_area_def *TextArea = TextAreaMap_Get(Key);
	bool Result = false;
	if (TextArea) {
		ui_id Id = {(uintptr_t)TextArea->Entry.Key};
		DoTextInput(TextArea);
	}
	else {
		printf("[DoTextInputFromMap]: Key \"%s\" does not exist!\n", Key);
		// @TODO(Roskuski): create a visual warning as well?
	}
	return Result;
}

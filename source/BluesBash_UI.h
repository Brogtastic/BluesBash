#pragma once

struct ui_id {
	uintptr_t KeyPointer; // the address of the start of the string key. should be unique in all valid cases.
};

#define Nouiid {0}

struct ui_context {
	ui_id Hot; // NOTE(Roskuski): ID of the element that is about to be interacted with.
	ui_id Active; // NOTE(Roskuski): ID of the element that is being interacted with.
};

global_var ui_context UIContext = {};

enum uim_version {
	Uim_Buttons = 0,
	Uim_TextAreas,

	Uim_LatestPlusOne, // NOTE(Roskuski): Keep this at the end
};

// @TODO(Roskuski): Might have to make a different return type for different ui elements.
struct ui_result {
	bool PerformAction;
	bool Hot;
};

struct button_def {
	map_entry Entry;

	Rectangle HitRect;
	float HitRotation;
	animation_state AniState;
	Rectangle GraphicRect;
	float GraphicRotation;
};

// @TODO(Roskuski): Moveable cursor on text areas?
struct text_area_def {
	map_entry Entry;
	char *Buffer;
	int BufferCurrentLength;
	int BufferMaxSize;
	int FontSize;
	Rectangle HitRect;
	Rectangle GraphicRect;
	animation_state AniState;
};

// full button key: uim file name + button name => "TopMenu_PlayButton"

// ButtonMap Start
#define ButtonMap_BucketCount (50)
#define ButtonMap_BackingCount (150)
#define ButtonMap_NoEntry (-1)

button_def ButtonMap_Backing[ButtonMap_BackingCount];
int ButtonMap_Buckets[ButtonMap_BucketCount];
int ButtonMap_FreeListHead;
// ButtonMap End

// TextAreaMap Start
#define TextAreaMap_BucketCount (50)
#define TextAreaMap_BackingCount (100)
#define TextAreaMap_NoEntry (-1)

text_area_def TextAreaMap_Backing[TextAreaMap_BackingCount];
int TextAreaMap_Buckets[TextAreaMap_BucketCount];
int TextAreaMap_FreeListHead;
// TextAreaMap End

void LoadUim(const char* Path);

// Tests if `Id` is the active id. Active means that we are currently interacting with this UI element.
bool IsActive(ui_id Id);

// Tests if Id is the hot id. Hot means that we are about to interact with this UI element. This is typically detected by the mouse hovering over the UI Element's interactive region.
bool IsHot(ui_id Id);

ui_result DoUIButton(button_def *Button, ui_id Id);
ui_result DoUIButtonFromMap(char *Key);

ui_result DoUITextArea(text_area_def *TextArea, ui_id Id);
ui_result DoUITextAreaFromMap(char *Key);
bool DoTextInput(text_area_def *TextArea);
bool DoTextInputFromMap(char *Key);

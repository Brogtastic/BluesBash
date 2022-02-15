#pragma once

struct ui_id {
	void *OwnerFunc; // Pointer to the function that "owns" this ui element
	int Index; // Index of that element in this function.
};

struct ui_context {
	ui_id Hot; // NOTE(Roskuski): ID of the element that is about to be interacted with.
	ui_id Active; // NOTE(Roskuski): ID of the element that is being interacted with.
};

global_var ui_context UIContext = {};

// @TODO(Roskuski): Might have to make a different return type for different ui elements.
struct ui_result {
	bool PerformAction;
	bool Hot;
};

// Tests if `Id` is the active id. Active means that we are currently interacting with this UI element.
bool IsActive(ui_id Id);

// Tests if Id is the hot id. Hot means that we are about to interact with this UI element. This is typically detected by the mouse hovering over the UI Element's interactive region.
bool IsHot(ui_id Id);

// Id: The ID of this Button
// GraphicsRect: the rectangle that will be textured with our graphics
// HitRect: the rectangle that is the HitBox for our button
// State: animation_state of the animation that corrisponds to our button
ui_result DoUIButton(ui_id Id, Rectangle GraphicsRect, Rectangle HitRect, animation_state State);

// NOTE(Roskuski): I decided that the lowest friction way to do this right now, is to use line number as our "index" for UI elements that are made in the same function.
// NOTE(Roskuski): If we make UI Elements in a function, there must be a void* variable called `CurrentFunction` that is a pointer to the function. You can do this by:
// NOTE(Rosksuki):`void *CurrentFunction = FunctionName` Note that FunctionName here doesn't have parenthesis.
// NOTE(Roskuski): As long as we don't make two different UI elements on the same line, this should be a fine assumption...

// NOTE(Roskuski): To Brog, if you make a new UI element, use this function, and then refer to `DoUIButton`'s parameters and ignore that first parameter exists. The purpose of this macro is to automatically populate the first parameter with a value that we do not have to think about.
#define DoUIButtonAutoId(...) DoUIButton({CurrentFunction, __LINE__}, __VA_ARGS__)

#pragma once

struct ui_id {
	void *OwnerFunc; // Pointer to the function that "owns" this ui element
	int Index; // Index that is incremented per each 
};

struct ui_context {
	ui_id Hot; // NOTE(Roskuski): function pointer of element that is about to be interacted with.
	ui_id Active; // NOTE(Roskuski): function point of element that is being interacted with.
};

global_var ui_context UIContext = {};

// @TODO(Roskuski): Might have to make a different return type for different ui elements.
struct ui_result {
	bool PerformAction;
	bool Hot;
};

bool IsActive(ui_id Id);
bool IsHot(ui_id Id);

ui_result DoUIButton(ui_id Id, Rectangle HitRect, animation_state State);

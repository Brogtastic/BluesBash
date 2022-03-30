package UIBuilder

import ray "vendor:raylib"
import "core:path/filepath"
import "core:os"
import "core:fmt"
import "core:io"
import "core:bufio"
import "core:strings"
import "core:mem"
import "core:math/linalg"
import "core:c"
import "core:sys/win32"
import "core:runtime"

frame :: struct {
	FrameLength : i32,
	Graphic : ray.Texture2D,
}

animation :: struct {
	FrameTime : f64,
	UniqueFrameCount : i32,
	Frames : []frame,
}

StringHash :: proc(String : []u8) -> (Hash : i32) {
	Hash = 0
	for Byte in String {
		Hash += i32(Byte)
	}
	return Hash
}

AnimationMap : map[string]animation

LoadPppFile :: proc(PppPath : string) {
	PppHandle, PppError := os.open(PppPath, os.O_RDONLY)
	if PppError != os.ERROR_NONE {
		fmt.eprintf("[LoadPppFile] Failed to open handle for file \"%s\"", PppPath)
	}
	PppStream := os.stream_from_handle(PppHandle)
	
	PppReader, PppReaderOk := io.to_reader(PppStream)
	defer io.close(cast(io.Closer)PppReader)
	if !PppReaderOk {
		fmt.eprintf("[LoadPppFile] io.to_reader() failed!");
	}
	
	KeyLen : i32
	io.read(PppReader, mem.ptr_to_bytes(&KeyLen))

	Key := make([]u8, KeyLen)
	defer delete(Key)
	io.read(PppReader, Key[:])

	Animation : animation = ---

	FrameRateN, FrameRateD : i32 
	io.read(PppReader, mem.ptr_to_bytes(&FrameRateN))
	io.read(PppReader, mem.ptr_to_bytes(&FrameRateD))
	Animation.FrameTime = (cast(f64)(FrameRateN)) / (cast(f64)FrameRateD)
	
	io.read(PppReader, mem.ptr_to_bytes(&Animation.UniqueFrameCount))
	Animation.Frames = make([]frame, Animation.UniqueFrameCount)

	for Index in 0..<Animation.UniqueFrameCount {
		PathLen : i32
		io.read(PppReader, mem.ptr_to_bytes(&PathLen))
		Path := make([]u8, PathLen)
		io.read(PppReader, Path[:])
		PathStr := strings.clone_from_bytes(Path)
		defer delete(PathStr)
		PathCStr := strings.clone_to_cstring(PathStr)
		defer delete(PathCStr)
		Animation.Frames[Index].Graphic = ray.LoadTexture(PathCStr)

		io.read(PppReader, mem.ptr_to_bytes(&Animation.Frames[Index].FrameLength))
	}
	AnimationMap[strings.clone_from_bytes(Key, context.temp_allocator)] = Animation
	fmt.printf("[LoadPppFile] Loading of \"%s\" was successful\n", PppPath)
}

/* File format
What we need to store:
Header:
 - KeyLen : i32
 - Key : [KeyLen]u8
 - Number of buttons : i32
 - Buttons

Button:
 - KeyLen : i32
 - Key : [KeyLen]u8
 - Graphic KeyLen : i32
 - Graphic Key : [Graphic KeyLen]u8
 - HitRect : ray.Rectangle
 - GraphicRect : ray.Rectangle
*/

SaveToFile :: proc(Path : cstring, FileNameOffset, FileExtentionOffset : u16) {
	Path := string(Path)
	Handle, HandleError := os.open(Path, os.O_WRONLY | os.O_CREATE)
	if (HandleError == os.ERROR_NONE) {
		Key := Path[FileNameOffset:FileExtentionOffset]
		KeyLen := i32(len(Key))
		os.write_ptr(Handle, &KeyLen, size_of(KeyLen))
		os.write_string(Handle, Key)

		ButtonCount := i32(len(ButtonList))
		os.write_ptr(Handle, &ButtonCount, size_of(ButtonCount))

		for Button in &ButtonList {
			KeyLen = i32(len(Button.Key))
			os.write_ptr(Handle, &KeyLen, size_of(KeyLen))
			os.write(Handle, Button.Key[:])

			KeyLen = i32(len(Button.GraphicKey))
			os.write_ptr(Handle, &KeyLen, size_of(KeyLen))
			os.write(Handle, Button.GraphicKey[:])

			os.write_ptr(Handle, &Button.HitRect, size_of(Button.HitRect))
			os.write_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))
		}
		os.close(Handle)
	}
	else do fmt.printf("[SaveToFile]: There was an error opening \"%s\"(%d)\n", Path, HandleError)
}

OpenFromFile :: proc(Path : cstring) {
	for Button in &ButtonList {
		delete(Button.Key)
		delete(Button.GraphicKey)
	}
	clear(&ButtonList)
	
	Path := string(Path)
	Handle, HandleError := os.open(Path, os.O_RDONLY)
	if (HandleError == os.ERROR_NONE) {
		Len : i32
		os.read_ptr(Handle, &Len, size_of(Len))
		os.seek(Handle, i64(Len), win32.FILE_CURRENT)
		ButtonCount : i32 
		os.read_ptr(Handle, &ButtonCount, size_of(ButtonCount))
		for Index in 0..<ButtonCount {
			Button : button_def
			os.read_ptr(Handle, &Len, size_of(Len))
			Button.Key = make([dynamic]u8, Len)
			os.read(Handle, Button.Key[:])

			os.read_ptr(Handle, &Len, size_of(Len))
			Button.GraphicKey = make([dynamic]u8, Len)
			os.read(Handle, Button.GraphicKey[:])

			os.read_ptr(Handle, &Button.HitRect, size_of(Button.HitRect))
			os.read_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))

			append(&ButtonList, Button)
		}
	}
	else do fmt.printf("[OpenFromFile]: There was anm error opening \"%s\"(%d)\n", Path, HandleError)
}

button_def :: struct {
	Key : [dynamic]u8,
	HitRect : ray.Rectangle,
	GraphicKey : [dynamic]u8,
	GraphicRect : ray.Rectangle,
}

// returns true if we are the active info element
DoInfoTextArea_string :: proc(Rect : ray.Rectangle, String : string, MousePos : linalg.Vector2f32, ClickHitInfo : ^bool, CallLoc := #caller_location) -> (Result : bool) {
	Result = false
	ray.DrawRectangleRec(Rect, ray.BLACK)
	CString := strings.clone_to_cstring(String, context.temp_allocator)

	if ray.CheckCollisionPointRec(MousePos, Rect) {
		if ray.IsMouseButtonPressed(.LEFT) {
			InfoSelectInfo.uiid  = CallLoc.line
			if ClickHitInfo^ != true do ClickHitInfo^ = true
		}
	}

	if InfoSelectInfo.uiid == CallLoc.line do Result = true

	TextColor := ray.WHITE
	if Result do TextColor = ray.GREEN
	ray.DrawText(CString, i32(Rect.x), i32(Rect.y), 20, TextColor)

	return Result
}

DoInfoTextArea_u8 :: proc(Rect : ray.Rectangle, String : []u8, MousePos : linalg.Vector2f32, ClickHitInfo : ^bool, CallLoc := #caller_location) -> (Result : bool) {
	return DoInfoTextArea_string(Rect, strings.clone_from_bytes(String, context.temp_allocator), MousePos, ClickHitInfo, CallLoc)
}

DoInfoTextArea :: proc {
	DoInfoTextArea_u8,
	DoInfoTextArea_string,
}

KeyboardTypeInput :: proc(Target : ^[dynamic]u8, Allocator := context.allocator) {
	CharList := map[ray.KeyboardKey]u8 { .A = 'a', .B = 'b', .C = 'c', .D = 'd', .E = 'e', .F = 'f', .G = 'g', .H = 'h', .I = 'i', .J = 'j', .K = 'k', .L = 'l', .M = 'm', .N = 'n', .O = 'o', .P = 'p', .Q = 'q', .R = 'r', .S = 's', .T = 't', .U = 'u', .V = 'v', .W = 'w', .X = 'x', .Y = 'y', .Z = 'z', }
	CharListUpper := map[ray.KeyboardKey]u8 { .A = 'A', .B = 'B', .C = 'C', .D = 'D', .E = 'E', .F = 'F', .G = 'G', .H = 'H', .I = 'I', .J = 'J', .K = 'K', .L = 'L', .M = 'M', .N = 'N', .O = 'O', .P = 'P', .Q = 'Q', .R = 'R', .S = 'S', .T = 'T', .U = 'U', .V = 'V', .W = 'W', .X = 'X', .Y = 'Y', .Z = 'Z', }

	if ray.IsKeyPressed(.BACKSPACE) && (len(Target^) != 0) {
		ordered_remove(Target, len(Target^) - 1)
	}
	else { 
		CharToAdd : u8 = 0
		for Character in ray.KeyboardKey.A..ray.KeyboardKey.Z {
			if ray.IsKeyPressed(Character) {
				if ray.IsKeyDown(.LEFT_SHIFT) {
					CharToAdd = CharListUpper[Character]
				}
				else {
					CharToAdd = CharList[Character]
				}
			}
		}
		if CharToAdd != 0 {
			append(Target, CharToAdd)
		}
	}
}

BluesBash_WindowWidth :: 1280
BluesBash_WindowHeight :: 720

WindowWidth :: BluesBash_WindowWidth
WindowHeight :: BluesBash_WindowHeight + 200
// extra height is for settings area

ButtonList : [dynamic]button_def

UISelectInfo : struct {
	Index : int,
	Type : enum {
		Button,
		TextArea,
	},
}

InfoSelectInfo : struct {
	uiid : i32,
}

main :: proc() {
	ray.InitWindow(WindowWidth, WindowHeight, "Blues Bash: UI Maker")
	ray.SetTargetFPS(60)
	{
		ray.BeginDrawing()
		ray.DrawText("Loading all PPP files...", WindowWidth/2 - (ray.MeasureText("Loading all PPP files...", 40))/2, WindowHeight/2, 40, ray.BLACK)
		ray.EndDrawing()
	}

	UISelectInfo.Index = -1

	AnimationMap = make(map[string]animation)
	defer delete(AnimationMap)
	// load all *.ppp files
	{
		ProcessedHandle, HandleErrno := os.open("resources/processed")
		assert(HandleErrno == os.ERROR_NONE)
		defer os.close(ProcessedHandle)

		ProcessedInfo, InfoErrno := os.read_dir(ProcessedHandle, 0)
		assert(InfoErrno == os.ERROR_NONE)
		defer delete(ProcessedInfo)

		for Info in ProcessedInfo {
			LoadPppFile(Info.fullpath)
		}
	}

	ButtonList = make([dynamic]button_def)
	IsDragging_Movement := false
	IsDragging_Resize := false

	for !ray.WindowShouldClose() {
		MousePos := ray.GetMousePosition()

		// NOTE(Roskuski): Place Button
		if ray.IsKeyPressed(.F1) {
			Button := button_def{
				HitRect = {MousePos.x, MousePos.y, 50, 50}, 
				GraphicRect = {MousePos.x, MousePos.y, 50, 50},
				GraphicKey = make([dynamic]u8),
				Key = make([dynamic]u8),
			}
			append(&Button.Key, fmt.tprintf("Button%d", len(ButtonList) + 1))
			Offset : int = 0
			for Index := 0; Index < len(ButtonList); Index += 1 {
				if strings.compare(strings.clone_from_bytes(Button.Key[:], context.temp_allocator), strings.clone_from_bytes(ButtonList[Index].Key[:], context.temp_allocator)) == 0 {
					Offset += 1
					clear(&Button.Key)
					append(&Button.Key, fmt.tprintf("Button%d", len(ButtonList) + 1 + Offset))
					Index = 0
				}
			}
			append(&Button.GraphicKey, "None")
			append(&ButtonList, Button)
		}

		// NOTE(Roskuski): Left Click Press => Select element for info pane. Selection is based off of Hit Rect
		if ray.IsMouseButtonPressed(.LEFT) && ray.CheckCollisionPointRec(MousePos, {0, 0, BluesBash_WindowWidth, BluesBash_WindowHeight}) {
			DidHit := false
			for Button, Index in ButtonList {
				if ray.IsKeyDown(.LEFT_CONTROL) && ray.CheckCollisionPointRec(MousePos, Button.GraphicRect) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && ray.CheckCollisionPointRec(MousePos, Button.HitRect) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
				}
			}

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// NOTE(Roskuski): Left Click Hold => move element Graphic + Hit;
		if IsDragging_Movement {
			if ray.IsMouseButtonUp(.LEFT) do IsDragging_Movement = false
			if UISelectInfo.Type == .Button {
				MouseDelta := ray.GetMouseDelta()
				if ray.IsKeyDown(.LEFT_ALT) {
					ButtonList[UISelectInfo.Index].HitRect.x += MouseDelta.x
					ButtonList[UISelectInfo.Index].HitRect.y += MouseDelta.y
				}
				else if ray.IsKeyDown(.LEFT_CONTROL) {
					ButtonList[UISelectInfo.Index].GraphicRect.x += MouseDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.y += MouseDelta.y
				}
				else {
					ButtonList[UISelectInfo.Index].HitRect.x += MouseDelta.x
					ButtonList[UISelectInfo.Index].HitRect.y += MouseDelta.y
					ButtonList[UISelectInfo.Index].GraphicRect.x += MouseDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.y += MouseDelta.y
				}
			}
		}

		// NOTE(Roskuski): Right Click => Select element
		if ray.IsMouseButtonPressed(.RIGHT) && ray.CheckCollisionPointRec(MousePos, {0, 0, BluesBash_WindowWidth, BluesBash_WindowHeight}) {
			DidHit := false
			for Button, Index in ButtonList {
				if ray.IsKeyDown(.LEFT_CONTROL) && ray.CheckCollisionPointRec(MousePos, Button.GraphicRect) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Resize = true
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && ray.CheckCollisionPointRec(MousePos, Button.HitRect) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Resize = true
				}
			}

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// NOTE(Roskuski): Right Click Hold => resize Hit
		if IsDragging_Resize {
			if ray.IsMouseButtonUp(.RIGHT) do IsDragging_Resize = false
			if UISelectInfo.Type == .Button {
				ResizeDelta := ray.GetMouseDelta()

				if ray.IsKeyDown(.LEFT_SHIFT) {
					ReferenceCoord := ResizeDelta.y
					if (abs(ResizeDelta.x) > abs(ResizeDelta.y))  {
						ReferenceCoord = ResizeDelta.x
					}
					if (ReferenceCoord > 0) {
						ResizeDelta.x = max(ResizeDelta.x, ResizeDelta.y)
						ResizeDelta.y = max(ResizeDelta.x, ResizeDelta.y)
					}
					else {
						ResizeDelta.x = min(ResizeDelta.x, ResizeDelta.y)
						ResizeDelta.y = min(ResizeDelta.x, ResizeDelta.y)
					}
				}

				if ray.IsKeyDown(.LEFT_ALT) {
					ButtonList[UISelectInfo.Index].HitRect.width += ResizeDelta.x
					ButtonList[UISelectInfo.Index].HitRect.height += ResizeDelta.y
				}
				else if ray.IsKeyDown(.LEFT_CONTROL) {
					ButtonList[UISelectInfo.Index].GraphicRect.width += ResizeDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.height += ResizeDelta.y
				}
				else {
					ButtonList[UISelectInfo.Index].HitRect.width += ResizeDelta.x
					ButtonList[UISelectInfo.Index].HitRect.height += ResizeDelta.y
					ButtonList[UISelectInfo.Index].GraphicRect.width += ResizeDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.height += ResizeDelta.y
				}

				if ButtonList[UISelectInfo.Index].HitRect.width <= 0 do ButtonList[UISelectInfo.Index].HitRect.width = 1
				if ButtonList[UISelectInfo.Index].HitRect.height <= 0 do ButtonList[UISelectInfo.Index].HitRect.height = 1
				if ButtonList[UISelectInfo.Index].GraphicRect.width <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.width = 1
				if ButtonList[UISelectInfo.Index].GraphicRect.height <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.height = 1
			}
		}

		// NOTE(Roskuski): Ctrl + A => resize Graphic under mouse to screen
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.A) && ray.IsKeyDown(.LEFT_CONTROL) {
			ButtonList[UISelectInfo.Index].GraphicRect.x = 0
			ButtonList[UISelectInfo.Index].GraphicRect.y = 0
			ButtonList[UISelectInfo.Index].GraphicRect.width = BluesBash_WindowWidth
			ButtonList[UISelectInfo.Index].GraphicRect.height = BluesBash_WindowHeight
		}

		// NOTE(Roskuski): Ctrl + E => resize Graphic to size of souce graphic
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.E) && ray.IsKeyDown(.LEFT_CONTROL) {
			Animation, Ok := AnimationMap[strings.clone_from_bytes(ButtonList[UISelectInfo.Index].GraphicKey[:], context.temp_allocator)]
			if Ok {
				ButtonList[UISelectInfo.Index].GraphicRect.width = f32(Animation.Frames[0].Graphic.width)
				ButtonList[UISelectInfo.Index].GraphicRect.height = f32(Animation.Frames[0].Graphic.height)
			}
		}

		// NOTE(Roskuski): Del => removes selected element
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.DELETE) {
			if UISelectInfo.Type == .Button {
				delete(ButtonList[UISelectInfo.Index].Key)
				delete(ButtonList[UISelectInfo.Index].GraphicKey)
				ordered_remove(&ButtonList, UISelectInfo.Index)
				UISelectInfo.Index = -1
			}
		}

		// NOTE(Roskuski): ArrowKeys => Nudge element
		// + Space => * factor of 10
		// + Alt => target hit
		// + crtl => target graphic
		// + shift => target width/height
		if UISelectInfo.Index != -1 {
			if UISelectInfo.Type == .Button {
				MoveDelta : linalg.Vector2f32 = {0, 0}
				if ray.IsKeyPressed(.UP) {
					MoveDelta.y = -1 
				}
				else if ray.IsKeyPressed(.DOWN) {
					MoveDelta.y = 1 
				}
				else if ray.IsKeyPressed(.LEFT) {
					MoveDelta.x = -1
				}
				else if ray.IsKeyPressed(.RIGHT) {
					MoveDelta.x = 1
				}

				if ray.IsKeyDown(.SPACE) {
					MoveDelta *= 10
				}

				if ray.IsKeyDown(.LEFT_ALT) && ray.IsKeyDown(.LEFT_SHIFT) {
					ButtonList[UISelectInfo.Index].HitRect.width += MoveDelta.x
					ButtonList[UISelectInfo.Index].HitRect.height += MoveDelta.y
				}
				else if ray.IsKeyDown(.LEFT_ALT) {
					ButtonList[UISelectInfo.Index].HitRect.x += MoveDelta.x
					ButtonList[UISelectInfo.Index].HitRect.y += MoveDelta.y
				}
				else if ray.IsKeyDown(.LEFT_CONTROL) && ray.IsKeyDown(.LEFT_SHIFT) {
					ButtonList[UISelectInfo.Index].GraphicRect.width += MoveDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.height += MoveDelta.y
				}
				else if ray.IsKeyDown(.LEFT_CONTROL) {
					ButtonList[UISelectInfo.Index].GraphicRect.x += MoveDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.y += MoveDelta.y
				}
				else if ray.IsKeyDown(.LEFT_SHIFT) {
					ButtonList[UISelectInfo.Index].HitRect.width += MoveDelta.x
					ButtonList[UISelectInfo.Index].HitRect.height += MoveDelta.y
					ButtonList[UISelectInfo.Index].GraphicRect.width += MoveDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.height += MoveDelta.y
				}
				else {
					ButtonList[UISelectInfo.Index].HitRect.x += MoveDelta.x
					ButtonList[UISelectInfo.Index].HitRect.y += MoveDelta.y
					ButtonList[UISelectInfo.Index].GraphicRect.x += MoveDelta.x
					ButtonList[UISelectInfo.Index].GraphicRect.y += MoveDelta.y
				}
			}
		}

		// NOTE(Roskuski): CTRL + O => Open Existing File
		if ray.IsKeyDown(.LEFT_CONTROL) && ray.IsKeyPressed(.O) {
			CurrentDir := os.get_current_directory()
			Param := win32.Open_File_Name_A{
				struct_size = size_of(win32.Open_File_Name_A),
				hwnd_owner = nil,
				instance = nil,
				filter = "UI Maker Files\x00*.uim\x00\x00",
				custom_filter = nil,
				max_cust_filter = 0,
				filter_index = 1,
				file = cstring(raw_data(make([]u8, 256))),
				max_file = 256,
				file_title = nil,
				max_file_title = 0,
				initial_dir = strings.clone_to_cstring(CurrentDir),
				title = "What UI Maker file shall I open?",
				flags = 0,
				file_offset = 0,
				file_extension = 0,
				def_ext = "uim",
				cust_data = 0,
				hook = nil,
				template_name = nil,
				pv_reserved = nil,
				dw_reserved = 0,
				flags_ex = 0,
			}
			defer delete(Param.file)
			defer delete(CurrentDir)
			defer delete(Param.initial_dir)
			win32.get_open_file_name_a(&Param)
			OpenFromFile(Param.file)
		}
		// NOTE(Roskuski): CTRL + S => Save Existing File
		if ray.IsKeyDown(.LEFT_CONTROL) && ray.IsKeyPressed(.S) {
			CurrentDir := os.get_current_directory()
			Param := win32.Open_File_Name_A{
				struct_size = size_of(win32.Open_File_Name_A),
				hwnd_owner = nil,
				instance = nil,
				filter = "UI Maker Files\x00*.uim\x00\x00",
				custom_filter = nil,
				max_cust_filter = 0,
				filter_index = 1,
				file = cstring(raw_data(make([]u8, 256))),
				max_file = 256,
				file_title = nil,
				max_file_title = 0,
				initial_dir = strings.clone_to_cstring(CurrentDir),
				title = "Where shall I save the UI File?",
				flags = 0,
				file_offset = 0,
				file_extension = 0,
				def_ext = "uim",
				cust_data = 0,
				hook = nil,
				template_name = nil,
				pv_reserved = nil,
				dw_reserved = 0,
				flags_ex = 0,
			}
			defer delete(Param.file)
			defer delete(CurrentDir)
			defer delete(Param.initial_dir)
			win32.get_save_file_name_a(&Param)
			SaveToFile(Param.file, Param.file_offset, Param.file_extension)
		}

		ray.BeginDrawing()

		ray.BeginScissorMode(0, 0, BluesBash_WindowWidth, BluesBash_WindowHeight)
		{ // Draw UI scene
			ray.ClearBackground(ray.BLACK)
			for ButtonDef in ButtonList {
				Animation, Ok := AnimationMap[strings.clone_from_bytes(ButtonDef.GraphicKey[:], context.temp_allocator)]
				if Ok {
					ray.DrawTextureQuad(Animation.Frames[0].Graphic, {1.0, 1.0}, {0, 0}, ButtonDef.GraphicRect, ray.RAYWHITE)
				}
				else { // Draw a purple placeholder box
					ray.DrawRectangleRec(ButtonDef.GraphicRect, ray.PURPLE)
				}
				Color := ray.RED
				Color.a = 0xff/2
				ray.DrawRectangleRec(ButtonDef.HitRect, Color)
			}
		}
		ray.EndScissorMode()

		ray.BeginScissorMode(0, BluesBash_WindowHeight, WindowWidth, WindowHeight - BluesBash_WindowHeight)
		SettingsCamera :: ray.Camera2D{
			offset = {0, BluesBash_WindowHeight},
			target = {0, 0},
			rotation = 0,
			zoom = 1,
		}
		ray.BeginMode2D(SettingsCamera)
		ray.SetMouseOffset(i32(-SettingsCamera.offset.x), i32(-SettingsCamera.offset.y))
		MousePos = ray.GetMousePosition()
		{
			ray.ClearBackground(ray.GRAY)

			if UISelectInfo.Index != -1 {
				if UISelectInfo.Type == .Button {
					ClickHitInfo := false
					Button := &ButtonList[UISelectInfo.Index]

					// @TODO(Roskuski): I beam cursor when editing fields.
					DoInfoTextArea({5, 5, 300, 20}, "Button Name:", MousePos, &ClickHitInfo)
					if DoInfoTextArea({5, 25, 300, 20}, Button.Key[:], MousePos, &ClickHitInfo) {
						if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.Key)
					}

					DoInfoTextArea({5, 45, 300, 20}, "Graphic Name:", MousePos, &ClickHitInfo)
					if DoInfoTextArea({5, 65, 300, 20}, Button.GraphicKey[:], MousePos, &ClickHitInfo) {
						if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.GraphicKey)
					}

					// @TODO(Roskuski) Allow typing into these number values
					DoInfoTextArea({310, 5, 300, 20}, "Hit Rect Params:", MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 25, 300, 20}, fmt.tprintf("x: %f", Button.HitRect.x), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 45, 300, 20}, fmt.tprintf("y: %f", Button.HitRect.y), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 65, 300, 20}, fmt.tprintf("width: %f", Button.HitRect.width), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 85, 300, 20}, fmt.tprintf("height: %f", Button.HitRect.height), MousePos, &ClickHitInfo)

					DoInfoTextArea({615, 5, 300, 20}, "Graphic Rect Params:", MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 25, 300, 20}, fmt.tprintf("x: %f", Button.GraphicRect.x), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 45, 300, 20}, fmt.tprintf("y: %f", Button.GraphicRect.y), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 65, 300, 20}, fmt.tprintf("width: %f", Button.GraphicRect.width), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 85, 300, 20}, fmt.tprintf("height: %f", Button.GraphicRect.height), MousePos, &ClickHitInfo)

					if ray.IsMouseButtonPressed(.LEFT) && ClickHitInfo == false {
						InfoSelectInfo.uiid = -1
					}
				}
				else if UISelectInfo.Type == .TextArea {
					assert(false)
				}
			}
		}
		ray.SetMouseOffset(0, 0)
		ray.EndMode2D()
		ray.EndScissorMode()

		ray.EndDrawing()
	}
}

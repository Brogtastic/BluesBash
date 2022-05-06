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
import "core:math"
import "core:c"
import "core:sys/win32"
import "core:runtime"

button_def :: struct {
	Key : [dynamic]u8,
	HitRect : ray.Rectangle,
	HitRotation : f32,
	GraphicKey : [dynamic]u8,
	GraphicRect : ray.Rectangle,
	GraphicRotation : f32, 
}

text_area_def :: struct {
	Key : [dynamic]u8,
	Buffer : [dynamic]u8,
	BufferMaxSize : i32,
	FontSize : i32,
	HitRect : ray.Rectangle,
	GraphicRect : ray.Rectangle,
	GraphicKey : [dynamic]u8,
}

right_click_mode :: enum {
	Resize,
	Rotate,
}

frame :: struct {
	FrameLength : i32,
	Graphic : ray.Texture2D,
}

animation_gutted :: struct {
	Frame : ray.Texture2D,
}

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

BluesBash_WindowWidth :: 1280
BluesBash_WindowHeight :: 720

WindowWidth :: BluesBash_WindowWidth
WindowHeight :: BluesBash_WindowHeight + 200
// extra height is for settings area

IBEAM_COLOR : ray.Color : {0xff, 0xff, 0xff, (0xff * 3)/4}

AnimationMap : map[string]animation_gutted
ButtonList : [dynamic]button_def
TextAreaList : [dynamic]text_area_def

file_version :: enum u32 { // NOTE: Add a new entry every time the format of the binary file changes.
	Buttons = 0, // First version, has button support
	TextAreas, // Adds support for text areas

	LatestPlusOne, // NOTE do not remove, always keep at end.
}

FILE_LATEST_VERSION :: file_version(u32(file_version.LatestPlusOne) - 1)
FILE_MAGIC_NUMBER :: [4]u8{'U', 'I', 'M', 0}

LoadPppFile :: proc(PppPath : string) {
	PppHandle, PppError := os.open(PppPath, os.O_RDONLY)
	if PppError != os.ERROR_NONE {
		fmt.eprintf("[LoadPppFile] Failed to open handle for file \"%s\"", PppPath)
	}
	else {
		KeyLen : i32
		os.read_ptr(PppHandle, &KeyLen, size_of(KeyLen))

		Key := make([]u8, KeyLen)
		defer delete(Key)
		os.read(PppHandle, Key[:]) 

		Animation : animation_gutted = ---

		os.seek(PppHandle, size_of(i32) * 3, win32.FILE_CURRENT)
		PathLen : i32
		os.read_ptr(PppHandle, &PathLen, size_of(PathLen))
		Path := make([]u8, PathLen + 1)
		defer delete(Path)
		os.read(PppHandle, Path)
		Path[PathLen] = 0
		Animation.Frame = ray.LoadTexture(cstring(&Path[0]))

		AnimationMap[strings.clone_from_bytes(Key)] = Animation
		fmt.printf("[LoadPppFile] Loading of \"%s\"'s first frame was successful\n", PppPath)
	}
}

/* File Format
Header:
 - Magic Number "UIM\0"
 - Version Number : u32
 - KeyLen : i32
 - Key : [KeyLen]u8
 - Number of buttons : i32
 - Buttons
 - Number of textareas : i32
 - TextAreas

Button:
 - KeyLen : i32
 - Key : [KeyLen]u8
 - Graphic KeyLen : i32
 - Graphic Key : [Graphic KeyLen]u8
 - HitRect : ray.Rectangle
 - HitRotation : f32
 - GraphicRect : ray.Rectangle
 - GraphicRotation : f32

TextArea:
 - KeyLen : i32,
 - Key : [KeyLen]u8
 - GraphicKeyLen : i32
 - GraphicKey : [GraphicKeyLen]u8
 - BufferMaxSize : i32
 - FontSize : i32
 - HitRect : ray.Rectangle
 - GraphicRect : ray.Rectangle
*/

WriteObject :: #force_inline proc(Handle : os.Handle, Object : ^$T) {
	os.write_ptr(Handle, Object, size_of(Object^))
}

ReadObject :: #force_inline proc(Handle : os.Handle, Object : ^$T) {
	os.read_ptr(Handle, Object, size_of(Object^))
}

// @TODO(Roskuski) validate that we do not have dublicate Keys in the list. If we have duplicate names then the Game's map will not be populated properly.
// @TODO(Roskuski) Move everything over to WriteObject
// @TODO(Roskuski) with the current opening mode os.O_WRONLY | os.O_CREATE the file that is writen into retains information from the pervious version if new the information is shorter. This is currently not a problem, because this vestigal data is never read. I don't like the idea of having dead data in files though.
SaveToFile :: proc(Path : cstring, FileNameOffset, FileExtentionOffset : u16) {
	Path := string(Path)
	Handle, HandleError := os.open(Path, os.O_WRONLY | os.O_CREATE)
	if (HandleError == os.ERROR_NONE) {
		FileMagicNumber := FILE_MAGIC_NUMBER
		FileVersionNumber := FILE_LATEST_VERSION
		os.write(Handle, FileMagicNumber[:])
		os.write_ptr(Handle, &FileVersionNumber, size_of(FileVersionNumber))
		Key := Path[FileNameOffset:FileExtentionOffset-1]
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
			os.write_ptr(Handle, &Button.HitRotation, size_of(Button.HitRotation))
			os.write_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))
			os.write_ptr(Handle, &Button.GraphicRotation, size_of(Button.GraphicRotation))
		}

		TextAreaCount := i32(len(TextAreaList))
		WriteObject(Handle, &TextAreaCount)

		for TextArea in &TextAreaList {
			KeyLen := i32(len(TextArea.Key))
			WriteObject(Handle, &KeyLen)
			os.write(Handle, TextArea.Key[:])

			KeyLen = i32(len(TextArea.GraphicKey))
			WriteObject(Handle, &KeyLen)
			os.write(Handle, TextArea.GraphicKey[:])

			WriteObject(Handle, &TextArea.BufferMaxSize)
			WriteObject(Handle, &TextArea.FontSize)
			WriteObject(Handle, &TextArea.HitRect)
			WriteObject(Handle, &TextArea.GraphicRect)
		}
		os.close(Handle)
	}
	else do fmt.printf("[SaveToFile]: There was an error opening \"%s\"(%d)\n", Path, HandleError)
}

OpenFromFile_Unversioned :: proc(Handle : os.Handle) {
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
		os.read_ptr(Handle, &Button.HitRotation, size_of(Button.HitRotation))
		os.read_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))
		os.read_ptr(Handle, &Button.GraphicRotation, size_of(Button.GraphicRotation))

		append(&ButtonList, Button)
	}
}

OpenFromFile_Buttons :: OpenFromFile_Unversioned

OpenFromFile_TextAreas :: proc(Handle : os.Handle) {
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
		os.read_ptr(Handle, &Button.HitRotation, size_of(Button.HitRotation))
		os.read_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))
		os.read_ptr(Handle, &Button.GraphicRotation, size_of(Button.GraphicRotation))

		append(&ButtonList, Button)
	}

	TextAreaCount : i32
	ReadObject(Handle, &TextAreaCount)
	for Index in 0..<TextAreaCount {
		TextArea : text_area_def
		using TextArea

		KeyLen : i32
		ReadObject(Handle, &KeyLen)
		TextArea.Key = make([dynamic]u8, KeyLen)
		os.read(Handle, Key[:])

		GraphicKeyLen : i32
		ReadObject(Handle, &GraphicKeyLen)
		TextArea.GraphicKey = make([dynamic]u8, GraphicKeyLen)
		os.read(Handle, GraphicKey[:])

		ReadObject(Handle, &BufferMaxSize)
		ReadObject(Handle, &FontSize)
		ReadObject(Handle, &HitRect)
		ReadObject(Handle, &GraphicRect)

		append(&TextAreaList, TextArea)
	}
}

OpenFromFile :: proc(Path : cstring) {
	for Button in &ButtonList {
		delete(Button.Key)
		delete(Button.GraphicKey)
	}
	clear(&ButtonList)

	for TextArea in &TextAreaList {
		delete(TextArea.Key)
		delete(TextArea.GraphicKey)
		delete(TextArea.Buffer)
	}
	clear(&TextAreaList)

	Path := string(Path)
	Handle, HandleError := os.open(Path, os.O_RDONLY)
	if HandleError != os.ERROR_NONE do fmt.printf("[OpenFromFile]: There was an error opening \"%s\"(%d)\n", Path, HandleError)
	
	MagicNumber : [4]u8
	NumRead, ReadError := os.read(Handle, MagicNumber[:])
	if NumRead == 4 && ReadError == os.ERROR_NONE {
		if MagicNumber == FILE_MAGIC_NUMBER {
			FileVersion : file_version
			NumRead, ReadError = os.read_ptr(Handle, cast(^u32)(&FileVersion), size_of(FileVersion))
			if NumRead == size_of(FileVersion) && ReadError == os.ERROR_NONE {
				switch (FileVersion) {
					case .Buttons: {
						OpenFromFile_Buttons(Handle)
					}
					case .TextAreas: {
						OpenFromFile_TextAreas(Handle)
					}
					case .LatestPlusOne: fallthrough
					case: { // NOTE: handles all invalid version numbers.
						fmt.printf("[OpenFromFile]: I opened a file from the future! Latest version is %d(file_version.%s), this file has version %d.\n", FILE_LATEST_VERSION, FILE_LATEST_VERSION, FileVersion)
					}
				}
			}
			else do fmt.printf("[OpenFromFile]: Failed to read file version!\n")
		}
		else {
			os.seek(Handle, 0, win32.FILE_BEGIN)
			OpenFromFile_Unversioned(Handle)
		}
	}
	else do fmt.printf("[OpenFromFile]: Failed to read file magic number!\n")
}

// returns true if we are the active info element
DoInfoTextArea_string :: proc(Rect : ray.Rectangle, String : string, MousePos : linalg.Vector2f32, ClickHitInfo : ^bool = nil, CallLoc := #caller_location) -> (Result : bool) {
	Result = false
	ray.DrawRectangleRec(Rect, ray.BLACK)
	CString := strings.clone_to_cstring(String, context.temp_allocator)

	if ClickHitInfo != nil {
		if ray.CheckCollisionPointRec(MousePos, Rect) {
			if ray.IsMouseButtonPressed(.LEFT) {
				InfoSelectInfo.uiid  = CallLoc.line
				if ClickHitInfo^ != true do ClickHitInfo^ = true
			}
		}
	}

	if InfoSelectInfo.uiid == CallLoc.line do Result = true

	TextColor := ray.WHITE
	if Result do TextColor = ray.GREEN
	TextLength := ray.MeasureText(CString, 20)
	XOffset : f32 = 0
	for f32(TextLength) - XOffset > Rect.width do XOffset += Rect.width/4
	ray.BeginScissorMode(i32(Rect.x), i32(Rect.y) + BluesBash_WindowHeight, i32(Rect.width), i32(Rect.height))
	ray.DrawText(CString, i32(Rect.x - XOffset), i32(Rect.y), 20, TextColor)
	if Result do ray.DrawRectangleRec({Rect.x - XOffset + f32(TextLength) + 3, Rect.y + 1 , 2, 18}, IBEAM_COLOR)
	ray.EndScissorMode()

	return Result
}

DoInfoTextArea_u8 :: proc(Rect : ray.Rectangle, String : []u8, MousePos : linalg.Vector2f32, ClickHitInfo : ^bool = nil, CallLoc := #caller_location) -> (Result : bool) {
	return DoInfoTextArea_string(Rect, strings.clone_from_bytes(String, context.temp_allocator), MousePos, ClickHitInfo, CallLoc)
}

DoInfoTextArea :: proc {
	DoInfoTextArea_u8,
	DoInfoTextArea_string,
}

DoInfoButton :: proc(Rect : ray.Rectangle, String : string, MousePos : linalg.Vector2f32, ClickHitInfo : ^bool, CallLoc := #caller_location) -> (Result : bool) {
	Result = false
	AccentColor := ray.WHITE

	if ray.CheckCollisionPointRec(MousePos, Rect) {
		if ray.IsMouseButtonPressed(.LEFT) {
			InfoSelectInfo.uiid  = CallLoc.line
			if ClickHitInfo^ != true do ClickHitInfo^ = true
			Result = true
		}
	}

	if (InfoSelectInfo.uiid == CallLoc.line) && ray.CheckCollisionPointRec(MousePos, Rect) && ray.IsMouseButtonDown(.LEFT) {
		AccentColor = ray.GREEN
	}

	InsideRect := ray.Rectangle{Rect.x + 1, Rect.y + 1, Rect.width - 2, Rect.height - 2}
	ray.DrawRectangleRec(Rect, ray.Color{0xdd, 0xdd, 0xdd, 0xff})
	ray.DrawRectangleRec(InsideRect, ray.BLACK)
	CString := strings.clone_to_cstring(String, context.temp_allocator)

	ray.DrawText(CString, i32(InsideRect.x), i32(Rect.y), 20, AccentColor)

	return
}

Swap :: proc(List : [dynamic]$T, IndexA, IndexB : int) {
	Temp := List[IndexA]
	List[IndexA] = List[IndexB]
	List[IndexB] = Temp
}

KeyboardTypeInput :: proc(Target : ^[dynamic]u8, MaxLen : i32 = 0, Allocator := context.allocator) {
	if ray.IsKeyPressed(.BACKSPACE) && (len(Target^) != 0) {
		ordered_remove(Target, len(Target^) - 1)
	}
	else {
		for UniChar := ray.GetCharPressed(); UniChar != 0; UniChar = ray.GetCharPressed() {
			if (UniChar < 255) {
				if (MaxLen == 0) || (MaxLen > i32(len(Target^))) {
					append(Target, u8(UniChar))
				}
			}
		}
	}
	if MaxLen != 0 do for i32(len(Target^)) > MaxLen {
		ordered_remove(Target, len(Target^) - 1)
	}
}

KeyboardNumberInput :: proc(Target : ^i32) {
	if ray.IsKeyPressed(.BACKSPACE) && (Target^ != 0) {
		Target^ /= 10
	}
	else {
		for UniChar := ray.GetCharPressed(); UniChar != 0; UniChar = ray.GetCharPressed() {
			if (UniChar >= '0') && (UniChar <= '9') {
				Target^ *= 10
				Target^ += i32(UniChar - '0')
			}
		}
	}
}

CheckCollisionPointRotatedRec :: proc (Point : linalg.Vector2f32, Rect : ray.Rectangle, Rotation : f32) -> (Result : bool) {
	Rot := Rotation * linalg.RAD_PER_DEG
	RotationMatrix : linalg.Matrix2x2f32 = {
		linalg.cos(Rot), -linalg.sin(Rot),
		linalg.sin(Rot), linalg.cos(Rot),
	}
	RectPoints := [4]linalg.Vector2f32{
		{0         , 0},
		{Rect.width, 0},
		{0         , Rect.height},
		{Rect.width, Rect.height},
	}
	TransformedPoints : [4]linalg.Vector2f32
	for Point in &RectPoints do Point -= {Rect.width/2, Rect.height/2}
	for Point, Index in RectPoints do TransformedPoints[Index] = linalg.matrix_mul_vector(RotationMatrix, Point)
	for Point in &TransformedPoints do Point += {Rect.x, Rect.y}

	Result = false
	Result = ray.CheckCollisionPointTriangle(Point, TransformedPoints[0], TransformedPoints[1], TransformedPoints[2]) 
	Result = Result || ray.CheckCollisionPointTriangle(Point, TransformedPoints[1], TransformedPoints[2], TransformedPoints[3])
	return Result
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

	AnimationMap = make(map[string]animation_gutted)
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
	IsDragging_Rotate := false
	RightClickMode := right_click_mode.Resize

	for !ray.WindowShouldClose() {
		MousePos := ray.GetMousePosition()

		// @ApplicationRef F1 => Place Button
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

		// @ApplicationRef F2 => Place TextArea
		if ray.IsKeyPressed(.F2) {
			TextArea := text_area_def{
				BufferMaxSize = 20,
				FontSize = 20,
				HitRect = {MousePos.x, MousePos.y, 50, 20},
				GraphicRect = {MousePos.x, MousePos.y, 50, 20},
				Key = make([dynamic]u8),
				GraphicKey = make([dynamic]u8),
				Buffer = make([dynamic]u8),
			}
			append(&TextArea.Key, fmt.tprintf("TextArea%d", len(TextAreaList) + 1))
			Offset : int = 0
			for Index := 0; Index < len(TextAreaList); Index += 1 {
				if strings.compare(strings.clone_from_bytes(TextArea.Key[:], context.temp_allocator), strings.clone_from_bytes(TextAreaList[Index].Key[:], context.temp_allocator)) == 0 {
					Offset += 1
					clear(&TextArea.Key)
					append(&TextArea.Key, fmt.tprintf("TextArea%d", len(TextAreaList) + 1 + Offset))
					Index = 0
				}
			}
			append(&TextArea.GraphicKey, "None")
			append(&TextArea.Buffer, "Sample Text")
			append(&TextAreaList, TextArea)
		}

		// @ApplicationRef Left Click Press => Select element for info pane. Selection is based off of Hit Rect
		// @TODO(Roskuski): this function has duplicated code with Right Click Press.
		if ray.IsMouseButtonPressed(.LEFT) && ray.CheckCollisionPointRec(MousePos, {0, 0, BluesBash_WindowWidth, BluesBash_WindowHeight}) {
			DidHit := false
			for Button, Index in ButtonList {
				if ray.IsKeyDown(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, Button.GraphicRect, Button.GraphicRotation) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, Button.HitRect, Button.HitRotation) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
				}
			}

			for TextArea, Index in TextAreaList {
				if ray.IsKeyDown(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, TextArea.GraphicRect, 0) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .TextArea
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
					RightClickMode = .Resize
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, TextArea.HitRect, 0) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .TextArea
					InfoSelectInfo.uiid = -1
					DidHit = true
					IsDragging_Movement = true
					RightClickMode = .Resize
				}
			}

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// @ApplicationRef Left Click Hold => move element Graphic + Hit;
		if IsDragging_Movement {
			if ray.IsMouseButtonUp(.LEFT) do IsDragging_Movement = false
			switch UISelectInfo.Type {
				case .Button:
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
				case .TextArea:
					MouseDelta := ray.GetMouseDelta()
					if ray.IsKeyDown(.LEFT_ALT) {
						TextAreaList[UISelectInfo.Index].HitRect.x += MouseDelta.x
						TextAreaList[UISelectInfo.Index].HitRect.y += MouseDelta.y
					}
					else if ray.IsKeyDown(.LEFT_CONTROL) {
						TextAreaList[UISelectInfo.Index].GraphicRect.x += MouseDelta.x
						TextAreaList[UISelectInfo.Index].GraphicRect.y += MouseDelta.y
					}
					else {
						TextAreaList[UISelectInfo.Index].HitRect.x += MouseDelta.x
						TextAreaList[UISelectInfo.Index].HitRect.y += MouseDelta.y
						TextAreaList[UISelectInfo.Index].GraphicRect.x += MouseDelta.x
						TextAreaList[UISelectInfo.Index].GraphicRect.y += MouseDelta.y
					}
			}
		}

		// @ApplicationRef Right Click => Select element
		if ray.IsMouseButtonPressed(.RIGHT) && ray.CheckCollisionPointRec(MousePos, {0, 0, BluesBash_WindowWidth, BluesBash_WindowHeight}) {
			DidHit := false
			for Button, Index in ButtonList {
				if ray.IsKeyDown(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, Button.GraphicRect, Button.GraphicRotation) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					switch (RightClickMode) {
						case .Resize: IsDragging_Resize = true
						case .Rotate: IsDragging_Rotate = true
					}
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, Button.HitRect, Button.HitRotation) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .Button
					InfoSelectInfo.uiid = -1
					DidHit = true
					switch (RightClickMode) {
						case .Resize: IsDragging_Resize = true
						case .Rotate: IsDragging_Rotate = true
					}
				}
			}

			for TextArea, Index in TextAreaList {
				if ray.IsKeyDown(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, TextArea.GraphicRect, 0) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .TextArea
					InfoSelectInfo.uiid = -1
					DidHit = true
					switch (RightClickMode) {
						case .Resize: IsDragging_Resize = true
						case .Rotate: IsDragging_Rotate = true
					}
					RightClickMode = .Resize
				}
				else if ray.IsKeyUp(.LEFT_CONTROL) && CheckCollisionPointRotatedRec(MousePos, TextArea.HitRect, 0) {
					UISelectInfo.Index = Index
					UISelectInfo.Type = .TextArea
					InfoSelectInfo.uiid = -1
					DidHit = true
					switch (RightClickMode) {
						case .Resize: IsDragging_Resize = true
						case .Rotate: IsDragging_Rotate = true
					}
					RightClickMode = .Resize
				}
			}

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// @ApplicationRef Right Click Hold + Right Click Mode "Resize" => Reize Element (More Follows...)
		// @ApplicationRef  + shift => Proportional resize
		// @ApplicationRef  + alt => Target just Hit Rect
		// @ApplicationRef  + ctrl => Target just Graphic Rect
		// @TODO(Roskuski): This feature's code path has a lot of duplicated code, and the flow is diffcult to read.
		// When I have some free time, I should simplify this.
		if IsDragging_Resize {
			if ray.IsMouseButtonUp(.RIGHT) do IsDragging_Resize = false
			ResizeDelta := ray.GetMouseDelta()

			Proportional := false
			TargetHit := false
			TargetGraphic := false
			if ray.IsKeyDown(.LEFT_SHIFT) do Proportional = true
			if ray.IsKeyDown(.LEFT_ALT) do TargetHit = true
			if ray.IsKeyDown(.LEFT_CONTROL) do TargetGraphic = true
			if !TargetHit && !TargetGraphic {
				TargetHit = true
				TargetGraphic = true
			}

			if TargetHit {
				if Proportional {
					Ratio : f32
					ProportionalDelta : linalg.Vector2f32
					Source : linalg.Vector2f32 
					switch UISelectInfo.Type {
						case .Button: Source = {ButtonList[UISelectInfo.Index].HitRect.width, ButtonList[UISelectInfo.Index].HitRect.height}
						case .TextArea: Source = {TextAreaList[UISelectInfo.Index].HitRect.width, TextAreaList[UISelectInfo.Index].HitRect.height}
					}
					if (Source.x >= Source.y) {
						Ratio = Source.x / Source.y
						ProportionalDelta.x = ResizeDelta.x * Ratio
						ProportionalDelta.y = ResizeDelta.x
					}
					else {
						Ratio = Source.y / Source.x
						ProportionalDelta.x = ResizeDelta.x
						ProportionalDelta.y = ResizeDelta.x * Ratio
					}

					switch UISelectInfo.Type {
						case .Button:
							ButtonList[UISelectInfo.Index].HitRect.x += ProportionalDelta.x/2
							ButtonList[UISelectInfo.Index].HitRect.y += ProportionalDelta.y/2
							ButtonList[UISelectInfo.Index].HitRect.width += ProportionalDelta.x
							ButtonList[UISelectInfo.Index].HitRect.height += ProportionalDelta.y
						case .TextArea:
							TextAreaList[UISelectInfo.Index].HitRect.x += ProportionalDelta.x/2
							TextAreaList[UISelectInfo.Index].HitRect.y += ProportionalDelta.y/2
							TextAreaList[UISelectInfo.Index].HitRect.width += ProportionalDelta.x
							TextAreaList[UISelectInfo.Index].HitRect.height += ProportionalDelta.y
					}
				}
				else {
					switch UISelectInfo.Type {
						case .Button:
							ButtonList[UISelectInfo.Index].HitRect.x += ResizeDelta.x/2
							ButtonList[UISelectInfo.Index].HitRect.y += ResizeDelta.y/2
							ButtonList[UISelectInfo.Index].HitRect.width += ResizeDelta.x
							ButtonList[UISelectInfo.Index].HitRect.height += ResizeDelta.y
						case .TextArea:
							TextAreaList[UISelectInfo.Index].HitRect.x += ResizeDelta.x/2
							TextAreaList[UISelectInfo.Index].HitRect.y += ResizeDelta.y/2
							TextAreaList[UISelectInfo.Index].HitRect.width += ResizeDelta.x
							TextAreaList[UISelectInfo.Index].HitRect.height += ResizeDelta.y
					}
				}
			}
			if TargetGraphic {
				if Proportional {
					Ratio : f32
					ProportionalDelta : linalg.Vector2f32
					Source : linalg.Vector2f32 
					switch UISelectInfo.Type {
						case .Button: Source = {ButtonList[UISelectInfo.Index].HitRect.width, ButtonList[UISelectInfo.Index].HitRect.height}
						case .TextArea: Source = {TextAreaList[UISelectInfo.Index].HitRect.width, TextAreaList[UISelectInfo.Index].HitRect.height}
					}
					if (Source.x >= Source.y) {
						Ratio = Source.x / Source.y
						ProportionalDelta.x = ResizeDelta.x * Ratio
						ProportionalDelta.y = ResizeDelta.x
					}
					else {
						Ratio = Source.y / Source.x
						ProportionalDelta.x = ResizeDelta.x
						ProportionalDelta.y = ResizeDelta.x * Ratio
					}

					switch UISelectInfo.Type {
						case .Button:
							ButtonList[UISelectInfo.Index].GraphicRect.x += ProportionalDelta.x/2
							ButtonList[UISelectInfo.Index].GraphicRect.y += ProportionalDelta.y/2
							ButtonList[UISelectInfo.Index].GraphicRect.width += ProportionalDelta.x
							ButtonList[UISelectInfo.Index].GraphicRect.height += ProportionalDelta.y
						case .TextArea: 
							TextAreaList[UISelectInfo.Index].GraphicRect.x += ProportionalDelta.x/2
							TextAreaList[UISelectInfo.Index].GraphicRect.y += ProportionalDelta.y/2
							TextAreaList[UISelectInfo.Index].GraphicRect.width += ProportionalDelta.x
							TextAreaList[UISelectInfo.Index].GraphicRect.height += ProportionalDelta.y
					}
				}
				else {
					switch UISelectInfo.Type {
						case .Button:
							ButtonList[UISelectInfo.Index].GraphicRect.x += ResizeDelta.x/2
							ButtonList[UISelectInfo.Index].GraphicRect.y += ResizeDelta.y/2
							ButtonList[UISelectInfo.Index].GraphicRect.width += ResizeDelta.x
							ButtonList[UISelectInfo.Index].GraphicRect.height += ResizeDelta.y
						case .TextArea:
							TextAreaList[UISelectInfo.Index].GraphicRect.x += ResizeDelta.x/2
							TextAreaList[UISelectInfo.Index].GraphicRect.y += ResizeDelta.y/2
							TextAreaList[UISelectInfo.Index].GraphicRect.width += ResizeDelta.x
							TextAreaList[UISelectInfo.Index].GraphicRect.height += ResizeDelta.y
					}
				}
			}

			switch UISelectInfo.Type {
				case .Button:
					if ButtonList[UISelectInfo.Index].HitRect.width <= 0 do ButtonList[UISelectInfo.Index].HitRect.width = 1
					if ButtonList[UISelectInfo.Index].HitRect.height <= 0 do ButtonList[UISelectInfo.Index].HitRect.height = 1
					if ButtonList[UISelectInfo.Index].GraphicRect.width <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.width = 1
					if ButtonList[UISelectInfo.Index].GraphicRect.height <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.height = 1
				case .TextArea:
					if TextAreaList[UISelectInfo.Index].HitRect.width <= 0 do TextAreaList[UISelectInfo.Index].HitRect.width = 1
					if TextAreaList[UISelectInfo.Index].HitRect.height <= 0 do TextAreaList[UISelectInfo.Index].HitRect.height = 1
					if TextAreaList[UISelectInfo.Index].GraphicRect.width <= 0 do TextAreaList[UISelectInfo.Index].GraphicRect.width = 1
					if TextAreaList[UISelectInfo.Index].GraphicRect.height <= 0 do TextAreaList[UISelectInfo.Index].GraphicRect.height = 1
			}
		}

		// @ApplicationRef Right Click Hold + Right Click Mode "Rotate" => Rotate Element (More Follows...)
		// @ApplicationRef  + shift => Proportional resize
		// @ApplicationRef  + alt => Target just Hit Rect
		// @ApplicationRef  + ctrl => Target just Graphic
		if IsDragging_Rotate {
			if ray.IsMouseButtonUp(.RIGHT) do IsDragging_Rotate = false
			switch UISelectInfo.Type {
				case .Button:
					Button := &ButtonList[UISelectInfo.Index]
					RotateDelta : f32 = ray.GetMouseDelta().x
					if ray.IsKeyDown(.LEFT_CONTROL) {
						Button.GraphicRotation += RotateDelta
					}
					else if ray.IsKeyDown(.LEFT_ALT) {
						Button.HitRotation += RotateDelta
					}
					else {
						Button.GraphicRotation += RotateDelta
						Button.HitRotation += RotateDelta
					}
				case .TextArea:
					// NOTE(Roskuski): No Rotation for text areas
			}
		}

		// @ApplicationRef Ctrl + A => resize Graphic under mouse to screen
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.A) && ray.IsKeyDown(.LEFT_CONTROL) {
			switch UISelectInfo.Type {
				case .Button:
					ButtonList[UISelectInfo.Index].GraphicRect.x = BluesBash_WindowWidth/2
					ButtonList[UISelectInfo.Index].GraphicRect.y = BluesBash_WindowHeight/2
					ButtonList[UISelectInfo.Index].GraphicRect.width = BluesBash_WindowWidth
					ButtonList[UISelectInfo.Index].GraphicRect.height = BluesBash_WindowHeight
				case .TextArea:
					TextAreaList[UISelectInfo.Index].GraphicRect.x = BluesBash_WindowWidth/2
					TextAreaList[UISelectInfo.Index].GraphicRect.y = BluesBash_WindowHeight/2
					TextAreaList[UISelectInfo.Index].GraphicRect.width = BluesBash_WindowWidth
					TextAreaList[UISelectInfo.Index].GraphicRect.height = BluesBash_WindowHeight
			}
		}

		// @ApplicationRef Ctrl + E => resize Graphic to size of souce graphic
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.E) && ray.IsKeyDown(.LEFT_CONTROL) {
			switch UISelectInfo.Type {
				case .Button:
					Animation, Ok := AnimationMap[strings.clone_from_bytes(ButtonList[UISelectInfo.Index].GraphicKey[:], context.temp_allocator)]
					if Ok {
						ButtonList[UISelectInfo.Index].GraphicRect.width = f32(Animation.Frame.width)
						ButtonList[UISelectInfo.Index].GraphicRect.height = f32(Animation.Frame.height)
					}
				case .TextArea:
					Animation, Ok := AnimationMap[strings.clone_from_bytes(TextAreaList[UISelectInfo.Index].GraphicKey[:], context.temp_allocator)]
					if Ok {
						TextAreaList[UISelectInfo.Index].GraphicRect.width = f32(Animation.Frame.width)
						TextAreaList[UISelectInfo.Index].GraphicRect.height = f32(Animation.Frame.height)
					}
			}
		}

		// @ApplicationRef Del => removes selected element
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.DELETE) {
			switch UISelectInfo.Type {
				case .Button:
					delete(ButtonList[UISelectInfo.Index].Key)
					delete(ButtonList[UISelectInfo.Index].GraphicKey)
					ordered_remove(&ButtonList, UISelectInfo.Index)
					UISelectInfo.Index = -1
				case .TextArea:
					delete(TextAreaList[UISelectInfo.Index].Key)
					delete(TextAreaList[UISelectInfo.Index].GraphicKey)
					ordered_remove(&TextAreaList, UISelectInfo.Index)
					UISelectInfo.Index = -1
			}
		}

		// @ApplicationRef ArrowKeys => Nudge element (More follows...)
		// + Space => Nudge by a factor of 10
		// + Alt => target hit
		// + crtl => target graphic with the current right click mode
		// + shift => target width/height
		if UISelectInfo.Index != -1 {
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

			TargetHit := ray.IsKeyDown(.LEFT_ALT)
			TargetGraphic := ray.IsKeyDown(.LEFT_CONTROL)
			ModSize := ray.IsKeyDown(.LEFT_SHIFT)
			if !TargetHit && !TargetGraphic {
				TargetHit = true
				TargetGraphic = true
			}

			switch UISelectInfo.Type {
				case .Button:
					Button := &ButtonList[UISelectInfo.Index]
					if TargetHit {
						if RightClickMode == .Rotate {
							Button.HitRotation += MoveDelta.x
						}
						else {
							if ModSize {
								Button.HitRect.width += MoveDelta.x
								Button.HitRect.height += MoveDelta.y
							}
							else {
								Button.HitRect.x += MoveDelta.x
								Button.HitRect.y += MoveDelta.y
							}
						}
					}

					if TargetGraphic {
						if RightClickMode == .Rotate {
							Button.GraphicRotation += MoveDelta.x
						}
						else if RightClickMode == .Resize {
							if ModSize {
								Button.GraphicRect.width += MoveDelta.x
								Button.GraphicRect.height += MoveDelta.y
							}
							else {
								Button.GraphicRect.x += MoveDelta.x
								Button.GraphicRect.y += MoveDelta.y
							}
						}
					}
				case .TextArea:
					TextArea := &TextAreaList[UISelectInfo.Index]
					if TargetHit {
						if ModSize {
							TextArea.HitRect.width += MoveDelta.x
							TextArea.HitRect.height += MoveDelta.y
						}
						else {
							TextArea.HitRect.x += MoveDelta.x
							TextArea.HitRect.y += MoveDelta.y
						}
					}

					if TargetGraphic {
						if RightClickMode == .Resize {
							if ModSize {
								TextArea.GraphicRect.width += MoveDelta.x
								TextArea.GraphicRect.height += MoveDelta.y
							}
							else {
								TextArea.GraphicRect.x += MoveDelta.x
								TextArea.GraphicRect.y += MoveDelta.y
							}
						}
					}
			}
		}

		// @ApplicationRef Ctrl + O => Open Existing File
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
			if (os.exists(strings.clone_from_cstring(Param.file))) do OpenFromFile(Param.file)
			else do fmt.printf("Open File \"%s\" does not exist!\n", Param.file)
			UISelectInfo.Index = -1
		}

		// @ApplicationRef Ctrl + S => Save Design as File
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
					ray.DrawTexturePro(Animation.Frame, {0, 0, f32(Animation.Frame.width), f32(Animation.Frame.height)}, ButtonDef.GraphicRect, {ButtonDef.GraphicRect.width/2, ButtonDef.GraphicRect.height/2}, ButtonDef.GraphicRotation, ray.WHITE)
				}
				else { // Draw a purple placeholder box
					ray.DrawRectanglePro(ButtonDef.GraphicRect, {ButtonDef.GraphicRect.width/2, ButtonDef.GraphicRect.height/2}, ButtonDef.GraphicRotation, ray.PURPLE)
				}
				Color := ray.RED
				Color.a = 0xff/2
				ray.DrawRectanglePro(ButtonDef.HitRect, {ButtonDef.HitRect.width/2, ButtonDef.HitRect.height/2}, ButtonDef.HitRotation, Color)
			}

			for TextAreaDef in TextAreaList {
				Animation, Ok := AnimationMap[strings.clone_from_bytes(TextAreaDef.GraphicKey[:], context.temp_allocator)]
				if Ok {
					ray.DrawTexturePro(Animation.Frame, {0, 0, f32(Animation.Frame.width), f32(Animation.Frame.height)}, TextAreaDef.GraphicRect, {TextAreaDef.GraphicRect.width/2, TextAreaDef.GraphicRect.height/2}, 0, ray.WHITE)
				}
				else {
					ray.DrawRectanglePro(TextAreaDef.GraphicRect, {TextAreaDef.GraphicRect.width/2, TextAreaDef.GraphicRect.height/2}, 0, ray.PURPLE)
				}


				{
					CenteredRect := ray.Rectangle{TextAreaDef.HitRect.x - TextAreaDef.HitRect.width/2, TextAreaDef.HitRect.y - f32(TextAreaDef.FontSize/2), TextAreaDef.HitRect.width, TextAreaDef.HitRect.height} 
					CString := strings.clone_to_cstring(strings.clone_from_bytes(TextAreaDef.Buffer[:], context.temp_allocator), context.temp_allocator)
					TextLength := ray.MeasureText(CString, TextAreaDef.FontSize)
					XOffset : f32 = 0
					for f32(TextLength) - XOffset > TextAreaDef.HitRect.width do XOffset += TextAreaDef.HitRect.width/4
					ray.BeginScissorMode(i32(CenteredRect.x), i32(CenteredRect.y), i32(CenteredRect.width), i32(CenteredRect.height))
					ray.DrawText(CString, i32(CenteredRect.x - XOffset), i32(CenteredRect.y), TextAreaDef.FontSize, ray.BLACK)
					ray.EndScissorMode()
				}

				Color := ray.RED
				Color.a = 0xff/2
				ray.DrawRectanglePro(TextAreaDef.HitRect, {TextAreaDef.HitRect.width/2, TextAreaDef.HitRect.height/2}, 0, Color)

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
				ClickHitInfo := false
				switch UISelectInfo.Type {
					case .Button:
						Button := &ButtonList[UISelectInfo.Index]
						DoInfoTextArea({5, 5, 300, 20}, "Button Name:", MousePos)
						if DoInfoTextArea({5, 25, 300, 20}, Button.Key[:], MousePos, &ClickHitInfo) {
							if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.Key)
						}

						DoInfoTextArea({5, 45, 300, 20}, "Graphic Name:", MousePos)
						if DoInfoTextArea({5, 65, 300, 20}, Button.GraphicKey[:], MousePos, &ClickHitInfo) {
							if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.GraphicKey)
						}
						DoInfoTextArea({5, 85, 200, 20}, fmt.tprintf("Draw Number: %d", UISelectInfo.Index), MousePos)
						if DoInfoButton({5, 105, 30, 20}, "--", MousePos, &ClickHitInfo) { // Move element closer to 0 in the button list.
							if UISelectInfo.Index != 0 {
								Swap(ButtonList, UISelectInfo.Index, UISelectInfo.Index - 1)
								UISelectInfo.Index -= 1
							}
						}
						if DoInfoButton({40, 105, 30, 20}, "++", MousePos, &ClickHitInfo) { // Move element closer to 0 in the button list.
							if UISelectInfo.Index != (len(ButtonList) - 1) {
								Swap(ButtonList, UISelectInfo.Index, UISelectInfo.Index + 1)
								UISelectInfo.Index += 1
							}
						}

						if DoInfoButton({5, 125, 300, 20}, fmt.tprintf("Right Click Mode: %s", RightClickMode), MousePos, &ClickHitInfo) {
							RightClickMode = right_click_mode(i32(RightClickMode) + 1)
							if i32(RightClickMode) == len(right_click_mode) do RightClickMode = .Resize
						}

						// @TODO(Roskuski): Allow typing into these number values
						DoInfoTextArea({310, 5, 300, 20}, "Hit Rect Params:", MousePos)
						DoInfoTextArea({310, 25, 300, 20}, fmt.tprintf("x: %f", Button.HitRect.x), MousePos)
						DoInfoTextArea({310, 45, 300, 20}, fmt.tprintf("y: %f", Button.HitRect.y), MousePos)
						DoInfoTextArea({310, 65, 300, 20}, fmt.tprintf("width: %f", Button.HitRect.width), MousePos)
						DoInfoTextArea({310, 85, 300, 20}, fmt.tprintf("height: %f", Button.HitRect.height), MousePos)
						DoInfoTextArea({310, 105, 300, 20}, fmt.tprintf("rotation: %f deg", Button.HitRotation), MousePos)

						DoInfoTextArea({615, 5, 300, 20}, "Graphic Rect Params:", MousePos)
						DoInfoTextArea({615, 25, 300, 20}, fmt.tprintf("x: %f", Button.GraphicRect.x), MousePos)
						DoInfoTextArea({615, 45, 300, 20}, fmt.tprintf("y: %f", Button.GraphicRect.y), MousePos)
						DoInfoTextArea({615, 65, 300, 20}, fmt.tprintf("width: %f", Button.GraphicRect.width), MousePos)
						DoInfoTextArea({615, 85, 300, 20}, fmt.tprintf("height: %f", Button.GraphicRect.height), MousePos)
						DoInfoTextArea({615, 105, 300, 20}, fmt.tprintf("rotation: %f deg", Button.GraphicRotation), MousePos)
					case .TextArea:
						TextArea := &TextAreaList[UISelectInfo.Index]
						DoInfoTextArea({5, 5, 300, 20}, "Text Area Name:", MousePos)
						if DoInfoTextArea({5, 25, 300, 20}, TextArea.Key[:], MousePos, &ClickHitInfo) {
							if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&TextArea.Key)
						}

						DoInfoTextArea({5, 45, 300, 20}, "Graphic Name:", MousePos)
						if DoInfoTextArea({5, 65, 300, 20}, TextArea.GraphicKey[:], MousePos, &ClickHitInfo) {
							if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&TextArea.GraphicKey)
						}

						DoInfoTextArea({5, 85, 300, 20}, "Sample Text:", MousePos)
						if DoInfoTextArea({5, 105, 300, 20}, TextArea.Buffer[:], MousePos, &ClickHitInfo) {
							if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&TextArea.Buffer, TextArea.BufferMaxSize)
						}

						if DoInfoTextArea({5, 125, 300, 20}, fmt.tprintf("Max Charcters: %d", TextArea.BufferMaxSize), MousePos, &ClickHitInfo) {
							KeyboardNumberInput(&TextArea.BufferMaxSize)
						}

						if DoInfoTextArea({5, 145, 300, 20}, fmt.tprintf("Font Size: %d", TextArea.FontSize), MousePos, &ClickHitInfo) {
							KeyboardNumberInput(&TextArea.FontSize)
						}

						// @TODO(Roskuski): Allow typing into these number values
						DoInfoTextArea({310, 5, 300, 20}, "Hit Rect Params:", MousePos)
						DoInfoTextArea({310, 25, 300, 20}, fmt.tprintf("x: %f", TextArea.HitRect.x), MousePos)
						DoInfoTextArea({310, 45, 300, 20}, fmt.tprintf("y: %f", TextArea.HitRect.y), MousePos)
						DoInfoTextArea({310, 65, 300, 20}, fmt.tprintf("width: %f", TextArea.HitRect.width), MousePos)
						DoInfoTextArea({310, 85, 300, 20}, fmt.tprintf("height: %f", TextArea.HitRect.height), MousePos)

						DoInfoTextArea({615, 5, 300, 20}, "Graphic Rect Params:", MousePos)
						DoInfoTextArea({615, 25, 300, 20}, fmt.tprintf("x: %f", TextArea.GraphicRect.x), MousePos)
						DoInfoTextArea({615, 45, 300, 20}, fmt.tprintf("y: %f", TextArea.GraphicRect.y), MousePos)
						DoInfoTextArea({615, 65, 300, 20}, fmt.tprintf("width: %f", TextArea.GraphicRect.width), MousePos)
						DoInfoTextArea({615, 85, 300, 20}, fmt.tprintf("height: %f", TextArea.GraphicRect.height), MousePos)
				}
				if ray.IsMouseButtonPressed(.LEFT) && ClickHitInfo == false {
					InfoSelectInfo.uiid = -1
				}
			}
		}
		ray.SetMouseOffset(0, 0)
		ray.EndMode2D()
		ray.EndScissorMode()

		ray.EndDrawing()
	}
}

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

button_def :: struct {
	Key : [dynamic]u8,
	HitRect : ray.Rectangle,
	HitRotation : f32,
	GraphicKey : [dynamic]u8,
	GraphicRect : ray.Rectangle,
	GraphicRotation : f32, 
}

right_click_mode :: enum {
	Resize,
	Rotate,
}

frame :: struct {
	FrameLength : i32,
	Graphic : ray.Texture2D,
}

animation :: struct {
	FrameTime : f64,
	UniqueFrameCount : i32,
	Frames : []frame,
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

AnimationMap : map[string]animation
ButtonList : [dynamic]button_def

StringHash :: proc(String : []u8) -> (Hash : i32) {
	Hash = 0
	for Byte in String {
		Hash += i32(Byte)
	}
	return Hash
}

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
	AnimationMap[strings.clone_from_bytes(Key)] = Animation
	fmt.printf("[LoadPppFile] Loading of \"%s\" was successful\n", PppPath)
}

/* File format
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
 - HitRotation : f32
 - GraphicRect : ray.Rectangle
 - GraphicRotation : f32
*/

// @TODO(Roskuski) validate that we do not have dublicate names
SaveToFile :: proc(Path : cstring, FileNameOffset, FileExtentionOffset : u16) {
	Path := string(Path)
	Handle, HandleError := os.open(Path, os.O_WRONLY | os.O_CREATE)
	if (HandleError == os.ERROR_NONE) {
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
			os.read_ptr(Handle, &Button.HitRotation, size_of(Button.HitRotation))
			os.read_ptr(Handle, &Button.GraphicRect, size_of(Button.GraphicRect))
			os.read_ptr(Handle, &Button.GraphicRotation, size_of(Button.GraphicRotation))

			append(&ButtonList, Button)
		}
	}
	else do fmt.printf("[OpenFromFile]: There was anm error opening \"%s\"(%d)\n", Path, HandleError)
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

		// @ApplicationRef Left Click Press => Select element for info pane. Selection is based off of Hit Rect
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

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// @ApplicationRef Left Click Hold => move element Graphic + Hit;
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

			if !DidHit {
				UISelectInfo.Index = -1
				InfoSelectInfo.uiid = -1
			}
		}

		// @ApplicationRef Right Click Hold + Right Click Mode "Resize" => Reize Element (More Follows...)
		// @ApplicationRef  + shift => Proportional resize
		// @ApplicationRef  + alt => Target just Hit Rect
		// @ApplicationRef  + ctrl => Target just Graphic Rect
		if IsDragging_Resize {
			if ray.IsMouseButtonUp(.RIGHT) do IsDragging_Resize = false
			if UISelectInfo.Type == .Button {
				Button := &ButtonList[UISelectInfo.Index]
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
						Source := linalg.Vector2f32{Button.HitRect.width, Button.HitRect.height}
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

						Button.HitRect.x += ProportionalDelta.x/2
						Button.HitRect.y += ProportionalDelta.y/2
						Button.HitRect.width += ProportionalDelta.x
						Button.HitRect.height += ProportionalDelta.y
					}
					else {
						Button.HitRect.x += ResizeDelta.x/2
						Button.HitRect.y += ResizeDelta.y/2
						Button.HitRect.width += ResizeDelta.x
						Button.HitRect.height += ResizeDelta.y
					}
				}
				if TargetGraphic {
					if Proportional {
						Ratio : f32
						ProportionalDelta : linalg.Vector2f32
						Source := linalg.Vector2f32{Button.GraphicRect.width, Button.GraphicRect.height}
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

						Button.GraphicRect.x += ProportionalDelta.x/2
						Button.GraphicRect.y += ProportionalDelta.y/2
						Button.GraphicRect.width += ProportionalDelta.x
						Button.GraphicRect.height += ProportionalDelta.y
					}
					else {
						Button.GraphicRect.x += ResizeDelta.x/2
						Button.GraphicRect.y += ResizeDelta.y/2
						Button.GraphicRect.width += ResizeDelta.x
						Button.GraphicRect.height += ResizeDelta.y
					}
				}

				if ButtonList[UISelectInfo.Index].HitRect.width <= 0 do ButtonList[UISelectInfo.Index].HitRect.width = 1
				if ButtonList[UISelectInfo.Index].HitRect.height <= 0 do ButtonList[UISelectInfo.Index].HitRect.height = 1
				if ButtonList[UISelectInfo.Index].GraphicRect.width <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.width = 1
				if ButtonList[UISelectInfo.Index].GraphicRect.height <= 0 do ButtonList[UISelectInfo.Index].GraphicRect.height = 1
			}
		}

		// @ApplicationRef Right Click Hold + Right Click Mode "Rotate" => Rotate Element (More Follows...)
		// @ApplicationRef  + shift => Proportional resize
		// @ApplicationRef  + alt => Target just Hit Rect
		// @ApplicationRef  + ctrl => Target just Graphic
		if IsDragging_Rotate {
			if ray.IsMouseButtonUp(.RIGHT) do IsDragging_Rotate = false
			if UISelectInfo.Type == .Button {
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
			}
		}

		// @ApplicationRef Ctrl + A => resize Graphic under mouse to screen
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.A) && ray.IsKeyDown(.LEFT_CONTROL) {
			ButtonList[UISelectInfo.Index].GraphicRect.x = BluesBash_WindowWidth/2
			ButtonList[UISelectInfo.Index].GraphicRect.y = BluesBash_WindowHeight/2
			ButtonList[UISelectInfo.Index].GraphicRect.width = BluesBash_WindowWidth
			ButtonList[UISelectInfo.Index].GraphicRect.height = BluesBash_WindowHeight
		}

		// @ApplicationRef Ctrl + E => resize Graphic to size of souce graphic
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.E) && ray.IsKeyDown(.LEFT_CONTROL) {
			Animation, Ok := AnimationMap[strings.clone_from_bytes(ButtonList[UISelectInfo.Index].GraphicKey[:], context.temp_allocator)]
			if Ok {
				ButtonList[UISelectInfo.Index].GraphicRect.width = f32(Animation.Frames[0].Graphic.width)
				ButtonList[UISelectInfo.Index].GraphicRect.height = f32(Animation.Frames[0].Graphic.height)
			}
		}

		// @ApplicationRef Del => removes selected element
		if (UISelectInfo.Index != -1) && ray.IsKeyPressed(.DELETE) {
			if UISelectInfo.Type == .Button {
				delete(ButtonList[UISelectInfo.Index].Key)
				delete(ButtonList[UISelectInfo.Index].GraphicKey)
				ordered_remove(&ButtonList, UISelectInfo.Index)
				UISelectInfo.Index = -1
			}
		}

		// @ApplicationRef ArrowKeys => Nudge element (More follows...)
		// + Space => Nudge by a factor of 10
		// + Alt => target hit
		// + crtl => target graphic with the current right click mode
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

				TargetHit := ray.IsKeyDown(.LEFT_ALT)
				TargetGraphic := ray.IsKeyDown(.LEFT_CONTROL)
				ModSize := ray.IsKeyDown(.LEFT_SHIFT)
				if !TargetHit && !TargetGraphic {
					TargetHit = true
					TargetGraphic = true
				}

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
					ray.DrawTexturePro(Animation.Frames[0].Graphic, {0, 0, f32(Animation.Frames[0].Graphic.width), f32(Animation.Frames[0].Graphic.height)}, ButtonDef.GraphicRect, {ButtonDef.GraphicRect.width/2, ButtonDef.GraphicRect.height/2}, ButtonDef.GraphicRotation, ray.WHITE)
				}
				else { // Draw a purple placeholder box
					ray.DrawRectanglePro(ButtonDef.GraphicRect, {ButtonDef.GraphicRect.width/2, ButtonDef.GraphicRect.height/2}, ButtonDef.GraphicRotation, ray.PURPLE)
				}
				Color := ray.RED
				Color.a = 0xff/2
				ray.DrawRectanglePro(ButtonDef.HitRect, {ButtonDef.HitRect.width/2, ButtonDef.HitRect.height/2}, ButtonDef.HitRotation, Color)
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
					DoInfoTextArea({5, 5, 300, 20}, "Button Name:", MousePos, &ClickHitInfo)
					if DoInfoTextArea({5, 25, 300, 20}, Button.Key[:], MousePos, &ClickHitInfo) {
						IBeamColor := ray.WHITE
						IBeamColor.a = (0xff * 3) / 4
						ray.DrawRectangleRec({5 + f32(ray.MeasureText(strings.clone_to_cstring(strings.clone_from_bytes(Button.Key[:], context.temp_allocator), context.temp_allocator), 20)) + 3, 25 + 1 , 2, 18}, IBeamColor)
						if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.Key)
					}

					DoInfoTextArea({5, 45, 300, 20}, "Graphic Name:", MousePos, &ClickHitInfo)
					if DoInfoTextArea({5, 65, 300, 20}, Button.GraphicKey[:], MousePos, &ClickHitInfo) {
						IBeamColor := ray.WHITE
						IBeamColor.a = (0xff * 3) / 4
						ray.DrawRectangleRec({5 + f32(ray.MeasureText(strings.clone_to_cstring(strings.clone_from_bytes(Button.GraphicKey[:], context.temp_allocator), context.temp_allocator), 20)) + 3, 65 + 1 , 2, 18}, IBeamColor)
						if ray.IsKeyUp(.LEFT_CONTROL) && ray.IsKeyUp(.LEFT_ALT) do KeyboardTypeInput(&Button.GraphicKey)
					}
					DoInfoTextArea({5, 85, 200, 20}, fmt.tprintf("Draw Number: %d", UISelectInfo.Index), MousePos, &ClickHitInfo)
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
					DoInfoTextArea({310, 5, 300, 20}, "Hit Rect Params:", MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 25, 300, 20}, fmt.tprintf("x: %f", Button.HitRect.x), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 45, 300, 20}, fmt.tprintf("y: %f", Button.HitRect.y), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 65, 300, 20}, fmt.tprintf("width: %f", Button.HitRect.width), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 85, 300, 20}, fmt.tprintf("height: %f", Button.HitRect.height), MousePos, &ClickHitInfo)
					DoInfoTextArea({310, 105, 300, 20}, fmt.tprintf("rotation: %f deg", Button.HitRotation), MousePos, &ClickHitInfo)

					DoInfoTextArea({615, 5, 300, 20}, "Graphic Rect Params:", MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 25, 300, 20}, fmt.tprintf("x: %f", Button.GraphicRect.x), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 45, 300, 20}, fmt.tprintf("y: %f", Button.GraphicRect.y), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 65, 300, 20}, fmt.tprintf("width: %f", Button.GraphicRect.width), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 85, 300, 20}, fmt.tprintf("height: %f", Button.GraphicRect.height), MousePos, &ClickHitInfo)
					DoInfoTextArea({615, 105, 300, 20}, fmt.tprintf("rotation: %f deg", Button.GraphicRotation), MousePos, &ClickHitInfo)

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

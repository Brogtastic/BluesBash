package PicturePreProcessor

import "core:io"
import "core:fmt"
import "core:os"
import "core:strconv"
import "core:path/filepath"
import "core:strings"

printf :: fmt.printf
println :: fmt.println

frame_file :: struct {
	Index : i32,
	Data : []u8,
}

CompareData :: proc(A, B : []u8) -> bool {
	Result := false
	if len(A) == len(B) {
		for Index in 0..<len(A) {
			if A[Index] != B[Index] {
				break
			}
		}
		Result = true
	}
	return Result
}

output_info :: struct {
	Index, Count : i32,
}

main :: proc() {
	if (len(os.args) != 6) {
		println(os.args[0], "<Format String For Files> <Frame Count> <Frame Rate Numeriator> <Frame Rate Denominator> <Output File Name>");
		println("Example:", os.args[0], "tree%d.png 100 1 60 Tree.ppp")
		println("  (the frame rate expressed there is 1/60th of a second")
		println("The first frame's image filename should start counting from 1.")
		return
	}
	FileFormat := os.args[1]
	FrameCount := i32(strconv.atoi(os.args[2]))
	FrameRateN := i32(strconv.atoi(os.args[3]))
	FrameRateD := i32(strconv.atoi(os.args[4]))
	OutputFileName := os.args[5]

	CurrentFrame, PrevousFrame : frame_file
	CurrentFrame.Index = -1
	PrevousFrame.Index = -1

	ConsecutiveFrames : i32 = 1

	OutputList := make([dynamic]output_info) 
	// @TODO fix memory leak

	for CurrentIndex : i32 = 1; CurrentIndex <= FrameCount; CurrentIndex += 1 { 
		{
			CurrentPath := fmt.tprintf(FileFormat, CurrentIndex)
			CurrentFrame.Index = CurrentIndex
			if CurrentFrame.Data != nil do delete(CurrentFrame.Data)
			Success : bool
			CurrentFrame.Data, Success = os.read_entire_file(CurrentPath)
			if !Success {
				CurrentFrame.Data, Success = os.read_entire_file(FileFormat)
				if !Success {
					printf("There was an loading both:\n\"%s\" and\n\"%s\"", CurrentPath, FileFormat);
				}
			}
		}

		if PrevousFrame.Index != -1 && PrevousFrame.Data != nil {
			if CompareData(CurrentFrame.Data, PrevousFrame.Data) {
				ConsecutiveFrames += 1
				if CurrentIndex == FrameCount { // emit if we are the frame always
					append(&OutputList, output_info{CurrentFrame.Index, ConsecutiveFrames})
				}
			}
			else { 
				append(&OutputList, output_info{PrevousFrame.Index, ConsecutiveFrames})
				PrevousFrame = CurrentFrame
				ConsecutiveFrames = 1
			}
		}
		else { // can't do work; PrevousFrame wasn't valid
			PrevousFrame = CurrentFrame
		}
	}
	OutputFilePath := fmt.tprintf("resources\\processed\\%s", OutputFileName);
	OutputFile, Error := os.open(OutputFilePath, os.O_WRONLY | os.O_CREATE)
	if Error != os.ERROR_NONE {
		println("[PPP] Failed to open OuptutFile:", OutputFilePath, ", Error: ", Error)
		return 
	}

	// File Format:
	//  Header:
	//   i32 length
	//   u8 AnimationName
	//   i32 FrameRateN
	//   i32 FrameRateD
	//   i32 UniqueFrameCount
	//  Data: (for UniqueFrameCount Times)
	//   i32 length of path
	//   u8 path data (as many as length indicates
	//   i32 How many frames should that frame be displayed for

	{
		_, File := filepath.split(OutputFilePath)
		FileName := strings.split(File, ".")
		defer delete(FileName)
		FileNameLen := i32(len(FileName[0]))
		os.write_ptr(OutputFile, &FileNameLen, size_of(FileNameLen))
		os.write_string(OutputFile, FileName[0]);
	}

	os.write_ptr(OutputFile, &FrameRateN, size_of(FrameRateN))
	os.write_ptr(OutputFile, &FrameRateD, size_of(FrameRateD))
	{
		TotalUniqueFrames := i32(len(OutputList))
		os.write_ptr(OutputFile, &TotalUniqueFrames, size_of(TotalUniqueFrames))
	}

	for _,Index in OutputList {
		Temp := fmt.tprintf(FileFormat, OutputList[Index].Index)
		TempLen := i32(len(Temp))
		os.write_ptr(OutputFile, &TempLen, size_of(TempLen))
		os.write_string(OutputFile, Temp)
		os.write_ptr(OutputFile, &OutputList[Index].Count, size_of(OutputList[Index].Count))
	}

	os.close(OutputFile)
	println("[PPP] Processed \"", OutputFilePath, "\" successfully")
}

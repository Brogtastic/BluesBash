/*******************************************************************************************
*
*   raylib [audio] example - Music playing (streaming)
*
*   This example has been created using raylib 1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(1280, 720, "title");

    InitAudioDevice();
    
    Sound middleC = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\C1.mp3");
    Sound middleE = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\Eb1.mp3");
    Sound middleG = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\G1.mp3");
    
    while(!WindowShouldClose()){
        
        if (IsKeyPressed(KEY_SPACE)){
            PlaySound(middleC);
            PlaySound(middleE);
            PlaySound(middleG);
        }
        
        if(IsKeyReleased(KEY_SPACE)){
            StopSound(middleC);
            StopSound(middleE);
            StopSound(middleG);
        }            
        
        BeginDrawing();

            ClearBackground(RAYWHITE);

        EndDrawing();
        
    }


   

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
    UnloadSound(middleC);
    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
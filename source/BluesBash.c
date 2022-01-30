#include "raylib.h"

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(1280, 720, "title");

    InitAudioDevice();
    
    Sound C1 = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\C1.mp3");
    Sound Eb1 = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\Eb1.mp3");
    Sound G1 = LoadSound("C:\\raylib\\npp\\bbAssets\\allNotes\\G1.mp3");
    
    int placement = 0;
    
    Sound keyboard[3] = {C1, Eb1, G1};
    
    while(!WindowShouldClose()){
        
        if (IsKeyPressed(KEY_F)){
            if((placement == 0) || (placement == 1)){
                placement = placement + 1;
            }
            
            PlaySound(keyboard[placement]);
        
        }
        
        
        if (IsKeyPressed(KEY_D)){
            if((placement == 1) || (placement == 2)){
                placement = placement - 1;
            }
            PlaySound(keyboard[placement]);
        }
        
        if(IsKeyReleased(KEY_F)){
            StopSound(keyboard[placement]);
        }            
        
        if(IsKeyReleased(KEY_D)){
            StopSound(keyboard[placement]);
        }
        
        if((IsKeyUp(KEY_F)) && (IsKeyUp(KEY_D))){
            StopSound(keyboard[0]);
            StopSound(keyboard[1]);
            StopSound(keyboard[2]);
        }
        
        BeginDrawing();

            ClearBackground(RAYWHITE);

        EndDrawing();
        
    }


   

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
    UnloadSound(C1);
    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
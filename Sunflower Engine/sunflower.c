#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <dsound.h>
#include <math.h>
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "triangle.h"
#include "array.h"
#include "matrix.h"
#include "light.h"
#include "texture.h"
#include "upng.h"
#include "camera.h"
#include "clipping.h"

#define internal static 
#define local_persist static 
#define global_variable static
#define Pi32 3.14159265359f

// Timer structs
#define FPS 60
#define MILLISECS_PER_FRAME 1000/FPS // ms/frame
LARGE_INTEGER frequency, time_a, time_b;

static bool running;
void Music(float deltaTime);

// SPRITE STRUCTS (Front and back)
static BITMAPINFO bitmap_info; // GDI struct defines pixel information, bitmap width, height [palettes]
static HBITMAP bitmap = 0; // defines framebuffer type, width, height, color format and bit values
static HDC bitmap_device_context = 0; // points to bitmap handle
bool LoadSprite(sprite_t *sprite, const char* filename);
void renderSprite(sprite_t sprite,int xoffset,int yoffset, float actualScale);

void VoxelEngine2(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap);
void VoxelEngine1(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap);
void VoxelEngine(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap);

// INPUT STRUCTS
bool keyboard[256] = {0}; // array with state of every key (with 0 - notpressed and 1 - pressed)
struct{
	int x, y;
	uint8_t buttons; // we'll or click events here
} mouse;
enum { MOUSE_LEFT = 0b1, MOUSE_MIDDLE = 0b10, MOUSE_RIGHT = 0b100, MOUSE_X1 = 0b1000, MOUSE_X2 = 0b10000 }; // 1byte

void Update(camera_t *camera, float deltaTime);
void Update_3d(float deltaTime);

// 3D RENDERER
mat4_t world_matrix;
mat4_t proj_matrix;
mat4_t view_matrix;
void process_graphics_pipeline_stages(mesh_t* mesh, float deltaTime, ArrayT* arrayT);

LRESULT CALLBACK WindowProcessMessage(HWND, UINT, WPARAM, LPARAM); // prototype (events function)

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
	static WNDCLASS window_class = { 0 }; // We create a windows class (struct we'll populate)
    static const wchar_t* window_class_name = L"My Window Class"; // We creat name 
    window_class.lpszClassName = (PCSTR)window_class_name; // define windows name
    window_class.lpfnWndProc = WindowProcessMessage; // pointer to function windows will call to handle events (messages)
    window_class.hInstance = hInstance; // set his id to current id
    RegisterClass(&window_class); // We register the class with windows
    
    // We inform GBI with info that'll never change about our pixels
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader); // set size
	bitmap_info.bmiHeader.biPlanes = 1; // number of color header always 1
	bitmap_info.bmiHeader.biBitCount = 32; // 32 bits per pixel FRGB
	bitmap_info.bmiHeader.biCompression = BI_RGB; // set to uncompressed RGB
	bitmap_device_context = CreateCompatibleDC(0); // We create device context handler
	
    static HWND window_handle;
    
#if 0
    window_handle = CreateWindow( // Creating actual windows (we return an id)
                                 (PCSTR)window_class_name,  // pass class name (L"My Window Class") (id?)
                                 "Emi's game",  // class nick; CS_OVERLAPPEDWINDOW: CW_USEDEFAULT
                                 WS_OVERLAPPEDWINDOW,  // style flags (WS_POPUP is borderless window and you need to define x,y,w,h)
                                 CW_USEDEFAULT,// x
                                 CW_USEDEFAULT,// y
                                 CW_USEDEFAULT,  // w
                                 CW_USEDEFAULT,// h
                                 NULL, // parent (only relevant to pop-up boxes)
                                 NULL,  // menu (windows derives from window class) (menus bad for games doe)
                                 hInstance, // handle
                                 NULL // lpParam
                                 );
#else
    int screenWidth = GetSystemMetrics(SM_CXSCREEN); // Get users screen width
    int screenHeight = GetSystemMetrics(SM_CYSCREEN); // and height
    window_handle = CreateWindow( (PCSTR)window_class_name, "Emi's game", WS_POPUP, 0, 0, screenWidth,screenHeight,
                                 NULL,NULL,hInstance,NULL);
#endif
    if(window_handle == NULL) { return -1; } // if error finish
    
    // Note(emi): before ever peek messages when we create a window, windows already processes several messages
	
    render_method = RENDER_TEXTURED;
    cull_method = CULL_BACKFACE;
    
    // Initialize the scene light direction
    init_light(vec3_new(0, 0, 1));
    
    static sprite_t colormap;
    LoadSpritePNG(&colormap, "./assets/EndColor.png");
    static sprite_t heightmap;
    LoadSpritePNG(&heightmap, "./assets/EndHeight.png");
    static sprite_t sprite;
    LoadSpritePNG(&sprite, "./assets/EndColor.png");
    
    // Loads mesh entities
    load_mesh("./assets/f117.obj", "./assets/f117.png", vec3_new(.5, .5, .5), vec3_new(0, 0, 0), vec3_new(0, 0, 0));
    
    // Initializing proj matrix
    float aspectx = (float)frame.w / (float)frame.h;
    float aspecty = (float)frame.h / (float)frame.w;
    float fovy = 3.141592/3.0; // 60 degrees
    float fovx = atan(tan(fovy/2)*aspectx)*1.99;
    float znear = 2.0;
    float zfar = 60.0;
    proj_matrix = mat4_make_perspective(fovy, aspecty, znear, zfar);
    
    // Initialize frustrum planes with a point and a normal
    init_frustum_planes(fovx, fovy, znear, zfar);
    
    QueryPerformanceFrequency(&frequency); // Get program's frequency
    QueryPerformanceCounter(&time_a);
    timeBeginPeriod(1); // Setting minimum sleep resolution 1ms
    
    ShowWindow(window_handle, nCmdShow); // actually show window lol (btw nCmdShow lmao)
    running = true;
    
    while(running) { // game loop
        // INPUT
        static MSG message = {0}; // create message struct (handles all sort of messages ALL)
        while(PeekMessage( // asyncrously retrieves messages
                          &message, // populate message and remove from queue
                          NULL, // handle to specific window (with NULL handles all windows in thread)
                          0, 0, // range minimum and maximum messages (if zero process them all)
                          PM_REMOVE))  
		{
            TranslateMessage(&message); // translate virtual keys to char messages (could remove to work with virtual)
            DispatchMessage(&message); // send message to our windows class function pointer "lpfnWndProc" xd
		}
        
        // FIX_FRAMERATE 
        QueryPerformanceCounter(&time_b);
        // Note: if we comment next three statements we get uncapped 
        //int timeToWait = MILLISECS_PER_FRAME - (float)(time_b.QuadPart-time_a.QuadPart)/frequency.QuadPart * 1000;
        //if (timeToWait > 0) Sleep(timeToWait); // if we have time to wait... then wait duh
        //QueryPerformanceCounter(&time_b);
        float deltaTime = (float)(time_b.QuadPart - time_a.QuadPart) / frequency.QuadPart; // ideally PIXELS_PER_SEC
        QueryPerformanceCounter(&time_a);
        
        // UPDATE
		Music(deltaTime);
		Update(&camera, deltaTime); // voxel update
        
        ArrayT arrayT;
        initArrayT(&arrayT);
        for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++)
        {
            mesh_t* mesh = get_mesh(mesh_index);
            // Process the graphics pipeline stages for every mesh
            process_graphics_pipeline_stages(mesh, deltaTime, &arrayT);
        }
        
        // RENDER
        clear_color_buffer(0xFF100020);
        draw_grid();
        VoxelEngine1(frame,camera, colormap, heightmap);
        
        for (int i = 0; i < arrayT.count; i++) // Main Renderer
        {
            triangle_t triangle = arrayT.triangles[i]; // Retrieve triangle
            
            // draw filled triangle
            if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE){
                draw_filled_triangle(triangle.points[0].x, triangle.points[0].y,//rendering 1st edge
                                     triangle.points[0].z, triangle.points[0].w,
                                     triangle.points[1].x, triangle.points[1].y, // 2nd edge
                                     triangle.points[1].z, triangle.points[1].w,
                                     triangle.points[2].x, triangle.points[2].y, 
                                     triangle.points[2].z, triangle.points[2].w, triangle.color); // 3rd edge
            }
            
            // Draw textured triangle
            if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE)
            {
                draw_textured_triangle(triangle.points[0].x, triangle.points[0].y, // xy for 1st vertex
                                       triangle.points[0].z, triangle.points[0].w,
                                       triangle.texcoords[0].u, triangle.texcoords[0].v, //uv for 1st vertex
                                       triangle.points[1].x, triangle.points[1].y, //2nd vertex
                                       triangle.points[1].z, triangle.points[1].w,
                                       triangle.texcoords[1].u, triangle.texcoords[1].v,
                                       triangle.points[2].x, triangle.points[2].y, // 3rd vertex
                                       triangle.points[2].z, triangle.points[2].w,
                                       triangle.texcoords[2].u, triangle.texcoords[2].v, triangle.texture);
                
            }
            
            // draw triangle wireframe
            if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || 
                render_method == RENDER_FILL_TRIANGLE_WIRE || render_method == RENDER_TEXTURED_WIRE)
            {
                draw_triangle(triangle.points[0].x, triangle.points[0].y,//rendering 1st edge
                              triangle.points[1].x, triangle.points[1].y, // 2nd edge
                              triangle.points[2].x, triangle.points[2].y, 0xFFFFFFFF);
            }
            
            // draw triangle vertex
            if (render_method == RENDER_WIRE_VERTEX)
            {
                draw_rect(triangle.points[0].x-4, triangle.points[0].y-4, 8, 8, 0xFFFF0000); // rendering 1st vector  
                draw_rect(triangle.points[1].x-4, triangle.points[1].y-4, 8, 8, 0xFFFF0000); // 2nd vector
                draw_rect(triangle.points[2].x-4, triangle.points[2].y-4, 8, 8, 0xFFFF0000); // 3rd vector
            }
        }
        freeArrayT(&arrayT);
        //renderSprite(sprite,0 ,0 ,.5);
        
        clear_z_buffer();
        // INVALIDATE TO REDRAW
        InvalidateRect( // marks section of window as invalid and needed to redraw
                       window_handle, // which window
                       NULL, // pointer to RECT (if null entire window)
                       false // if true background is erased when BeginPaint called, false to reamain unchanged
                       ); 
        UpdateWindow(window_handle); // update window immidiately passes message (redraw whenever we want)
        
    }
    //free(z_buffer)
    //free(sprite.pixels);
    free(colormap.pixels);
    //free(frame.pixels);
    free_meshes();
    //freeArrayF(&mesh.arrayF);
    //freeArrayV(&mesh.arrayV);
    return 0;
}

// Defining WindowsProcessMessage (function we dispatch our message)
LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool has_focus = true;
    
    switch(message) 
    {
        case WM_QUIT: // if running or destroy break
        case WM_DESTROY: {
            running = false;
        } break;
        
        case WM_PAINT: // All GDI objects and memory are setup so now "GDI copy my pixels to the window" 
        { 
            static PAINTSTRUCT paint; // struct that contains information to paint window (including update area)
            static HDC device_context; // handle device context
            device_context = BeginPaint(window_handle, &paint); // fills paint and gives us device context to paint
            BitBlt( // IMPORTANT rectangle to rectangle copy (pss we could use StrechDIBits or SetDIBitsToDevice)
                   device_context,
                   paint.rcPaint.left, // x
                   paint.rcPaint.top, // y
                   paint.rcPaint.right - paint.rcPaint.left, // w (could be 0)
                   paint.rcPaint.bottom - paint.rcPaint.top, // h (later we'll specify to not render everything anyway)
                   bitmap_device_context, // SOURCE note: bitmap_device_context->bitmap->framebuffer.pixels
                   paint.rcPaint.left, // xcoord
                   paint.rcPaint.top, // ycoord
                   SRCCOPY // Flag to just copy
                   );
            EndPaint(window_handle, &paint); // documentation tell us to use this (could break something idk)
            
        } break;
        
        case WM_SIZE: // message send when we create a window and resize
        {	 
            bitmap_info.bmiHeader.biWidth  = LOWORD(lParam); // We get window and height trough...
            bitmap_info.bmiHeader.biHeight = HIWORD(lParam); // lParam passed with the message
            
            // bitmap setup
            if (bitmap) DeleteObject(bitmap); // if bitmap was already created we delete and replace
            bitmap = CreateDIBSection( // Function that creates a HBITMAP that applications can write to
                                      NULL, // handle to device context
                                      &bitmap_info,  //pointer to bitmap info to get [new] dimensions, colors, etc
                                      DIB_RGB_COLORS, // what kind of data 
                                      &frame.pixels, // pass pointer to pixel array pointer, to redirect to big enough memory chunk
                                      0, // Handle to filemapping object (can be NULL)
                                      0 // Offset from the beginning of the file mapping referenced before lol (can be NULL ofc)
                                      );
            SelectObject(bitmap_device_context, bitmap); // point device context to bitmap
            
            frame.w  = LOWORD(lParam);
            frame.h = HIWORD(lParam);
            if (z_buffer) free(z_buffer);
            z_buffer = (float*)malloc(sizeof(float)*frame.width*frame.height);
        } break;
        
        case WM_KILLFOCUS:  // Message sent immediatly before loses keyboard focus
        { // note: we also use focus because windows can ocassionaly sent event even being unfocused
            has_focus = false;
            memset(keyboard, 0, 256 * sizeof(keyboard[0])); // clean keyboard with 0s
            mouse.buttons = 0; // clean mouse buttons too
        } break;
        
        case WM_SETFOCUS: has_focus = true; break; // Message sent after gained keyboard focus
        
        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(NULL);
            }
        }break;
        
        // If some key is or was pressed we check current and previous state extracting 31st and 32nd lParam
        case WM_SYSKEYDOWN: // when not focused or ALT
        case WM_SYSKEYUP:
        case WM_KEYDOWN: // when focused
        case WM_KEYUP:
        {
            if (has_focus)
            {
                static bool key_is_down, key_was_down;
                key_is_down = ((lParam & (1 << 31)) == 0); // if 31 lParam bit == 0 key is down
                key_was_down = ((lParam & (1 << 30)) != 0); // if 30 lParam bit == 1 key was down
                if (key_is_down != key_was_down) // we do this to ignore key repeats (sended after seconds pressing key)
                {	// wParam has key index.. we set given index in keyboard array to 1
                    keyboard[(uint8_t)wParam] = key_is_down;
                    if (key_is_down)
                    {
                        switch(wParam) // get key pressed
                        {
                            case 'M':
                            case VK_ESCAPE: 
                            {
                                running = false; 
                            } break;
                            case VK_F4:
                            {
                                if (lParam & (1 << 29)) // if ALT
                                    running = false;
                            } break;
                            case VK_F2: // rendering options
                            {
                                static int render_option = 4;
                                render_option = (render_option + 1) % 5;
                                if (render_option == 0)
                                    render_method = RENDER_FILL_TRIANGLE;
                                else if (render_option == 1)
                                    render_method = RENDER_WIRE;
                                else if (render_option == 2)
                                    render_method = RENDER_WIRE_VERTEX;
                                else if (render_option == 3)
                                    render_method = RENDER_TEXTURED;
                                else if (render_option == 4)
                                    render_method = RENDER_TEXTURED_WIRE;
                            } break;
                            case VK_F3:
                            {
                                if (cull_method == CULL_BACKFACE)
                                    cull_method = CULL_NONE;
                                else
                                    cull_method = CULL_BACKFACE;
                            } break;
                        }
                    }
                }
                
            }
            
            
        } break;
        
        case WM_MOUSEMOVE: // this messsage gives us coordinates
        {
            mouse.x = LOWORD(lParam); // x is straightforward
            mouse.y = frame.h - 1 - HIWORD(lParam); // y starts from top hence subtract h-1 to invert
        } break;
        
        // mouse right, left and middle click events  (we'll or mouse.buttons on click and inverse'and' on release)
        case WM_LBUTTONDOWN: mouse.buttons |=  MOUSE_LEFT;   break; // Turn on mouse.button
        case WM_LBUTTONUP: 	 mouse.buttons &= ~MOUSE_LEFT;   break; // Turn off mouse.button
        case WM_MBUTTONDOWN: mouse.buttons |=  MOUSE_MIDDLE; break;
        case WM_MBUTTONUP:	 mouse.buttons &= ~MOUSE_MIDDLE; break;
        case WM_RBUTTONDOWN: mouse.buttons |=  MOUSE_RIGHT;  break;
        case WM_RBUTTONUP:   mouse.buttons &= ~MOUSE_RIGHT;  break;
        
        case WM_XBUTTONDOWN: // X buttons, additional mouse buttons note: never hear of em 
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) mouse.buttons |= MOUSE_X1;
        else mouse.buttons |= MOUSE_X2; break;
        case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)	mouse.buttons &= ~MOUSE_X1;
        else mouse.buttons &= ~MOUSE_X2; break;
        
        case WM_MOUSEWHEEL: 
        { // freakin funny: in MOUSEWHEEL event direction is stored in 32nd bit of wParam 
            //OutputDebugStringA("%s\n", wParam & (0b1<<31) ? "Down" : "Up");
        } break;
        
        default: { // otherwise send message to default windows procedure
            return DefWindowProc(window_handle, message, wParam, lParam);
        } break;
    }
    return 0;
}

void Update(camera_t *camera, float deltaTime)
{
    static int keyboard_x = 0, keyboard_y = 0; // lookup to keyboard and check vectors
    bool run = false;
    
    if (keyboard[VK_SHIFT]) 
        run = true;
    
    if (keyboard[VK_UP]) 
    {
        camera->x += cos(camera->angle) * deltaTime*(300<<(run?7:0)); // now instead of moving globally move respect to angle
        camera->y += sin(camera->angle) * deltaTime*(300<<(run?7:0));
    }
    if (keyboard[VK_DOWN]) 
    {
        camera->x -= cos(camera->angle) * deltaTime*(300<<(run?4:0));
        camera->y -= sin(camera->angle) * deltaTime*(300<<(run?4:0));
    }
    if (keyboard[VK_RIGHT]) 
    {
        camera->x += cos(camera->angle+3.141592/2)*deltaTime*(300<<(run?4:0)); // now instead of moving globally move respect to angle
        camera->y += sin(camera->angle+3.141592/2)*deltaTime*(300<<(run?4:0));
    }
    if (keyboard[VK_LEFT]) 
    {
        camera->x += cos(camera->angle-3.141592/2)*deltaTime*(300<<(run?4:0));
        camera->y += sin(camera->angle-3.141592/2)*deltaTime*(300<<(run?4:0));
    }
    if (keyboard['W']     ) camera->horizon -= deltaTime * 1000; // if down arrow  || 'S' pressed then y--
    if (keyboard['S']     ) camera->horizon += deltaTime * 1000; // if down arrow  || 'S' pressed then y--
    if (keyboard['D']     ) camera->angle += deltaTime * 1.5; // if down arrow  || 'S' pressed then y--
    if (keyboard['A']     ) camera->angle -= deltaTime * 1.5; // if down arrow  || 'S' pressed then y--
    if (keyboard['Z']     ) camera->height += deltaTime * 60; // if down arrow  || 'S' pressed then y--
    if (keyboard['X']     ) camera->height -= deltaTime * 60; // if down arrow  || 'S' pressed then y--
}

void Update_3d(float deltaTime)
{
    // 3d good ones
    if (keyboard['Z']     ) update_camera_position(vec3_new(get_camera_position().x, 
                                                            get_camera_position().y + deltaTime * 3.0,
                                                            get_camera_position().z)); 
    if (keyboard['X']     ) update_camera_position(vec3_new(get_camera_position().x, 
                                                            get_camera_position().y - deltaTime * 3.0,
                                                            get_camera_position().z));
    if (keyboard['W']     ) rotate_camera_pitch(deltaTime * -1.5); // if down arrow  || 'S' pressed then y--
    if (keyboard['S']     ) rotate_camera_pitch(deltaTime * 1.5);// if down arrow  || 'S' pressed then y--
    if (keyboard['D']     ) rotate_camera_yaw(deltaTime * 1.5); // if down arrow  || 'S' pressed then y--
    if (keyboard['A']     ) rotate_camera_yaw(deltaTime * -1.5); // if down arrow  || 'S' pressed then y--
    if (keyboard[VK_UP]   ) {
        update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * deltaTime));
        update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keyboard[VK_DOWN] ) {
        update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * deltaTime));
        update_camera_position(vec3_sub(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keyboard[VK_RIGHT]   ) {
        vec3_t rotated = vec3_from_vec4(mat4_mul_vec4(mat4_make_rotation_y(3.141592/2), vec4_from_vec3(get_camera_direction())));
        update_camera_forward_velocity(vec3_mul(rotated, 5.0 * deltaTime));
        update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
    }
    if (keyboard[VK_LEFT] ) {
        vec3_t rotated = vec3_from_vec4(mat4_mul_vec4(mat4_make_rotation_y(3.141592/2), vec4_from_vec3(get_camera_direction())));
        update_camera_forward_velocity(vec3_mul(rotated, 5.0 * deltaTime));
        update_camera_position(vec3_sub(get_camera_position(), get_camera_forward_velocity()));
    }
}

void VoxelEngine2(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap)
{
    // RENDER...
    /*int32_t skycolor = 0xFF00AFFF;
    for (int i = 0; i < (frame.h)*frame.w; i+=4)
    {
        frame.pixels[i] = skycolor;
        frame.pixels[i+1] = skycolor;
        frame.pixels[i+2] = skycolor;
        frame.pixels[i+3] = skycolor;
    }*/
    
    float sinangle = sin(camera.angle);
    float cosangle = cos(camera.angle);
    float plx = cosangle*camera.zfar + sinangle*camera.zfar; 
    float ply = sinangle*camera.zfar - cosangle*camera.zfar;
    float prx = cosangle*camera.zfar - sinangle*camera.zfar;
    float pry = sinangle*camera.zfar + cosangle*camera.zfar;
    
    // Voxel engine 2
    for (int i = 0; i < frame.w; i+=4) // for each ray
    {
        // get change in x and y with respect to ztep for every ray
        // note: prx-plf gets me distance, divided by each ray step * i(current step) + plx to return / change in zfar
        float delta_x =  (plx + (prx-plx)/frame.w*i) / camera.zfar;
        float delta_y =  (ply + (pry-ply)/frame.w*i) / camera.zfar;
        float delta_x1 = (plx + (prx-plx)/frame.w*(i+1)) / camera.zfar;
        float delta_y1 = (ply + (pry-ply)/frame.w*(i+1)) / camera.zfar;
        float delta_x2 = (plx + (prx-plx)/frame.w*(i+2)) / camera.zfar;
        float delta_y2 = (ply + (pry-ply)/frame.w*(i+2)) / camera.zfar;
        float delta_x3 = (plx + (prx-plx)/frame.w*(i+3)) / camera.zfar;
        float delta_y3 = (ply + (pry-ply)/frame.w*(i+3)) / camera.zfar;
        
        float rx = camera.x; // set (or reset) x and y coordinates
        float ry = camera.y;
        float rx1 = camera.x; // set (or reset) x and y coordinates
        float ry1 = camera.y;
        float rx2 = camera.x; // set (or reset) x and y coordinates
        float ry2 = camera.y;
        float rx3 = camera.x; // set (or reset) x and y coordinates
        float ry3 = camera.y;
        
        float max_height = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        float max_height1 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        float max_height2 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        float max_height3 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        
        float dz = 1;
        for (int z = 1; z < camera.zfar; z+=dz) // for each zstep
        {
            rx += delta_x*dz;
            ry += delta_y*dz;
            rx1 += delta_x1*dz;
            ry1 += delta_y1*dz;
            rx2 += delta_x2*dz;
            ry2 += delta_y2*dz;
            rx3 += delta_x3*dz;
            ry3 += delta_y3*dz;
            
            // Get height values from map
            // times heightmap.w cause we translating frame height to map height (& makes it modulus)
            int mapoffset = (heightmap.w*((int)(ry) & heightmap.h-1)) + ((int)(rx) & heightmap.w-1);
            int mapoffset1 = (heightmap.w*((int)(ry1) & heightmap.h-1)) + ((int)(rx1) & heightmap.w-1);
            int mapoffset2 = (heightmap.w*((int)(ry2) & heightmap.h-1)) + ((int)(rx2) & heightmap.w-1);
            int mapoffset3 = (heightmap.w*((int)(ry3) & heightmap.h-1)) + ((int)(rx3) & heightmap.w-1);
            
            int heightonscreen = (int)((heightmap.pixels[mapoffset]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            int heightonscreen1 = (int)((heightmap.pixels[mapoffset1]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            int heightonscreen2 = (int)((heightmap.pixels[mapoffset2]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            int heightonscreen3 = (int)((heightmap.pixels[mapoffset3]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            
            if (heightonscreen < 0) heightonscreen = 0; // to protect from writing outside memory
            if (heightonscreen1 < 0) heightonscreen1 = 0; // to protect from writing outside memory
            if (heightonscreen2 < 0) heightonscreen2 = 0; // to protect from writing outside memory
            if (heightonscreen3 < 0) heightonscreen3 = 0; // to protect from writing outside memory
            if (heightonscreen > frame.h) heightonscreen = frame.h;
            if (heightonscreen1 > frame.h) heightonscreen1 = frame.h;
            if (heightonscreen2 > frame.h) heightonscreen2 = frame.h;
            if (heightonscreen3 > frame.h) heightonscreen3 = frame.h;
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen >= max_height) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height; y < heightonscreen; y++)
                {
                    frame.pixels[y*frame.w + i] = colormap.pixels[mapoffset]; // i cause we talking this ray
                }
                max_height = heightonscreen;
            }
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen1 >= max_height1) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height1; y < heightonscreen1; y++)
                {
                    frame.pixels[y*frame.w + (i+1)] = colormap.pixels[mapoffset1]; // i cause we talking this ray
                }
                max_height1 = heightonscreen1;
            }
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen2 >= max_height2) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height2; y < heightonscreen2; y++)
                {
                    frame.pixels[y*frame.w + (i+2)] = colormap.pixels[mapoffset2]; // i cause we talking this ray
                }
                max_height2 = heightonscreen2;
            }
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen3 >= max_height3) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height3; y < heightonscreen3; y++)
                {
                    frame.pixels[y*frame.w + (i+3)] = colormap.pixels[mapoffset3]; // i cause we talking this ray
                }
                max_height3 = heightonscreen3;
            }
            if (1)
                dz *= 1.005;
            else
                dz += 0.02;
        }
    }
}


void VoxelEngine1(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap)
{
    // RENDER...
    /*int32_t skycolor = 0xFF00AFFF;
    for (int i = 0; i < (frame.h)*frame.w; i+=4)
    {
        frame.pixels[i] = skycolor;
        frame.pixels[i+1] = skycolor;
        frame.pixels[i+2] = skycolor;
        frame.pixels[i+3] = skycolor;
    }*/
    
    float sinangle = sin(camera.angle);
    float cosangle = cos(camera.angle);
    float plx = cosangle*camera.zfar + sinangle*camera.zfar; 
    float ply = sinangle*camera.zfar - cosangle*camera.zfar;
    float prx = cosangle*camera.zfar - sinangle*camera.zfar;
    float pry = sinangle*camera.zfar + cosangle*camera.zfar;
    
    // Voxel engine 1
    for (int i = 0; i < frame.w; i+=4) // for each ray
    {
        // get change in x and y with respect to ztep for every ray
        // note: prx-plf gets me distance, divided by each ray step * i(current step) + plx to return / change in zfar
        float delta_x =  (plx + (prx-plx)/frame.w*i) / camera.zfar;
        float delta_y =  (ply + (pry-ply)/frame.w*i) / camera.zfar;
        float delta_x2 = (plx + (prx-plx)/frame.w*(i+2)) / camera.zfar;
        float delta_y2 = (ply + (pry-ply)/frame.w*(i+2)) / camera.zfar;
        
        float rx = camera.x; // set (or reset) x and y coordinates
        float ry = camera.y;
        float rx2 = camera.x; // set (or reset) x and y coordinates
        float ry2 = camera.y;
        
        float max_height = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        float max_height2 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        
        float dz= 1.0;
        for (int z = 1; z < camera.zfar; z+=dz) // for each zstep
        {
            if (max_height >= frame.h
                && max_height2 >= frame.h) break;
            
            rx += delta_x*dz;
            ry += delta_y*dz;
            rx2 += delta_x2*dz;
            ry2 += delta_y2*dz;
            
            // Get height values from map
            // times heightmap.w cause we translating frame height to map height (& makes it modulus)
            int mapoffset = (heightmap.w*((int)(ry) & heightmap.h-1)) + ((int)(rx) & heightmap.w-1);
            int mapoffset2 = (heightmap.w*((int)(ry2) & heightmap.h-1)) + ((int)(rx2) & heightmap.w-1);
            
            int heightonscreen = (int)((heightmap.pixels[mapoffset]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            int heightonscreen2 = (int)((heightmap.pixels[mapoffset2]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            
            if (heightonscreen < 0) heightonscreen = 0; // to protect from writing outside memory
            if (heightonscreen2 < 0) heightonscreen2 = 0; // to protect from writing outside memory
            if (heightonscreen > frame.h) heightonscreen = frame.h;
            if (heightonscreen2 > frame.h) heightonscreen2 = frame.h;
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen >= max_height) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height; y < heightonscreen; y++)
                {
                    frame.pixels[y*frame.w + i] = colormap.pixels[mapoffset]; // i cause we talking this ray
                    frame.pixels[y*frame.w + (i+1)] = colormap.pixels[mapoffset]; // i cause we talking this ray
                }
                max_height = heightonscreen;
            }
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen2 >= max_height2) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height2; y < heightonscreen2; y++)
                {
                    frame.pixels[y*frame.w + (i+2)] = colormap.pixels[mapoffset2]; // i cause we talking this ray
                    frame.pixels[y*frame.w + (i+3)] = colormap.pixels[mapoffset2]; // i cause we talking this ray
                }
                max_height2 = heightonscreen2;
            }
            if (0)
                dz += 0.02;
            else
                dz *= 1.006;
        }
    }
}


void VoxelEngine(sprite_t frame,camera_t camera,sprite_t colormap,sprite_t heightmap)
{ 
    // TODO:: at some point z won't matter height will be high enough than you won't be able to pass it
    // probably there's a ratio i'll have to find
    // TODO: You can multithread rays find a way to do it
    
    // RENDER...
    /*int32_t skycolor = 0xFF00AFFF;
    for (int i = 0; i < (frame.h)*frame.w; i+=4)
    {
        frame.pixels[i] = skycolor;
        frame.pixels[i+1] = skycolor;
        frame.pixels[i+2] = skycolor;
        frame.pixels[i+3] = skycolor;
    }*/
    
    float sinangle = sin(camera.angle);
    float cosangle = cos(camera.angle);
    float plx = cosangle*camera.zfar + sinangle*camera.zfar; 
    float ply = sinangle*camera.zfar - cosangle*camera.zfar;
    float prx = cosangle*camera.zfar - sinangle*camera.zfar;
    float pry = sinangle*camera.zfar + cosangle*camera.zfar;
    
    // Voxel engine 0
    for (int i = 0; i < frame.w; i+=4) // for each ray
    {
        // get change in x and y with respect to ztep for every ray
        // note: prx-plf gets me distance, divided by each ray step * i(current step) + plx to return / change in zfar
        float delta_x =  (plx + (prx-plx)/frame.w*i) / camera.zfar;
        float delta_y =  (ply + (pry-ply)/frame.w*i) / camera.zfar;
        //float delta_x1 = (plx + (prx-plx)/frame.w*(i+1)) / camera.zfar;
        //float delta_y1 = (ply + (pry-ply)/frame.w*(i+1)) / camera.zfar;
        //float delta_x2 = (plx + (prx-plx)/frame.w*(i+2)) / camera.zfar;
        //float delta_y2 = (ply + (pry-ply)/frame.w*(i+2)) / camera.zfar;
        //float delta_x3 = (plx + (prx-plx)/frame.w*(i+3)) / camera.zfar;
        //float delta_y3 = (ply + (pry-ply)/frame.w*(i+3)) / camera.zfar;
        
        float rx = camera.x; // set (or reset) x and y coordinates
        float ry = camera.y;
        //float rx1 = camera.x; // set (or reset) x and y coordinates
        //float ry1 = camera.y;
        //float rx2 = camera.x; // set (or reset) x and y coordinates
        //float ry2 = camera.y;
        //float rx3 = camera.x; // set (or reset) x and y coordinates
        //float ry3 = camera.y;
        
        float max_height = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        //float max_height1 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        //float max_height2 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        //float max_height3 = 0.0; // tallest voxel yet so to just render like a stack (reset to 0 always)
        
        float dz = 1;
        for (int z = 1; z < camera.zfar; z+=dz) // for each zstep
        {
            // TODO: here break as soon as z i'ts to big to pass max_height
            //if(z > max_height*max_height) break;
            
            rx += delta_x*dz;
            ry += delta_y*dz;
            //rx1 += delta_x1;
            //ry1 += delta_y1;
            //rx2 += delta_x2;
            //ry2 += delta_y2;
            //rx3 += delta_x3;
            //ry3 += delta_y3;
            
            // Get height values from map
            // times heightmap.w cause we translating frame height to map height (& makes it modulus)
            int mapoffset = (heightmap.w*((int)(ry) & heightmap.h-1)) + ((int)(rx) & heightmap.w-1);
            //int mapoffset1 = (heightmap.w*((int)(ry1) & heightmap.h-1)) + ((int)(rx1) & heightmap.w-1);
            //int mapoffset2 = (heightmap.w*((int)(ry2) & heightmap.h-1)) + ((int)(rx2) & heightmap.w-1);
            //int mapoffset3 = (heightmap.w*((int)(ry3) & heightmap.h-1)) + ((int)(rx3) & heightmap.w-1);
            
            int heightonscreen = (int)((heightmap.pixels[mapoffset]&0x000000ff)-camera.height)*2048/z+camera.horizon;
            //int heightonscreen1 = (int)((heightmap.pixels[mapoffset1]&0x000000ff)-camera.height)*1024/z+camera.horizon;
            //int heightonscreen2 = (int)((heightmap.pixels[mapoffset2]&0x000000ff)-camera.height)*1024/z+camera.horizon;
            //int heightonscreen3 = (int)((heightmap.pixels[mapoffset3]&0x000000ff)-camera.height)*1024/z+camera.horizon;
            
            if (heightonscreen < 0) heightonscreen = 0; // to protect from writing outside memory
            //if (heightonscreen1 < 0) heightonscreen1 = 0; // to protect from writing outside memory
            //if (heightonscreen2 < 0) heightonscreen2 = 0; // to protect from writing outside memory
            //if (heightonscreen3 < 0) heightonscreen3 = 0; // to protect from writing outside memory
            if (heightonscreen > frame.h) heightonscreen = frame.h;
            //if (heightonscreen1 > frame.h) heightonscreen1 = frame.h;
            //if (heightonscreen2 > frame.h) heightonscreen2 = frame.h;
            //if (heightonscreen3 > frame.h) heightonscreen3 = frame.h;
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            // only render the terrain pixels if the new projected height is taller that the previous max_height
            if (heightonscreen >= max_height) // not entering
            {
                //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                // Draw pixels from previous max_height until the new projected height
                for (int y = max_height; y < heightonscreen; y++)
                {
                    frame.pixels[y*frame.w + i] = colormap.pixels[mapoffset]; // i cause we talking this ray
                    frame.pixels[y*frame.w + (i+1)] = colormap.pixels[mapoffset]; // i cause we talking this ray
                    frame.pixels[y*frame.w + (i+2)] = colormap.pixels[mapoffset]; // i cause we talking this ray
                    frame.pixels[y*frame.w + (i+3)] = colormap.pixels[mapoffset]; // i cause we talking this ray
                }
                max_height = heightonscreen;
            }
            /*
                        // only render the terrain pixels if the new projected height is taller that the previous max_height
                        if (heightonscreen1 >= max_height1) // not entering
                        {
                            //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                            // Draw pixels from previous max_height until the new projected height
                            for (int y = max_height1; y < heightonscreen1; y++)
                            {
                                frame.pixels[y*frame.w + (i+1)] = colormap.pixels[mapoffset1]; // i cause we talking this ray
                            }
                            max_height1 = heightonscreen1;
                        }
                        // only render the terrain pixels if the new projected height is taller that the previous max_height
                        if (heightonscreen2 >= max_height2) // not entering
                        {
                            //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                            // Draw pixels from previous max_height until the new projected height
                            for (int y = max_height2; y < heightonscreen2; y++)
                            {
                                frame.pixels[y*frame.w + (i+2)] = colormap.pixels[mapoffset2]; // i cause we talking this ray
                            }
                            max_height2 = heightonscreen2;
                        }
                        // only render the terrain pixels if the new projected height is taller that the previous max_height
                        if (heightonscreen3 >= max_height3) // not entering
                        {
                            //frame.pixels[(int)((ry)*frame.w/4)+(int)(rx/4)] = 0xFFFF0000;
                            // Draw pixels from previous max_height until the new projected height
                            for (int y = max_height3; y < heightonscreen3; y++)
                            {
                                frame.pixels[y*frame.w + (i+3)] = colormap.pixels[mapoffset3]; // i cause we talking this ray
                            }
                            max_height3 = heightonscreen3;
                        }*/
            if (1)
                dz *= 1.005;
            else
                dz += 0.02;
        }
    }
}


void Music(float deltaTime)
{
    static float music_counter = 0.0f;
    if (music_counter == 0.0f)
    {
        PlaySound(TEXT("./assets/CruelAngelsThesis.wav"), NULL, SND_ASYNC);
    }
    music_counter +=  deltaTime;
    const float song_duration = 60*4+05;
    const float song_padding = 0.0;
    if (music_counter > song_duration + song_padding)
        music_counter = 0.0f;
}

bool LoadSprite(sprite_t *sprite, const char* filename)
{
    uint32_t image_data_address; // pointer to 1st pixel (relative?)
    uint32_t width; // w
    uint32_t height; // h
    uint32_t pixel_count; // w*h 
    uint16_t bit_depth; // 32 bits probs
    uint8_t byte_depth; // pixel_count / bit_depth
    uint32_t *pixels; // actual first pixel pointer
    
    OutputDebugStringA("Loading bitmap file");
    FILE* file;
    fopen_s(&file, filename, "rb");
    
    if (!file) // open file error
    {
        OutputDebugStringA("couldn't open file");
        return false;
    }
    
    // Magic characters BM
    if ( !(fgetc(file)=='B' && fgetc(file)=='M')) // if error reading BM
    { //note fgetc offsets 1 byte from stream
        OutputDebugStringA("Bitmap 'B' 'M' error");
        return false;
    }
    
    // Eating and reading info from header
    fseek(file, 8, SEEK_CUR); // We eat next 8 bytes from current stream (file size, junk)
    fread(&image_data_address, 4, 1, file); // 4 bytes for data address
    fseek(file, 4, SEEK_CUR); // we eat 4 butes for header size
    fread(&width, 4, 1, file); //4 bytes for width
    fread(&height, 4, 1,  file); // 4 bytes for height
    fseek(file, 2, SEEK_CUR); // we eat 2 bytes for color planes
    fread(&bit_depth, 2, 1, file); // 2 bytes for bits per pixel
    pixel_count = width * height; // We calculate the pixel count
    byte_depth = bit_depth / 8; // We calcuclate byte depth
    
    if (bit_depth != 32) 
    {// note (emi): There are other formats for bitmap
        
        printf("Byte:depth: %d\n", byte_depth);
        printf("Bit depth is different than 32 %s", filename);
        
    }
    
    pixels = (uint32_t *)malloc(pixel_count * byte_depth); // Allocate memory needed and point pixels to it
    if (!pixels) // couldn't allocate memory
    {
        OutputDebugStringA("couldn't allocate enough memory");
        return false;
    }
    
    fseek(file, image_data_address, SEEK_SET); // start from 0 and goto image data address to read from there
    fread(pixels, byte_depth, pixel_count, file); // into pixels a total of byte_depth*pixel_count from file
    
    sprite->w = width; // set w
    sprite->h = height; // set h
    sprite->p = pixels; // set p pointer = pixels
    
    fclose(file);
    
    return true;
    
}

void renderSprite(sprite_t sprite, int xoffset, int yoffset, float scale)
{
    int step = 1 / scale;
    //offseting image rendering
    xoffset = (xoffset >= frame.w - sprite.w*scale) ? frame.w - sprite.w*scale: xoffset;
    yoffset = (yoffset >= frame.h - sprite.h*scale) ? frame.h - sprite.h*scale: yoffset;
    xoffset = (xoffset < 0) ? 0: xoffset;
    yoffset = (yoffset < 0) ? 0: yoffset;
    
    // image renderer
    for(int y = 0; y < sprite.h; y += step) { // for each pixel in sprite
        for(int x = 0; x < sprite.w; x += step) {
            static uint32_t source_index, target_index;
            static float alpha, anti_alpha;
            static uint32_t sr, sg, sb; // Source
            static uint32_t tr, tg, tb; // Target
            source_index = x + y*sprite.w; // Source coordinate
            target_index = (x*scale + xoffset) + (y*scale+yoffset)*frame.w; // Targe coordinate
            
            if (target_index >= frame.w * frame.h) break; // if pixel is outside memory
            
            // get alpha & anti-alpha
            alpha = (float)    (((sprite.p[source_index] & 0xff000000) >> 24) >>7); // value between 0-1
            anti_alpha = 1.f - alpha;
            
            // alpha sprite
            sr    = (uint32_t)(((sprite.p[source_index] & 0x00ff0000) >> 16) * alpha)      << 16;
            sg    = (uint32_t)(((sprite.p[source_index] & 0x0000ff00) >>  8) * alpha)      <<  8;
            sb    =             (sprite.p[source_index] & 0x000000ff       ) * alpha;
            
            // alpha framebuffer (complement of sprite that's why we use antialpha)
            tr    = (uint32_t)((( frame.p[target_index] & 0x00ff0000) >> 16) * anti_alpha) << 16;
            tg    = (uint32_t)((( frame.p[target_index] & 0x0000ff00) >>  8) * anti_alpha) <<  8;
            tb    =             ( frame.p[target_index] & 0x000000ff       ) * anti_alpha;
            
            frame.pixels[target_index] = sb + tb + sg + tg + sr + tr; // mix sprite with framebuffer
        }
    }
}


void process_graphics_pipeline_stages(mesh_t* mesh, float deltaTime, ArrayT* arrayT)
{
    ///////////////////////////////////////////////////////////////////////////////
    // Process the graphics pipeline stages for all the mesh triangles
    ///////////////////////////////////////////////////////////////////////////////
    // +-------------+
    // | Model space |  <-- original mesh vertices
    // +-------------+
    // |   +-------------+
    // `-> | World space |  <-- multiply by world matrix
    //     +-------------+
    //     |   +--------------+
    //     `-> | Camera space |  <-- multiply by view matrix
    //         +--------------+
    //         |    +------------+
    //         `--> |  Clipping  |  <-- clip against the six frustum planes
    //              +------------+
    //              |    +------------+
    //              `--> | Projection |  <-- multiply by projection matrix (accounting for screen)
    //                   +------------+
    //                   |    +-------------+
    //                   `--> | Image space |  <-- apply perspective divide (applying z perspective and flattening)
    //                        +-------------+
    //                        |    +--------------+
    //                        `--> | Screen space |  <-- ready to render
    //                             +--------------+
    ///////////////////////////////////////////////////////////////////////////////
    
    // Mesh transformations frame by frame
    mesh->rotation.x += 1.5 * deltaTime; 
    mesh->rotation.y += .5 * deltaTime; 
    mesh->rotation.z += 1.5 * deltaTime;
    mesh->scale.x += 0 * deltaTime;
    mesh->scale.y += .0 * deltaTime;
    mesh->scale.z += 0 * deltaTime;
    mesh->translation.x += 0.0 * deltaTime;
    mesh->translation.y += .0 * deltaTime;
    mesh->translation.z = 5;
    
    // creating world matrixes
    mat4_t scale_matrix = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
    mat4_t translation_matrix = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh->rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh->rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh->rotation.z);
    
    // creating the view matrix with camera. TODO(EMI): MERGE BOTH CAMERAS 
    vec3_t up_direction = {0, 1, 0};
    vec3_t target = {0, 0, 1};
    // note: comment these 2 to not account for camera movement
    //Update_3d(deltaTime);
    //target = get_camera_lookat_target();
    
    view_matrix = mat4_look_at(get_camera_position(), target, up_direction); // pushing univers around the camera
    
    
    for (int i = 0; i < mesh->arrayF.count; i++) // main world projector
    {
        face_t mesh_face = mesh->arrayF.faces[i]; // retrieve mesh faces from array
        
        vec3_t face_vertices[3]; // Retrieving vertices from the (cube mesh)
        face_vertices[0] = mesh->arrayV.vertices[mesh_face.a]; // -1 cause indexing starts at 0 (fixed it :D)
        face_vertices[1] = mesh->arrayV.vertices[mesh_face.b];
        face_vertices[2] = mesh->arrayV.vertices[mesh_face.c];
        
        vec4_t transformed_vertices[3];
        transformed_vertices[0] = vec4_from_vec3(face_vertices[0]);
        transformed_vertices[1] = vec4_from_vec3(face_vertices[1]);
        transformed_vertices[2] = vec4_from_vec3(face_vertices[2]);
        
        // 3D wolrd matrix multiply each face vertex (just changing axis scale, rotating it and shearing 3d origin)
        world_matrix = mat4_identity(); // btw order matters and cube is in the origin 
        world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
        world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
        world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix); /* TODO */
        world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
        world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);
        transformed_vertices[0] = mat4_mul_vec4(world_matrix, transformed_vertices[0]);
        transformed_vertices[1] = mat4_mul_vec4(world_matrix, transformed_vertices[1]);
        transformed_vertices[2] = mat4_mul_vec4(world_matrix, transformed_vertices[2]);
        transformed_vertices[0] = mat4_mul_vec4(view_matrix, transformed_vertices[0]);
        transformed_vertices[1] = mat4_mul_vec4(view_matrix, transformed_vertices[1]);
        transformed_vertices[2] = mat4_mul_vec4(view_matrix, transformed_vertices[2]);
        
        // CULLING checkpoint 
        vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
        vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */
        vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C   B */
        // 1. calculate AB & AC
        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);
        // 2. triangle normal with AB X AC
        vec3_t normal = vec3_cross(vector_ab, vector_ac);
        // extra: it is normal to normalize the normal (plus QWTF)
        vec3_Qnormalize(&normal);
        // 3. vector from triangle to camera
        vec3_t origin = {0, 0, 0}; // camera position is the origin
        vec3_t camera_ray = vec3_sub(origin, vector_a);
        // 4. if plane facing camera (camera_ray dot normal)
        float dot_normal_camera = vec3_dot(normal, camera_ray);
        // 5. Important: bypassing planes that don't face camera
        if (cull_method == CULL_BACKFACE)
            if (dot_normal_camera < 0) continue;
        
        // CLIPPING triangle bit by bit (pun intended)
        polygon_t polygon = create_polygon_from_triangle(vec3_from_vec4(transformed_vertices[0]), 
                                                         vec3_from_vec4(transformed_vertices[1]),
                                                         vec3_from_vec4(transformed_vertices[2]),
                                                         mesh_face.a_uv,
                                                         mesh_face.b_uv,
                                                         mesh_face.c_uv); // creates polygon from vertex duh
        // cuts with respect to RIGHT, LEFT, TOP, BOTTOM, FAR & NEAR
        clip_polygon(&polygon); 
        
        // reclipping polygon into triangles array
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;
        triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);
        
        // finalize clippling by taking what i was doing with one triangle but for all triangles generated
        for (int t = 0; t < num_triangles_after_clipping; t++)
        {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];
            
            // Projecting 3d vertex in 2d plane and saving the TRIANGLE (with projection matrix now)
            vec4_t projected_points[3]; // temp
            projected_points[0] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[0]);
            projected_points[1] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[1]);
            projected_points[2] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[2]);
            
            //  scale and translate to the middle of the screen
            projected_points[0].x *= (frame.width/2.0);
            projected_points[0].y *= (frame.height/2.0);
            projected_points[1].x *= (frame.width/2.0);
            projected_points[1].y *= (frame.height/2.0);
            projected_points[2].x *= (frame.width/2.0);
            projected_points[2].y *= (frame.height/2.0);
            
            projected_points[0].x += (frame.width/2.0);
            projected_points[0].y += (frame.height/2.0);
            projected_points[1].x += (frame.width/2.0);
            projected_points[1].y += (frame.height/2.0);
            projected_points[2].x += (frame.width/2.0);
            projected_points[2].y += (frame.height/2.0);
            
            // calculate color based in light vector and face normal
            float light_intensity_factor = -vec3_dot(normal, get_light_direction()); //neg cause normal & camera are opposite
            uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);
            
            triangle_t triangle_to_render = // filling 2D triangle created
            {
                .points = {
                    {projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                    {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                    {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w},
                },
                .texcoords = 
                {
                    {triangle_after_clipping.texcoords[0].u, triangle_after_clipping.texcoords[0].v}, // 2D coords for uv
                    {triangle_after_clipping.texcoords[1].u, triangle_after_clipping.texcoords[1].v},
                    {triangle_after_clipping.texcoords[2].u, triangle_after_clipping.texcoords[2].v},
                },
                .color = triangle_color,
                .texture = mesh->texture
            };
            
            // storing result into array to render
            writeArrayT(arrayT, triangle_to_render);
        }
    }
    
}
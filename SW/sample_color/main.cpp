#include <iostream>
#include <chrono>
#include <windows.h>
#include <string>
#include <thread> 
#include <mutex>
#include <condition_variable>
#include <winnt.h>
#include "SerialPort.h"
#include "WinDesktopDup.h"

WinDesktopDup dup;

int screenW = 0; // width of main monitor
int screenH = 0; // height of main 
uint8_t *screen; // monitor pixels BGRA matrix (RGBA inverted)

const uint8_t HLEDS = 15; // # of leds on upper/lower side of screen
const uint8_t VLEDS = 8; // # of leds on right/left side of screen
const int LEDTOT = (HLEDS * 2) + (VLEDS * 2);
const int ledValuesLength = (LEDTOT * 3 * 3) + 1; // (LEDTOT * nOfChannels * charperChannel) + \n
char ledValues[ledValuesLength] = { '0' }; // example: [25525525517052001255255255...] --> led n.2 is [170,52,1]

int ledValuesEz[LEDTOT * 3] = { 0 };

bool terminates = false;
int t_count = 0; // thread counter
std::mutex mu_t_count; // protects t_count
std::mutex mu_exec;
std::condition_variable cv_count;
std::condition_variable cv_execute;

const int BAUDRATE = 115200;
std::thread threads[LEDTOT];

const double pixelSize = 0.26; // mm
const int tw = 128; // tile width
const int th = 135; // tile height

bool init()
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    if (!GetWindowRect(hDesktop, &desktop))
        return false;

    screenH = desktop.bottom;
    screenW = desktop.right;

    screen = (uint8_t*) malloc(screenW * screenH * 4); // TODO free here at the end? uint8_t instead of BYTE?
    if (!screen)
    {
        printf("Can't allocate requested buffer");
        return false;
    }

    ledValues[ledValuesLength - 1] = '\0'; // this will not change

    return true;
}

void computeTileAvgColor(int startX, int startY, int tileWidth, int tileHeight, int step, int ledIndex)
{
    int yCoord = 0;
    int totPixel = (tileWidth / step) * (tileHeight / step) ;
    int r = 0, g = 0, b = 0;

    std::string red;
    std::string green;
    std::string blue;

    for (int y = startY; y < startY + tileHeight; y += step)
    {
        yCoord = y * screenW;
        for (int x = startX; x < startX + tileWidth; x += step)
        {
            r += screen[4 * (yCoord + x) + 2];
            g += screen[4 * (yCoord + x) + 1];
            b += screen[4 * (yCoord + x)];
        }
    }

    r /= totPixel;
    g /= totPixel;
    b /= totPixel;

    red = std::to_string(r);
    green = std::to_string(g);
    blue = std::to_string(b);

    // add leading zeroes padding if needed
    if (r < 10)
        red.insert(0, 2, '0');
    else if (r < 100)
        red.insert(0, 1, '0');

    if (r < 10)
        green.insert(0, 2, '0');
    else if (r < 100)
        green.insert(0, 1, '0');

    if (r < 10)
        blue.insert(0, 2, '0');
    else if (r < 100)
        blue.insert(0, 1, '0');

    ledIndex *= 3;
    // save result in clear for debug
    ledValuesEz[ledIndex] = r;
    ledValuesEz[ledIndex + 1] = g;
    ledValuesEz[ledIndex + 2] = b; 

    ledIndex *= 3;
    // save result as chars to be sent over serial port
    ledValues[ledIndex + 0] = red[0];
    ledValues[ledIndex + 1] = red[1];
    ledValues[ledIndex + 2] = red[2];
    ledValues[ledIndex + 3] = green[0];
    ledValues[ledIndex + 4] = green[1];
    ledValues[ledIndex + 5] = green[2];
    ledValues[ledIndex + 6] = blue[0];
    ledValues[ledIndex + 7] = blue[1];
    ledValues[ledIndex + 8] = blue[2];
}

void drawRectangle(int r, int g, int b, int startX, int startY, int endX, int endY)
{
    HDC screenDC = GetDC(NULL); //NULL gets whole screen
    HBRUSH brush = CreateSolidBrush(RGB(r, g, b)); //create brush
    SelectObject(screenDC, brush); //select brush into DC
    Rectangle(screenDC, startX, startY, endX, endY); //draw rectangle over whole screen

    //clean up stuff here
    DeleteDC(screenDC);
}

void drawPixel(int r, int g, int b, int x, int y)
{
    HDC screenDC = GetDC(NULL); //NULL gets whole screen
    //SelectObject(screenDC); //select brush into DC
    SetPixel(screenDC, x, y, RGB(r, g, b));

    //clean up stuff here
    DeleteDC(screenDC);
}

void threadWork(int startX, int startY, int tileWidth, int tileHeight, int step, int ledIndex)
{
    while (!terminates)
    {
        // start sync barrier
        std::unique_lock <std::mutex> locker1(mu_exec); // ask to access var
        cv_execute.wait(locker1); // auto releases the locker1 when waking

        // work ...
        computeTileAvgColor(startX, startY, tileWidth, tileHeight, step, ledIndex);
        //printf("executed %d \n", ledIndex);

        // end sync barrier       
        std::unique_lock <std::mutex> locker2(mu_t_count); // ask to access var
        t_count--;
        if(t_count == 0) cv_count.notify_one(); // sad this can't be optimized
        locker2.unlock();
    }
}

void visualizeColors()
{
    // create top side threads, starting left
    int l = 0;
    for (int t = 0; t < HLEDS; t++)
    {
        drawRectangle(ledValuesEz[l], ledValuesEz[l + 1], ledValuesEz[l + 2], 1920 + (tw * t), 0, 1920 + (tw * t) + tw, th);
        l += 3;
    }

    // create right side threads, starting top
    for (int t = 0; t < VLEDS; t++)
    {
        drawRectangle(ledValuesEz[l], ledValuesEz[l + 1], ledValuesEz[l + 2], 1920 + screenW - tw, th * t, 1920 + screenW, (th * t) + th);
        l += 3;
    }

    // create bottom side threads, starting right
    for (int t = HLEDS - 1; t >= 0; t--)
    {
        drawRectangle(ledValuesEz[l], ledValuesEz[l + 1], ledValuesEz[l + 2], 1920 + (tw * t), screenH - th, 1920 + (tw * t) + tw, 1080);
        l += 3;
    }

    // create left side threads, starting bottom
    for (int t = VLEDS - 1; t >= 0; t--)
    {
        drawRectangle(ledValuesEz[l], ledValuesEz[l + 1], ledValuesEz[l + 2], 1920, th * t, 1920 + tw, (th * t) + th);
        l += 3;
    }
}

bool initThreads(std::thread *threads)
{
    int ledIndex = 0;

    // create top side threads, starting left
    for (int t = 0; t < HLEDS; t++)
    {
        threads[ledIndex] = std::thread(threadWork, tw * t, 0, tw, th, 2, ledIndex);
        ledIndex++;
    }

    // create right side threads, starting top
    for (int t = 0; t < VLEDS; t++)
    {
        threads[ledIndex] = std::thread(threadWork, screenW - tw, th * t, tw, th, 2, ledIndex);
        ledIndex++;
    }

    // create bottom side threads, starting right
    for (int t = HLEDS  - 1; t >= 0; t--)
    {
        threads[ledIndex] = std::thread(threadWork, tw * t, screenH - th, tw, th, 2, ledIndex);
        ledIndex++;
    }

    // create left side threads, starting bottom
    for (int t = VLEDS - 1; t >= 0; t--)
    {
        threads[ledIndex] = std::thread(threadWork, 0, th * t, tw, th, 2, ledIndex);
        ledIndex++;
    }

    return true;
}

int main()
{
    if(!init())
        return -1;

    char comport[] = "COM14";
    SerialPort serialPort(&comport[0], BAUDRATE);
    if(!serialPort.isConnected())
        return -1;

    if(!dup.Initialize())
        return -1;

    if(!initThreads(&threads[0]))
        return -1; 
            
    while (true)
    {
        dup.CaptureFrame(screen);

        auto t1 = std::chrono::high_resolution_clock::now();

        // start threads
        t_count = LEDTOT;
        cv_execute.notify_all();

        // wait for threads to finish
        std::unique_lock <std::mutex> locker(mu_t_count); // ask to access var
        cv_count.wait(locker, []() {return t_count == 0;}); // auto releases the locker1 when waking

        //visualizeColors();

        //std::cout << ledValues << std::endl;
        // transmit over UART
        serialPort.Write(ledValues, ledValuesLength - 1);
        printf("%s", ledValues);
        printf("\n------\n");

        auto t2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << ms_double.count()/1000.00 << "s\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
        
    for (uint8_t t = 0; t < LEDTOT; t++)
        threads[t].join();

    free(screen);

    return 0;
}

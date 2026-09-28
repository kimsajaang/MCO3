#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

//colors according to ansi table
const string GREEN = "\033[32m";
const string YELLOW = "\033[33m";
const string CYAN = "\033[36m";
const string MAGENTA = "\033[35m";
const string RED = "\033[31m";
const string OG = "\033[0m";

//colors the marquee text cycles through every time it bounces
const string BOUNCE_COLORS[] = { GREEN, YELLOW, CYAN, MAGENTA, RED };
const int COLOR_COUNT = 5;

//shared variables between the main thread and the marquee thread
atomic<bool> isMarqueeRunning(false);  // true = marquee animation is currently active
atomic<bool> appRunning(true);         // true = program is still running, false = time to shut down
atomic<int> marqueeSpeed(500);         // how many milliseconds to wait between each animation frame


string marqueeText = "CSOPESY - OS Emulator Marquee";

mutex consoleMutex;  // protects console cursor movement / drawing since only 1 thread can write here once at a time
mutex textMutex;      // protects marqueeText from being read/written at since ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


const int MARQUEE_ROW = 11;    //line where the blank space of the box is located
const int MARQUEE_WIDTH = 60;  // how many characters wide the box interior is
const int MARQUEE_HEIGHT = 6;  // how many lines tall the box interior is, so the text can also move up and down


//turns on ansi support so the colors and scroll region work on older consoles too
void enableAnsi() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}


//this function draws one line of text at the fixed marquee row,
//without disturbing wherever the user is currently typing.
//it now draws every line of the box (frame[0] is the top line), so the text can move up and down too
void drawMarqueeAtBox(const string frame[], const string& color) {
#ifdef _WIN32

    lock_guard<mutex> lock(consoleMutex);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD originalPos = csbi.dwCursorPosition;


    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);


    //draw each line of the box, one below the other
    for (int r = 0; r < MARQUEE_HEIGHT; r++) {
        COORD marqueePos;
        marqueePos.X = 1;
        marqueePos.Y = MARQUEE_ROW + r;
        SetConsoleCursorPosition(hConsole, marqueePos);
        cout << color << frame[r] << OG << flush;
    }


    SetConsoleCursorPosition(hConsole, originalPos);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#endif
}

//fills the whole box with spaces, used when the marquee is stopped
void clearMarqueeBox() {
    string blank[MARQUEE_HEIGHT];
    for (int r = 0; r < MARQUEE_HEIGHT; r++) {
        blank[r] = string(MARQUEE_WIDTH, ' ');
    }
    drawMarqueeAtBox(blank, YELLOW);
}

void setupScrollRegion() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // scroll only from row 14 (1-based) down to the bottom of the window,
    // then park the cursor at the first scrollable row after the header
    //the row number now depends on the box height and the hint line under the box (with a height of 1 it is 15)
    int scrollTop = MARQUEE_ROW + MARQUEE_HEIGHT + 3;
    cout << "\033[" << scrollTop << ";" << rows << "r" << "\033[" << (scrollTop + 1) << ";1H" << flush;
#endif
}

void printHeader() {
#ifdef _WIN32
    // Make sure we always start drawing from the very top-left
    // corner of the console, so the layout stays consistent.
    COORD topPos;
    topPos.X = 0;
    topPos.Y = 0;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), topPos);
#endif


    cout << R"(   ______ _____ ____  ____  ________________  )" << "\n";
    cout << R"(  / ____// ___// __ \/ __ \/ ____/ ___/\ \/ / )" << "\n";
    cout << R"( / /     \__ \/ / / / /_/ / __/  \__ \  \  /  )" << "\n";
    cout << R"(/ /___  ___/ / /_/ / ____/ /___ ___/ /  / /   )" << "\n";
    cout << R"(\____/ /____/\____/_/   /_____//____/  /_/    )" << "\n\n";

    cout << GREEN << "Welcome to CSOPESY OS Emulator!" << OG << "\n";
    cout << "Group developer: Fabregas, Matthew M. | Ibanez, Kane Joshua T. | Li, Bowen | Teoxon, Jat R. \n";
    cout << "Version date: Sept 18, 2026\n\n";

    //marquee box frame
    cout << CYAN << "+" << string(MARQUEE_WIDTH, '-') << "+" << OG << "\n";
    for (int r = 0; r < MARQUEE_HEIGHT; r++) {
        cout << CYAN << "|" << string(MARQUEE_WIDTH, ' ') << "|" << OG << "\n";
    }
    cout << CYAN << "+" << string(MARQUEE_WIDTH, '-') << "+" << OG << "\n";

    //hint under the box, it is part of the header so it stays pinned and never scrolls away
    cout << YELLOW << "Type 'help' for commands" << OG << "\n\n";
}


void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    printHeader();
    setupScrollRegion();
}


void runExitAnimation() {
    cout << "\n" << YELLOW << "Command Line Interface shutting down" << OG;

    // Print three dots, one every 250 milliseconds, so it looks
    // like something is happening instead of the program just
    // freezing for a second.
    for (int i = 0; i < 3; ++i) {
        this_thread::sleep_for(chrono::milliseconds(250));
        cout << YELLOW << "." << flush;
    }
    cout << "\n\n";

    const int barWidth = 30;

    for (int i = 0; i <= barWidth; ++i) {
        cout << "\r" << CYAN << "[";

        for (int j = 0; j < i; ++j) {
            cout << "=";
        }


        if (i < barWidth) {
            cout << ">";
        }

        int emptyStart;
        if (i < barWidth) {
            emptyStart = i + 1;
        }
        else {
            emptyStart = i;
        }
        for (int j = emptyStart; j < barWidth; ++j) {
            cout << " ";
        }

        cout << "] " << (i * 100 / barWidth) << "%" << OG << flush;

        this_thread::sleep_for(chrono::milliseconds(25));
    }

    cout << "\n\n" << GREEN << "System shutdown complete. Goodbye!" << OG << "\n\n";
    this_thread::sleep_for(chrono::milliseconds(400));
}


//gives back either 1 or -1 at random, used to pick a new direction
int randomDirection() {
    if (rand() % 2 == 0) {
        return 1;
    }
    else {
        return -1;
    }
}


void marqueeWorker() {
    int posX = 0;   // current horizontal position (column) of the text inside the box
    int posY = 0;   // current vertical position (line) of the text inside the box
    int dirX = randomDirection();   // 1 = moving right, -1 = moving left (random at the start)
    int dirY = randomDirection();   // 1 = moving down, -1 = moving up (random at the start)
    int colorIndex = 0;             // which color from BOUNCE_COLORS the text is using right now

    while (appRunning) {
        if (isMarqueeRunning) {


            string currentText;
            {
                lock_guard<mutex> lock(textMutex);
                currentText = marqueeText;
            }

            //if the text is longer than the box, only show the part that fits
            if (static_cast<int>(currentText.length()) > MARQUEE_WIDTH) {
                currentText = currentText.substr(0, MARQUEE_WIDTH);
            }

            int textLen = static_cast<int>(currentText.length());


            //maxX and maxY are the furthest the text can go before it would touch the edge of the box
            int maxX = MARQUEE_WIDTH - textLen;
            int maxY = MARQUEE_HEIGHT - 1;

            //if set_text made the text longer while running, keep the position inside the box
            if (posX > maxX) {
                posX = maxX;
            }
            if (posY > maxY) {
                posY = maxY;
            }


            //build the whole box: every line is blank except the line where the text is
            string frame[MARQUEE_HEIGHT];
            for (int r = 0; r < MARQUEE_HEIGHT; r++) {
                frame[r] = string(MARQUEE_WIDTH, ' ');
            }
            frame[posY].replace(posX, textLen, currentText);

            //check again before drawing so a stop_marquee that just blanked the box isn't overwritten
            if (!isMarqueeRunning) {
                continue;
            }

            drawMarqueeAtBox(frame, BOUNCE_COLORS[colorIndex]);

            //move one step diagonally
            posX = posX + dirX;
            posY = posY + dirY;

            //find out which edges of the box the text touched
            bool hitX = false;
            bool hitY = false;

            if (posX >= maxX) {
                posX = maxX;
                dirX = -1;
                hitX = true;
            }
            else if (posX <= 0) {
                posX = 0;
                dirX = 1;
                hitX = true;
            }

            if (posY >= maxY) {
                posY = maxY;
                dirY = -1;
                hitY = true;
            }
            else if (posY <= 0) {
                posY = 0;
                dirY = 1;
                hitY = true;
            }

            //if the text touched any edge, switch to the next color (it shows on the next frame)
            if (hitX || hitY) {
                colorIndex = colorIndex + 1;
                if (colorIndex >= COLOR_COUNT) {
                    colorIndex = 0;
                }
            }

            //hit a side edge only -> bounce off it and pick a random up/down direction
            //hit the top/bottom edge only -> bounce off it and pick a random left/right direction
            //hit a corner (both) -> both directions already bounced back
            if (hitX && !hitY) {
                dirY = randomDirection();
            }
            else if (hitY && !hitX) {
                dirX = randomDirection();
            }

            //sleep in 50ms chunks instead of one long sleep so stop/exit respond right away
            //the total wait is still marqueeSpeed
            int remaining = marqueeSpeed.load();
            while (remaining > 0 && appRunning && isMarqueeRunning) {
                int chunk;
                if (remaining < 50) {
                    chunk = remaining;
                }
                else {
                    chunk = 50;
                }
                this_thread::sleep_for(chrono::milliseconds(chunk));
                remaining = remaining - chunk;
            }

        }
        else {
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}

// help commands
void showHelp() {
    cout << "\n" << MAGENTA << "Available Commands:" << OG << "\n";
    cout << "  help          - displays the commands and its description\n";
    cout << "  start_marquee - starts the marquee \"animation\"\n";
    cout << "  stop_marquee  - stops the marquee \"animation\"\n";
    cout << "  set_text      - accepts a text input and displays it as a marquee\n";
    cout << "  set_speed     - sets the marquee animation refresh in milliseconds\n";
    cout << "  clear         - clears the console screen\n";
    cout << "  exit          - terminates the console\n\n";
}

int main() {
    enableAnsi();

    //seed the random numbers so the direction is different every run
    srand(static_cast<unsigned int>(time(0)));

    clearScreen();

    thread worker(marqueeWorker);

    string command;

    while (appRunning) {
        cout << "Command> ";

        if (!getline(cin, command)) {

            break;
        }

        if (command.empty()) {

            continue;
        }

        //split the typed line into the command and its argument
        //e.g. "set_text Hello world!" -> cmd = "set_text", arg = "Hello world!"
        string cmd = command;
        string arg = "";

        //look for the first space in the typed line, -1 means there is no space
        int spaceIndex = -1;
        int commandLen = static_cast<int>(command.length());
        for (int i = 0; i < commandLen; i++) {
            if (command[i] == ' ') {
                spaceIndex = i;
                break;
            }
        }

        //if there is a space, everything before it is the command and everything after it is the argument
        if (spaceIndex != -1) {
            cmd = command.substr(0, spaceIndex);
            arg = command.substr(spaceIndex + 1);

            //remove extra spaces in front of the argument
            while (arg.length() > 0 && arg[0] == ' ') {
                arg.erase(0, 1);
            }
        }

        if (cmd == "help") {
            showHelp();
        }
        else if (cmd == "start_marquee") {
            if (isMarqueeRunning) {
                cout << YELLOW << "Marquee is already running." << OG << "\n\n";
            }
            else {
                isMarqueeRunning = true;
                cout << GREEN << "Marquee started." << OG << "\n\n";
            }
        }
        else if (cmd == "stop_marquee") {
            if (!isMarqueeRunning) {
                cout << YELLOW << "Marquee is already stopped." << OG << "\n\n";
            }
            else {
                isMarqueeRunning = false;

                clearMarqueeBox();
                cout << RED << "Marquee stopped." << OG << "\n\n";
            }
        }
        else if (cmd == "set_text") {
            //use the text typed after set_text, if none was given then ask for it
            string newText = arg;
            if (newText.empty()) {
                cout << "Enter marquee text: ";
                getline(cin, newText);
            }

            if (!newText.empty()) {

                lock_guard<mutex> lock(textMutex);
                marqueeText = newText;
                cout << GREEN << "Marquee text set to: \"" << marqueeText << "\"" << OG << "\n\n";
            }
            else {
                cout << YELLOW << "Text cannot be empty." << OG << "\n\n";
            }
        }
        else if (cmd == "set_speed") {
            //use the number typed after set_speed, if none was given then ask for it
            string s = arg;
            if (s.empty()) {
                cout << "Enter refresh speed in ms (e.g. 50-1000): ";
                getline(cin, s);
            }

            //check every character, the input is only valid if all of them are digits (0-9)
            //the length limit of 5 keeps the number from getting too big for an int
            int speed = 0;
            int sLen = static_cast<int>(s.length());
            bool validInput = true;

            if (sLen == 0 || sLen > 5) {
                validInput = false;
            }

            for (int i = 0; i < sLen; i++) {
                if (s[i] < '0' || s[i] > '9') {
                    validInput = false;
                }
                else {
                    speed = speed * 10 + (s[i] - '0');
                }
            }

            if (validInput && speed > 0) {
                marqueeSpeed = speed;
                cout << GREEN << "Marquee speed set to: " << speed << " ms" << OG << "\n\n";
            }
            else {
                cout << RED << "Invalid speed. Please enter a positive integer." << OG << "\n\n";
            }
        }
        else if (cmd == "clear") {
            clearScreen();
        }
        else if (cmd == "exit") {
            isMarqueeRunning = false;
            appRunning = false;
            runExitAnimation();
            break;
        }
        else {
            cout << RED << "Unknown command: \"" << command << "\". Type 'help' for instructions." << OG << "\n\n";
        }
    }

    //make sure the worker stops even if the loop ended because input closed
    appRunning = false;

    if (worker.joinable()) {
        worker.join();
    }

    //put normal terminal scrolling back
    cout << "\033[r" << flush;

    return 0;
}
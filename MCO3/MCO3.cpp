#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

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

//shared variables between the main thread and the marquee thread
atomic<bool> isMarqueeRunning(false);  // true = marquee animation is currently active
atomic<bool> appRunning(true);         // true = program is still running, false = time to shut down
atomic<int> marqueeSpeed(100);         // how many milliseconds to wait between each animation frame


string marqueeText = "CSOPESY - OS Emulator Marquee";

mutex consoleMutex;  // protects console cursor movement / drawing since only 1 thread can write here once at a time
mutex textMutex;      // protects marqueeText from being read/written at since ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


const int MARQUEE_ROW = 11;    //line where the blank space of the box is located
const int MARQUEE_WIDTH = 60;  // how many characters wide the box interior is


//this function draws one line of text at the fixed marquee row,
//without disturbing wherever the user is currently typing.
void drawMarqueeAtBox(const string& content) {
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

   
    COORD marqueePos;
    marqueePos.X = 1;              
    marqueePos.Y = MARQUEE_ROW;    
    SetConsoleCursorPosition(hConsole, marqueePos);
    cout << YELLOW << content << OG << flush;


    SetConsoleCursorPosition(hConsole, originalPos);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#endif
}

void setupScrollRegion() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // scroll only from row 14 (1-based) down to the bottom of the window,
    // then park the cursor at the first scrollable row after the header
    cout << "\033[14;" << rows << "r" << "\033[15;1H" << flush;
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
    cout << CYAN << "|" << string(MARQUEE_WIDTH, ' ') << "|" << OG << "\n";
    cout << CYAN << "+" << string(MARQUEE_WIDTH, '-') << "+" << OG << "\n\n";
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



void marqueeWorker() {
    int position = 0;   // current horizontal position of the text inside the box
    int direction = 1;   // 1 = moving right, -1 = moving left

    while (appRunning) {
        if (isMarqueeRunning) {

          
            string currentText;
            {
                lock_guard<mutex> lock(textMutex);
                currentText = marqueeText;
            }

            int textLen = static_cast<int>(currentText.length());

          
            int maxPos = MARQUEE_WIDTH - textLen;
            if (maxPos < 0) {
                maxPos = 0;
            }

         
            string line = string(MARQUEE_WIDTH, ' ');

            if (maxPos > 0) {
                line.replace(position, textLen, currentText);
            }
            else {
                line = currentText.substr(0, MARQUEE_WIDTH);
            }

            drawMarqueeAtBox(line);

            if (maxPos > 0) {
                position = position + direction;

                if (position >= maxPos) {
                    position = maxPos;
                    direction = -1;
                }

                else if (position <= 0) {
                    position = 0;
                    direction = 1;
                }
            }

            this_thread::sleep_for(chrono::milliseconds(marqueeSpeed.load()));

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

   

        if (command == "help") {
            showHelp();
        }
        else if (command == "start_marquee") {
            if (isMarqueeRunning) {
                cout << YELLOW << "Marquee is already running." << OG << "\n\n";
            }
            else {
                isMarqueeRunning = true;
                cout << GREEN << "Marquee started." << OG << "\n\n";
            }
        }
        else if (command == "stop_marquee") {
            if (!isMarqueeRunning) {
                cout << YELLOW << "Marquee is already stopped." << OG << "\n\n";
            }
            else {
                isMarqueeRunning = false;
               
                drawMarqueeAtBox(string(MARQUEE_WIDTH, ' '));
                cout << RED << "Marquee stopped." << OG << "\n\n";
            }
        }
        else if (command == "set_text") {
            cout << "Enter marquee text: ";
            string newText;
            getline(cin, newText);

            if (!newText.empty()) {
             
                lock_guard<mutex> lock(textMutex);
                marqueeText = newText;
                cout << GREEN << "Marquee text set to: \"" << marqueeText << "\"" << OG << "\n\n";
            }
            else {
                cout << YELLOW << "Text cannot be empty." << OG << "\n\n";
            }
        }
        else if (command == "set_speed") {
            cout << "Enter refresh speed in ms (e.g. 50-500): ";
            int speed;


            bool validInput = (cin >> speed) ? true : false;

            if (validInput && speed > 0) {
                marqueeSpeed = speed;
                cin.ignore();
                cout << GREEN << "Marquee speed set to: " << speed << " ms" << OG << "\n\n";
            }
            else {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << RED << "Invalid speed. Please enter a positive integer." << OG << "\n\n";
            }
        }
        else if (command == "clear") {
            clearScreen();
        }
        else if (command == "exit") {
            isMarqueeRunning = false;
            appRunning = false;
            runExitAnimation();
            break;
        }
        else {
            cout << RED << "Unknown command: \"" << command << "\". Type 'help' for instructions." << OG << "\n\n";
        }
    }
    if (worker.joinable()) {
        worker.join();
    }

    return 0;
}
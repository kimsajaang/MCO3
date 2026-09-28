CSOPESY MARQUEE CONSOLE

This is our CSOPESY Machine Project. It is a simple command-line OS emulator
made in C++. The program has a console interface and a text marquee that can
be controlled using commands.

SOURCE CODE LINK
https://github.com/kimsajaang/MCO3

GROUP MEMBERS
- Fabregas, Matthew M.
- Ibanez, Kane Joshua T.
- Li, Bowen
- Teoxon, Jat R.

WHAT THE PROGRAM DOES
The program displays a console screen with a moving marquee. The user can
start and stop the marquee, change the text, and change how fast it refreshes.
The marquee runs on a separate thread so the user can still type commands
while it is moving.

REQUIREMENTS
- Windows
- Visual Studio with Desktop development with C++ installed
- C++17 or a newer C++ standard

HOW TO RUN
1. Open MCO3.sln in Visual Studio.
2. Choose Debug and x64 or x86 as the configuration.
3. Build the solution using Build > Build Solution.
4. Run the program using Debug > Start Without Debugging or press Ctrl+F5.
5. Type a command after Command> and press Enter.

ENTRY POINT
This is a C++ console program, so it does not have an entry class.
The main function is located in the following file:
MCO3/MCO3.cpp

The entry function is:
int main()

COMMANDS
help          Shows the available commands.
start_marquee Starts the marquee animation.
stop_marquee  Stops the marquee animation.
set_text      Lets the user enter new marquee text.
set_speed     Changes the refresh speed in milliseconds.
clear         Clears and redraws the console screen.
exit          Stops the program and shows the shutdown animation.

EXAMPLE
Command> start_marquee
Command> set_text
Enter marquee text: Hello CSOPESY
Command> set_speed
Enter refresh speed in ms (e.g. 50-500): 100
Command> stop_marquee
Command> exit

IMPORTANT NOTE
The speed must be a positive whole number. Smaller numbers make the marquee
update faster.
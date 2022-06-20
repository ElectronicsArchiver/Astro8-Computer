
#include <vector>
#include <algorithm> 
#include <string> 
#include <chrono>
#include <limits.h>
#include <SDL.h>
#include <iostream>
#include <sstream>


using namespace std::chrono;
using namespace std;


int AReg = 0;
int BReg = 0;
int InstructionReg = 0;
int flags[3] = { 0, 0, 0 };
int bus = 0;
int outputReg = 0;
int memoryIndex = 0;
int programCounter = 0;

int imgX = 0;
int imgY = 0;

float slowdownAmnt = 1;
int iterations = 0;

vector<int> memoryBytes;

string instructions[16] = { "NOP", "LODA", "LODB", "ADD", "SUB", "OUT", "JMP", "STA", "LDI", "JMPZ", "JMPC", "HLT", "LDAIN", "", "", "" };
string action = "";
vector<vector<int>> microinstructionData;

SDL_Rect r;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The renderer we'll be rendering to
SDL_Renderer* gRenderer = NULL;

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

// Function List
bool Update(float deltatime);
void DrawPixel(int x, int y, int r, int g, int b);
int InitGraphics(std::string windowTitle, int width, int height, int pixelScale);
string charToString(char* a);
unsigned BitRange(unsigned value, unsigned offset, unsigned n);
string DecToHexFilled(int input, int desiredSize);
string BinToHexFilled(string input, int desiredSize);
int BinToDec(string input);
string DecToBin(int input);
string DecToBinFilled(int input, int desiredSize);
string HexToBin(string s, int desiredSize);
int HexToDec(string hex);
vector<string> explode(const string& str, const char& ch);
vector<string> parseCode(string input);
static inline void ltrim(std::string& s);
static inline void rtrim(std::string& s);
static inline string trim(std::string s);
void GenerateMicrocode();
string SimplifiedHertz(float input);

SDL_Texture* texture;
std::vector< unsigned char > pixels(64 * 64 * 4, 0);


void apply_pixels(
	std::vector<unsigned char>& pixels,
	SDL_Texture* texture,
	unsigned int screen_width)
{
	SDL_UpdateTexture
	(
		texture,
		NULL,
		pixels.data(),
		screen_width * 4
	);
}

void DisplayTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void clear_buffers(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
	SDL_RenderClear(renderer);
}

void set_pixel(
	std::vector<unsigned char>* pixels,
	int x, int y, int screen_width,
	Uint8 r, Uint8 g, Uint8 b, Uint8 a
)
{
	const unsigned int offset = (y * 4 * screen_width) + x * 4;
	(*pixels)[offset + 0] = r;        // b
	(*pixels)[offset + 1] = g;        // g
	(*pixels)[offset + 2] = b;        // r
	(*pixels)[offset + 3] = a;    // a
}

void destroy(SDL_Renderer* renderer, SDL_Window* window)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}


int main(int argc, char** argv)
{

	//InstructionReg = 100;
	////int b = (BitRange(InstructionReg, 6, 4) * 64) + (0 * 4) + (flags[0] * 2) + flags[1];
	//unsigned b = BitRange(640, 6, 4); // Should output 10011010010 to 1101 (13)
	//cout << b << " == 0b" << DecToBin(b) << endl;

	// Gather user inputted code
	cout << ("v Emu. Code input v\n");
	string code = "";
	string line;
	while (true) {
		getline(cin, line);
		if (line.empty()) {
			break;
		}
		code += line + "\n";
	}

	// Generate memory from code and convert from hex to decimal
	vector<string> mbytes = parseCode(code);
	for (int memindex = 0; memindex < mbytes.size(); memindex++)
		memoryBytes.push_back(HexToDec(mbytes[memindex]));

	GenerateMicrocode();

	cout << endl;
	cout << DecToBin(16) << endl;
	cout << HexToDec("ffff") << endl;
	cout << HexToBin("ffff", 17) << endl;
	cout << endl;

	InitGraphics("Astro-8 Emulator", 64, 64, 6);



	float dt = 0;
	bool running = true;
	while (running)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = false;
			}
		}


		Update(dt);

		// Calculate frame time
		auto stopTime = std::chrono::high_resolution_clock::now();
		dt = std::chrono::duration<float, std::chrono::milliseconds::period>(stopTime - startTime).count() / 1000.0f;
	}


	//SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
	//SDL_RenderClear(gRenderer);

	destroy(gRenderer, gWindow);
	SDL_Quit();

	return 0;
}

steady_clock::time_point start;
float renderedFrameTime = 0;
bool Update(float deltatime)
{
	renderedFrameTime += deltatime;

	//cout << programCounter << ")  ";
	for (int step = 0; step < 16; step++)
	{

		//cout<<("     microcode: " + mcode)<<endl;

		if (step == 0)
		{
			// CR
			// AW
			memoryIndex = programCounter;
			// RM
			// IW
			InstructionReg = memoryBytes[memoryIndex];
			// CE
			programCounter += 1;
			step = 1;
			continue;
		}
		//cout << "step:" << step << endl;
		int microcodeLocation = (BitRange((unsigned)InstructionReg, 12, 4) * 64) + (step * 4) + (flags[0] * 2) + flags[1];
		vector<int> mcode = microinstructionData[microcodeLocation];

		//cout << ("\nmcLoc- " + DecToBinFilled(InstructionReg, 16).substr(0, 4) + DecToBinFilled(step, 4) + to_string(flags[0]) + to_string(flags[1])) << "  ==  " << microcodeLocation << endl;
		//cout << ("mcDat- " + mcode) << endl;

		//while (memoryIndex >= 4000)
		//    memoryIndex -= 4000;
		//if (memoryIndex < 0)
		//    memoryIndex = -memoryIndex;

		//cout<<("ftmem=" + memoryIndex);
		// 0-su  1-iw  2-dw  3-st  4-ce  5-cr  6-wm  7-ra  8-eo  9-fl  10-j  11-wb  12-wa  13-rm  14-aw  15-ir  16-ei
		// Execute microinstructions
		if (mcode[8] == 1)
		{ // EO
			//cout << ("EO ");
			if (mcode[0] == 1) // SU
			{
				flags[0] = 0;
				flags[1] = 1;
				if (AReg - BReg == 0)
					flags[0] = 1;
				bus = AReg - BReg;
				if (bus < 0)
				{
					bus = 65535 + bus;
					flags[1] = 0;
				}
			}
			else
			{
				flags[0] = 0;
				flags[1] = 0;
				if (AReg + BReg == 0)
					flags[0] = 1;
				bus = AReg + BReg;
				if (bus >= 65535)
				{
					bus = bus - 65535;
					flags[1] = 1;
				}
			}
		}
		if (mcode[5] == 1)
		{ // CR
			//cout << ("CR ");
			bus = programCounter;
		}
		if (mcode[7] == 1)
		{ // RA
			//cout << ("RA ");
			bus = AReg;
		}
		if (mcode[13] == 1)
		{ // RM
			//cout << ("RM ");
			//cout << "\nmemread: " + to_string(memoryIndex )+ " " + to_string(memoryBytes[memoryIndex] )+ "\n";
			bus = memoryBytes[memoryIndex];
		}
		if (mcode[15] == 1)
		{ // IR
			//cout << ("IR ");
			bus = BitRange(InstructionReg, 0, 12);
		}
		if (mcode[1] == 1)
		{ // IW
			//cout << ("IW ");
			InstructionReg = bus;
		}
		if (mcode[2] == 1)
		{ // DW
			//cout << ("DW ");
			outputReg = bus;
			//cout << ("\no: " + to_string(outputReg) + " A: " + to_string(AReg) + " B: " + to_string(BReg) + " bus: " + to_string(bus) + " Ins: " + to_string(InstructionReg) + " img:(" + to_string(imgX) + ", " + to_string(imgY) + ")\n");

			// Write to LED screen
			int r = BitRange(bus, 10, 15) * 8; // Get first 5 bits
			int g = BitRange(bus, 5, 10) * 8; // get middle bits
			int b = BitRange(bus, 0, 5) * 8; // Gets last 5 bits


			//set_pixel(&pixels, imgX, imgY, 64, r, g, b, 255);
			DrawPixel(imgX, imgY, r, g, b);

			imgX++;
			if (imgX >= 64)
			{
				imgY++;
				imgX = 0;
			}
			if (imgY >= 64) // The final layer is done, reset counter and render image
			{
				imgY = 0;

				// Apply pixels and render
				SDL_SetRenderDrawColor(gRenderer, 60, 60, 60, SDL_ALPHA_OPAQUE);
				SDL_RenderClear(gRenderer);
				apply_pixels(pixels, texture, 64);
				DisplayTexture(gRenderer, texture);

				cout << "\r                " << "\r" << SimplifiedHertz(1.0f / deltatime) + "\tFPS: " + to_string(1.0f / renderedFrameTime);

				renderedFrameTime = 0;
			}

		}
		if (mcode[4] == 1)
		{ // CE
			//cout << ("CE ");
			programCounter += 1;
		}
		if (mcode[6] == 1)
		{ // WM
			//cout << ("WM ");
			memoryBytes[memoryIndex] = bus;
		}
		if (mcode[10] == 1)
		{ // J
			//cout << ("J ");
			//cout<<Line(DecToBinFilled(InstructionReg, 16));
			//cout<<Line(DecToBinFilled(InstructionReg, 16).Substring(4, 12));
			programCounter = BitRange(InstructionReg, 0, 12);
		}
		if (mcode[11] == 1)
		{ // WB
			//cout << ("WB ");
			BReg = bus;
		}
		if (mcode[12] == 1)
		{ // WA
			//cout << ("WA ");
			AReg = bus;
		}
		if (mcode[14] == 1)
		{ // AW
			//cout << ("AW ");
			memoryIndex = BitRange(bus, 0, 12);
		}
		if (mcode[3] == 1)
		{ // ST
			//cout<<("ST ");
			cout << ("\n== PAUSED from HLT ==\n\n");
			cout << ("FINAL VALUES |=  o: " + to_string(outputReg) + " A: " + to_string(AReg) + " B: " + to_string(BReg) + " bus: " + to_string(bus) + " Ins: " + to_string(InstructionReg) + " img:(" + to_string(imgX) + ", " + to_string(imgY) + ")\n");
			system("pause");
			exit(1);
		}

		if (mcode[16] == 1)
		{ // EI
			//cout<<("EI ");
			//cout<<endl;
			break;
		}
		//else
		//	cout<<endl;
	}

	//cout << ("o: " + to_string(outputReg) + " A: " + to_string(AReg) + " B: " + to_string(BReg) + " bus: " + to_string(bus) + " Ins: " + to_string(InstructionReg) + " img:(" + to_string(imgX) + ", " + to_string(imgY) + ")\n");
//}
	iterations += 1;

	return true;
}

void DrawPixel(int x, int y, int r, int g, int b)
{
	SDL_SetRenderDrawColor(gRenderer, r, g, b, 255);
	SDL_RenderDrawPoint(gRenderer, x, y);
}

string SimplifiedHertz(float input) {
	if (input >= 1000000000) // GHz
		return to_string(round(input / 1000000000.0f * 10.0f) / 10.0f) + " GHz";
	if (input >= 1000000) // MHz
		return to_string(round(input / 1000000.0f * 10.0f) / 10.0f) + " MHz";
	if (input >= 1000) // KHz
		return to_string(round(input / 1000.0f * 10.0f) / 10.0f) + " KHz";

	return to_string(round(input * 10.0f) / 10.0f) + " KHz";
}

int InitGraphics(std::string windowTitle, int width, int height, int pixelScale)
{
	int WINDOW_WIDTH = width;
	int WINDOW_HEIGHT = height;
	int PIXEL_SCALE = pixelScale;

	// Initialize SDL components
	SDL_Init(SDL_INIT_VIDEO);

	gWindow = SDL_CreateWindow(windowTitle.c_str(), 40, 40, WINDOW_WIDTH * PIXEL_SCALE, WINDOW_HEIGHT * PIXEL_SCALE, SDL_WINDOW_SHOWN | SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	SDL_RenderSetLogicalSize(gRenderer, WINDOW_WIDTH * PIXEL_SCALE, WINDOW_HEIGHT * PIXEL_SCALE);
	SDL_RenderSetScale(gRenderer, PIXEL_SCALE, PIXEL_SCALE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);

	texture = SDL_CreateTexture(
		gRenderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH, WINDOW_HEIGHT);

	//Get window surface
	gScreenSurface = SDL_GetWindowSurface(gWindow);

	return 0;
}

string charToString(char* a)
{
	string s(a);
	return s;
}
/*unsigned createMask(unsigned a, unsigned b)
{
	unsigned r = 0;
	for (unsigned i = a; i <= b; i++)
		r |= 1 << i;

	return r;
}
unsigned BitRange(unsigned x, unsigned min, unsigned max) {
	unsigned r = createMask(min, max);
	unsigned result = r & x;
	return result;
}*/

// Gets range of bits inside of an integer <value> starting at <offset> inclusive for <n> range
unsigned BitRange(unsigned value, unsigned offset, unsigned n)
{
	const unsigned max_n = CHAR_BIT * sizeof(unsigned);
	if (offset >= max_n)
		return 0; /* value is padded with infinite zeros on the left */
	value >>= offset; /* drop offset bits */
	if (n >= max_n)
		return value; /* all  bits requested */
	const unsigned mask = (1u << n) - 1; /* n '1's */
	return value & mask;
}
string DecToHexFilled(int input, int desiredSize)
{
	stringstream ss;
	ss << hex << input;
	string output(ss.str());

	while (output.length() < desiredSize)
	{
		output = "0" + output;
	}

	return output;
}
string BinToHexFilled(string input, int desiredSize)
{
	int dec = BinToDec(input);
	string output = DecToHexFilled(dec, 0);

	while (output.length() < desiredSize)
	{
		output = "0" + output;
	}

	return output;
}
int BinToDec(string input)
{
	return stoi(input, nullptr, 2);
}
string DecToBin(int input)
{
	string r;
	int n = input;
	while (n != 0) { r = (n % 2 == 0 ? "0" : "1") + r; n /= 2; }
	return r;
}
string DecToBinFilled(int input, int desiredSize)
{
	string output = DecToBin(input);

	while (output.length() < desiredSize)
	{
		output = "0" + output;
	}

	return output;
}
string HexToBin(string s, int desiredSize)
{
	string out;
	for (auto i : s) {
		uint8_t n;
		if (i <= '9' and i >= '0')
			n = i - '0';
		else
			n = 10 + i - 'A';
		for (int8_t j = 3; j >= 0; --j)
			out.push_back((n & (1 << j)) ? '1' : '0');
	}

	// Fill
	while (out.length() < desiredSize)
	{
		out = "0" + out;
	}
	if (out.length() > desiredSize)
		out = out.substr(out.length() - desiredSize);
	return out;
}

int HexToDec(string hex)
{
	unsigned long result = 0;
	for (int i = 0; i < hex.length(); i++) {
		if (hex[i] >= 48 && hex[i] <= 57)
		{
			result += (hex[i] - 48) * pow(16, hex.length() - i - 1);
		}
		else if (hex[i] >= 65 && hex[i] <= 70) {
			result += (hex[i] - 55) * pow(16, hex.length() - i - 1);
		}
		else if (hex[i] >= 97 && hex[i] <= 102) {
			result += (hex[i] - 87) * pow(16, hex.length() - i - 1);
		}
	}
	return result;
}

vector<string> explode(const string& str, const char& ch) {
	string next;
	vector<string> result;

	// For each character in the string
	for (string::const_iterator it = str.begin(); it != str.end(); it++) {
		// If we've hit the terminal character
		if (*it == ch) {
			// If we have some characters accumulated
			if (!next.empty()) {
				// Add them to the result vector
				result.push_back(next);
				next.clear();
			}
		}
		else {
			// Accumulate the next character into the sequence
			next += *it;
		}
	}
	if (!next.empty())
		result.push_back(next);
	return result;
}

vector<string> parseCode(string input)
{
	vector<string> outputBytes;
	for (int i = 0; i < 4000; i++)
		outputBytes.push_back("0000");

	string icopy = input;
	transform(icopy.begin(), icopy.end(), icopy.begin(), ::toupper);
	vector<string> splitcode = explode(icopy, '\n');

	cout << endl;

	int memaddr = 0;
	for (int i = 0; i < splitcode.size(); i++)
	{
		if (trim(splitcode[i]) == "")
		{
			continue;
		}

		vector<string> splitBySpace = explode(splitcode[i], ' ');

		if (splitBySpace[0][0] == ',')
		{
			cout << ("-\t" + splitcode[i] + "\n");
			continue;
		}
		if (splitBySpace[0] == "SET")
		{
			string hVal = DecToHexFilled(stoi(splitBySpace[2]), 4);
			outputBytes[stoi(splitBySpace[1])] = hVal;
			cout << ("-\t" + splitcode[i] + "\t  ~   ~\n");
			continue;
		}

		cout << (to_string(memaddr) + " " + splitcode[i] + "   \t  =>  ");

		// Find index of instruction
		for (int f = 0; f < sizeof(instructions) / sizeof(instructions[0]); f++)
		{
			if (instructions[f] == splitBySpace[0])
			{
				cout << (DecToHexFilled(f, 1));
				outputBytes[memaddr] = DecToHexFilled(f, 1);
			}
		}

		// Check if any args are after the command
		if (splitcode[i] != splitBySpace[0])
		{
			cout << (DecToHexFilled(stoi(splitBySpace[1]), 3));
			outputBytes[memaddr] += DecToHexFilled(stoi(splitBySpace[1]), 3);
		}
		else
		{
			cout << ("0");
			outputBytes[memaddr] += "000";
		}
		cout << ("\n");
		memaddr += 1;
	}

	/*
	cout << ("\n000:");
	for (int outindex = 0; outindex < outputBytes.size(); outindex++)
	{
		if (outindex % 8 == 0 && outindex != 0)
		{
			string locationTmp = DecToHexFilled(outindex, 3);
			transform(locationTmp.begin(), locationTmp.end(), locationTmp.begin(), ::toupper);
			cout << ("\n" + locationTmp + ":");
		}
		string bytetmp = (" " + outputBytes[outindex]);
		transform(bytetmp.begin(), bytetmp.end(), bytetmp.begin(), ::toupper);
		cout << bytetmp;
	}*/
	return outputBytes;
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline string trim(std::string s) {
	string ss = s;
	ltrim(ss);
	rtrim(ss);
	return ss;
}

void GenerateMicrocode()
{
	// Generate zeros in data
	vector<string> output;
	vector<int> ii;
	for (int osind = 0; osind < 1024; osind++) { output.push_back("00000"); microinstructionData.push_back(ii); }

	string microinstructions[] = { "SU", "IW", "DW", "ST", "CE", "CR", "WM", "RA", "EO", "FL", "J", "WB", "WA", "RM", "AW", "IR", "EI" };
	string flags[] = { "ZEROFLAG", "CARRYFLAG" };
	string instructioncodes[] = {
			"fetch( 0=aw,cr & 1=rm,iw,ce & 2=ei", // Fetch
			"loda( 2=aw,ir & 3=wa,rm & 4=ei", // LoadA
			"lodb( 2=aw,ir & 3=wb,rm & 4=ei", // LoadB
			"add( 2=aw,ir & 3=wb,rm & 4=wa,eo,fl & 5=ei", // Add <addr>
			"sub( 2=aw,ir & 3=rm,wb & 4=wa,eo,su,fl & 5=ei", // Subtract <addr>
			"out( 2=ra,dw & 3=ei", // Output to decimal display and LCD screen
			"jmp( 2=ir,j & 3=ei", // Jump <addr>
			"sta( 2=aw,ir & 3=ra,wm & 4=ei", // Store A <addr>
			"ldi( 2=wa,ir & 3=ei", // Load immediate A <val>
			"jmpz( 2=ir,j | zeroflag & 3=ei", // Jump if zero <addr>
			"jmpc( 2=ir,j | carryflag & 3=ei", // Jump if carry <addr>
			"hlt( 2=st & 3=ei", // Stop the computer clock
			"ldain( 2=ra,aw & 3=wa,rm & 4=ei", // Load from reg A as memory address, then copy value from memory into A
	};

	// Remove spaces from instruction codes and make uppercase
	for (int cl = 0; cl < sizeof(instructioncodes) / sizeof(instructioncodes[0]); cl++)
	{
		string newStr = "";
		for (int clc = 0; clc < instructioncodes[cl].length(); clc++)
		{
			if (instructioncodes[cl][clc] != ' ')
				newStr += instructioncodes[cl][clc];
		}
		transform(newStr.begin(), newStr.end(), newStr.begin(), ::toupper);
		cout << (newStr) << endl;
		instructioncodes[cl] = newStr;
	}

	// Create indexes for instructions, which allows for duplicates to execute differently for different parameters
	int instIndexes[sizeof(instructioncodes) / sizeof(instructioncodes[0])];
	vector<string> seenNames;
	for (int cl = 0; cl < sizeof(instructioncodes) / sizeof(instructioncodes[0]); cl++)
	{
		string instName = explode(instructioncodes[cl], '(')[0];
		bool foundInList = false;
		for (int clc = 0; clc < seenNames.size(); clc++)
		{
			if (instName == seenNames[clc])
			{
				instIndexes[cl] = clc;
				foundInList = true;
				break;
			}
		}
		if (!foundInList)
		{
			seenNames.push_back(instName);
			instIndexes[cl] = seenNames.size() - 1;
		}
		instructioncodes[cl] = explode(instructioncodes[cl], '(')[1];
	}

	// Special process fetch instruction
	cout << ("\n" + instructioncodes[0] + "\n");
	for (int ins = 0; ins < sizeof(instructioncodes) / sizeof(instructioncodes[0]); ins++) // Iterate through all definitions of instructions
	{
		int correctedIndex = instIndexes[ins];

		string startaddress = DecToBinFilled(correctedIndex, 4);

		vector<string> instSteps = explode(instructioncodes[0], '&');
		for (int step = 0; step < instSteps.size(); step++) // Iterate through every step
		{
			int actualStep = stoi(explode(instSteps[step], '=')[0]);
			string stepContents = explode(explode(instSteps[step], '=')[1], '|')[0];

			string midaddress = DecToBinFilled(actualStep, 4);

			string stepComputedInstruction = "";
			for (int mins = 0; mins < sizeof(microinstructions) / sizeof(microinstructions[0]); mins++)
			{
				if (stepContents.find(microinstructions[mins]) != std::string::npos)
					stepComputedInstruction += "1";
				else
					stepComputedInstruction += "0";
			}

			// Compute flags combinations
			for (int flagcombinations = 0; flagcombinations < (sizeof(flags) / sizeof(flags[0])) * (sizeof(flags) / sizeof(flags[0])); flagcombinations++)
			{
				char endaddress[] = { '0', '0' };
				// Look for flags
				if (instSteps[step].find("|") != std::string::npos)
				{
					vector<string> inststepFlags = explode(explode(instSteps[step], '|')[1], ',');
					for (int flag = 0; flag < inststepFlags.size(); flag++) // Iterate through all flags in step
					{
						for (int checkflag = 0; checkflag < (sizeof(flags) / sizeof(flags[0])); checkflag++) // What is the index of the flag
						{
							if (inststepFlags[flag] == flags[checkflag])
								endaddress[checkflag] = '1';
						}
					}
				}
				string tmpFlagCombos = DecToBinFilled(flagcombinations, 2);
				char* newendaddress = (char*)tmpFlagCombos.c_str();

				bool doesntmatch = false;
				for (int i = 0; i < (sizeof(endaddress) / sizeof(endaddress[0])); i++)
				{
					if (endaddress[i] == '1')
					{
						if (newendaddress[i] != '1')
							doesntmatch = true;
					}
				}
				if (doesntmatch)
					continue;

				cout << ("\t& " + startaddress + " " + midaddress + " " + charToString(newendaddress) + "  =  " + BinToHexFilled(stepComputedInstruction, 4) + "\n");
				output[BinToDec(startaddress + midaddress + charToString(newendaddress))] = BinToHexFilled(stepComputedInstruction, 5);
			}
		}

		//cout<<Line();
	}

	// Do actual processing
	for (int ins = 1; ins < (sizeof(instructioncodes) / sizeof(instructioncodes[0])); ins++) // Iterate through all definitions of instructions
	{
		int correctedIndex = instIndexes[ins];

		cout << (instructioncodes[correctedIndex] + "\n");

		string startaddress = DecToBinFilled(correctedIndex, 4);

		vector<string> instSteps = explode(instructioncodes[correctedIndex], '&');
		for (int step = 0; step < instSteps.size(); step++) // Iterate through every step
		{
			int actualStep = stoi(explode(instSteps[step], '=')[0]);
			string stepContents = explode(explode(instSteps[step], '=')[1], '|')[0];

			string midaddress = DecToBinFilled(actualStep, 4);

			string stepComputedInstruction = "";
			for (int mins = 0; mins < (sizeof(microinstructions) / sizeof(microinstructions[0])); mins++)
			{
				if (stepContents.find(microinstructions[mins]) != std::string::npos)
					stepComputedInstruction += "1";
				else
					stepComputedInstruction += "0";
			}

			// Compute flags combinations
			for (int flagcombinations = 0; flagcombinations < (sizeof(flags) / sizeof(flags[0])) * (sizeof(flags) / sizeof(flags[0])); flagcombinations++)
			{
				char endaddress[] = { '0', '0' };
				int stepLocked[] = { 0, 0 };
				// If flags are specified in current step layer, set them to what is specified and lock that bit
				if (instSteps[step].find("|") != std::string::npos)
				{
					vector<string> inststepFlags = explode(explode(instSteps[step], '|')[1], ',');
					for (int flag = 0; flag < inststepFlags.size(); flag++) // Iterate through all flags in step
					{
						for (int checkflag = 0; checkflag < (sizeof(flags) / sizeof(flags[0])); checkflag++) // What is the index of the flag
						{
							if (inststepFlags[flag].find(flags[checkflag]) != std::string::npos)
							{
								if (inststepFlags[flag][0] == '!')
									endaddress[checkflag] = '0';
								else
									endaddress[checkflag] = '1';
								stepLocked[checkflag] = 1;
							}
						}
					}
				}
				string tmpFlagCombos = DecToBinFilled(flagcombinations, 2);
				char* newendaddress = (char*)tmpFlagCombos.c_str();

				// Make sure the current combination doesn't change the locked bits, otherwise go to next step
				bool doesntmatch = false;
				for (int i = 0; i < (sizeof(endaddress) / sizeof(endaddress[0])); i++)
				{
					if (stepLocked[i] == 1)
					{
						if (newendaddress[i] != endaddress[i])
							doesntmatch = true;
					}
				}
				if (doesntmatch)
					continue;

				cout << ("\t& " + startaddress + " " + midaddress + " " + charToString(newendaddress) + "  =  " + BinToHexFilled(stepComputedInstruction, 5));
				cout << endl;
				output[BinToDec(startaddress + midaddress + charToString(newendaddress))] = BinToHexFilled(stepComputedInstruction, 5);
			}
		}

		//cout<<Line();
	}


	string processedOutput = "";

	// Print the output
	cout << ("\nv3.0 hex words addressed\n");
	processedOutput += "\nv3.0 hex words addressed\n";
	cout << ("000: ");
	processedOutput += "000: ";
	for (int outindex = 0; outindex < output.size(); outindex++)
	{
		if (outindex % 8 == 0 && outindex != 0)
		{
			string locationTmp = DecToHexFilled(outindex, 3);
			transform(locationTmp.begin(), locationTmp.end(), locationTmp.begin(), ::toupper);
			cout << ("\n" + locationTmp + ": ");
			processedOutput += "\n" + DecToHexFilled(outindex, 3) + ": ";
		}
		cout << (output[outindex] + " ");
		processedOutput += output[outindex] + " ";

		string ttmp = output[outindex];
		transform(ttmp.begin(), ttmp.end(), ttmp.begin(), ::toupper);

		string binversion = HexToBin(ttmp, 17);
		for (int i = 0; i < binversion.size(); i++)
		{
			microinstructionData[outindex].push_back(binversion[i] == '1');
		}
	}
}
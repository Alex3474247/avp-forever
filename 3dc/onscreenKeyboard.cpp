
#include "onscreenKeyboard.h"
#include "logString.h"
#include "d3_func.h"
#include "stdio.h"
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>

extern "C" {
extern void D3D_DrawRectangle(int x, int y, int w, int h, int alpha);
extern unsigned char KeyboardInput[];
};

extern "C" 
{
	#include "avp_menugfx.hpp"
	#include "platform.h"
}

#define ONE_FIXED	65536

static int Osk_GetCurrentLocation();
std::string Osk_GetKeyLabel(int buttonIndex);

static int currentRow = 0;
static int currentColumn = 0;

static int currentValue = 0;

static int osk_x = 320;
static int osk_y = 280;
static int oskWidth = 400;
static int oskHeight = 200;

static const int keyWidth = 30;
static const int keyHeight = 30;

static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;

struct ButtonStruct
{
	int numWidthBlocks;
	int height;
	int width;
	int positionOffset;
	int stringId;
	bool isBlank;
};
std::vector<ButtonStruct> keyVector;

// we store our strings seperately and index using the stringId (to avoid duplicates)
std::vector<std::string> stringVector;

const int numVerticalKeys = 5;
const int numHorizontalKeys = 12;
const int numKeys = numVerticalKeys * numHorizontalKeys;

static bool is_active = false;
static bool is_inited = false;

static int buttonId = 0;

static char buf[100];

template <class T> void Osk_AddKey(T buttonLabel, int numWidthBlocks)
{
	ButtonStruct newButton = {0};

	std::stringstream stringStream;

	stringStream << buttonLabel;

	newButton.stringId = buttonId;
	newButton.numWidthBlocks = numWidthBlocks;
	newButton.width = (numWidthBlocks * keyWidth) + space_between_keys * (numWidthBlocks - 1);
	newButton.height = keyHeight;

	// store the string in its own vector
	stringVector.push_back(stringStream.str());

	if (stringStream.str() == "BLANK")
	{
		newButton.isBlank = true;
	}

	int positionOffset = 0;

	int blockCount = numWidthBlocks;

	// for each block in a button, add it to the key vector
	while (blockCount)
	{
		newButton.positionOffset = positionOffset;
		keyVector.push_back(newButton);

		positionOffset++;
		blockCount--;
	}

	buttonId++;
}

void Osk_Init()
{
	currentRow = 0;
	currentColumn = 0;

	// do top row of numbers
	for (int i = 9; i >= 0; i--)
	{
		Osk_AddKey(i, 1);
	}

	Osk_AddKey("Done", 2);

	// second row..
	for (char letter = 'a'; letter <= 'j'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Shift", 2);

	// third row..
	for (char letter = 'k'; letter <= 't'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Caps Lock", 2);

	// fourth row..
	for (char letter = 'u'; letter <= 'z'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Backspace", 4);
	Osk_AddKey("Symbols",	2);

	// fifth row
	Osk_AddKey("Space", 6);
	Osk_AddKey("<",		2);
	Osk_AddKey(">",		2);
	Osk_AddKey("BLANK", 2);

//	sprintf(buf, "number of keys added to osk: %d\n", keyVector.size());
//	OutputDebugString(buf);

	assert (keyVector.size() == 60);

	currentValue = 0;

	oskWidth = (keyWidth * numHorizontalKeys) + (space_between_keys * numHorizontalKeys) + (indent_space * 2);
	oskHeight = (keyHeight * numVerticalKeys) + (space_between_keys * numVerticalKeys) + (indent_space * 2);

	is_inited = true;
}

void Osk_Draw()
{
	if (!Osk_IsActive()) 
		return;

	osk_x = (640 - oskWidth) / 2;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d curentChar: %s\n", currentRow, currentColumn, Osk_GetCurrentLocation(), keyVector.at(Osk_GetCurrentLocation()).name.c_str());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, D3DCOLOR_ARGB(120, 80, 160, 120));

	// start off with an indent to space things out nicely
	int pos_x = osk_x + indent_space;
	int pos_y = osk_y + indent_space;

	int innerSquareWidth = keyWidth - outline_border_size * 2;
	int innerSquareHeight = keyHeight - outline_border_size * 2;

	int index = 0;

	for (int y = 0; y < numVerticalKeys; y++)
	{
		pos_x = osk_x + indent_space;

		int widthCount = numHorizontalKeys;

		while (widthCount)
		{
			// only draw keys not marked as blank keys
			if (!keyVector.at(index).isBlank)
			{
				DrawQuad(pos_x, pos_y, keyVector.at(index).width, keyVector.at(index).height, D3DCOLOR_ARGB(200, 255, 255, 255));

				if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 38, 80, 145));
				else
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 128, 128, 128));

				RenderSmallMenuText((char*)Osk_GetKeyLabel(index).c_str(), pos_x + (keyVector.at(index).width / 2)/*(keyVector.at(index).width - outline_border_size * 2 / 2)*/, pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_LEFTJUSTIFIED);
				//RenderMenuText((char*)Osk_GetKeyLabel(index).c_str(), pos_x + (keyVector.at(index).width / 2), pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_LEFTJUSTIFIED);
			}

			pos_x += (keyVector.at(index).width + space_between_keys);
			widthCount -= keyVector.at(index).numWidthBlocks;

			index += keyVector.at(index).numWidthBlocks;
		}
		pos_y += (keyHeight + space_between_keys);
	}
}

bool Osk_IsActive()
{
	return is_active; // sort this later to only appear for text entry on xbox
}

std::string Osk_GetKeyLabel(int buttonIndex)
{
	return stringVector.at(keyVector.at(buttonIndex).stringId);
}

void Osk_Activate()
{
	if (is_inited == false)
		Osk_Init();

	is_active = true;
}

void Osk_Deactivate()
{
	is_active = false;
}

#ifdef _XBOX
extern void AddKeyToQueue(char virtualKeyCode);
#endif

char Osk_HandleKeypress()
{
	std::string buttonLabel = Osk_GetKeyLabel(Osk_GetCurrentLocation());

	char selectedChar;

	if (buttonLabel == "Done")
	{
#ifdef _XBOX
		AddKeyToQueue(KEY_CR);
#endif
		return 0;
	}

	else if (buttonLabel == "Shift")
	{
		return 0;
	}

	else if (buttonLabel == "Caps Lock")
	{
		return 0;
	}

	else if (buttonLabel == "Symbols")
	{
		return 0;
	}

	else if (buttonLabel == "Space")
	{
		return ' ';
	}

	else if (buttonLabel == "Backspace")
	{
#ifdef _XBOX
		AddKeyToQueue(KEY_BACKSPACE);
#endif
		return 0;
	}

	else if (buttonLabel == "<")
	{
		return 0;
	}

	else if (buttonLabel == ">")
	{
		return 0;
	}

	else 
	{
		//return StringToInt(buttonLabel);
		selectedChar = buttonLabel.at(0);
		return selectedChar;
	}

	return 0;
}

static int Osk_GetCurrentLocation()
{
	return currentValue;
}

void Osk_MoveLeft()
{
	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// store some information about current key
	int buttonOffset = keyVector.at(currentPosition).positionOffset;
	int width = keyVector.at(currentPosition).numWidthBlocks;

	// lets do the actual left move
	currentColumn -= buttonOffset + 1;

	// wrap?
	if (currentColumn < 0)
		currentColumn = numHorizontalKeys - 1;

	// where are we now?
	currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		buttonOffset = keyVector.at(currentPosition).positionOffset;

		currentColumn -= buttonOffset + 1;

		// wrap?
		if (currentColumn < 0)
			currentColumn = numHorizontalKeys - 1;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// then align to button left..
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveRight()
{
	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	int buttonOffset = keyVector.at(currentPosition).positionOffset;
	int width = keyVector.at(currentPosition).numWidthBlocks;

	currentColumn += width - buttonOffset;

	// wrap?
	if (currentColumn >= numHorizontalKeys) // add some sort of numColumns?
		currentColumn = 0;

	// where are we now?
	currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		buttonOffset = keyVector.at(currentPosition).positionOffset;

		currentColumn += width - buttonOffset;

		// wrap?
		if (currentColumn >= numHorizontalKeys) // add some sort of numColumns?
			currentColumn = 0;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// then align to button left..
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveUp()
{
	currentRow--;

	// make sure we're not outside the grid
	if (currentRow < 0)
		currentRow = 4;

	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		currentRow--;

		// make sure we're not outside the grid
		if (currentRow < 0)
			currentRow = 4;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// left align button value
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveDown() 
{
	currentRow++;

	// make sure we're not outside the grid
	if (currentRow > 4)
		currentRow = 0;

	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		currentRow++;

		// make sure we're not outside the grid
		if (currentRow > 4)
			currentRow = 0;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// left align button value
	currentValue -= keyVector.at(currentValue).positionOffset;
}
#include "Zuma_Prefix.pch"

#include <SexyAppFramework/ButtonListener.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/DDImage.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/Widget.h>
#include <SexyAppFramework/WidgetManager.h>

#include "CircleButton.h"
#include "CircleCommon.h"
#include "CircleShootApp.h"
#include "LevelParser.h"
#include "HighScoreMgr.h"
#include "ProfileMgr.h"
#include "PracticeScreen.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

bool gUnlocked = false;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PracticeScreen::PracticeScreen()
{
	mNextButton = MakeButton(0, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTNEXTBUTTON, 4);
	mBackButton = MakeButton(1, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTBACKBUTTON, 4);
	mGauntPlayButton = MakeButton(2, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTPLAYBUTTON, 4);
	mMainMenuButton = MakeButton(3, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTMAINMENUBUTTON, 3);
	mPracticeButton = MakeButton(5, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTPRACTICEBUTTON, 3);
	mSurvivalButton = MakeButton(6, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_GAUNTSURVIVALBUTTON, 3);

	mThumbnail = NULL;
	mScoreSet = new std::multiset<HighScore>();
	mGradientImageLocked = CreateGradientImage("LoCKED", 0x85);
	mGradientImageQuestionMark = CreateGradientImage("?", 0xFF);

	mCurrentBoard = GetCircleShootApp()->mProfile->mLastPracticeBoard;
	mIsEndless = GetCircleShootApp()->mProfile->mPracticeEndless;
	mDifficulty = GetCircleShootApp()->mProfile->mPracticeDifficulty;
	mCurrentDoor = 0;
	mHighlightDoor = -1;
	mMaxStage = 0;
	mLastClickCnt = 0;

	IntPoint aDoors[12] = {
		IntPoint(209 * 2 + 320, 179 * 2 + 60),
		IntPoint(232 * 2 + 320, 129 * 2 + 60),
		IntPoint(249 * 2 + 320, 82 * 2 + 60),
		IntPoint(265 * 2 + 320, 28 * 2 + 60),
		IntPoint(320, 60),
		IntPoint(320, 60),
		IntPoint(320, 60),
		IntPoint(320, 60),
		IntPoint(320, 60),
		IntPoint(320, 60),
		IntPoint(320, 60)
	};

	for (int i = 0; i < 4; i++)
	{
		mDoorInfo[i].mImage = (MemoryImage *)Sexy::GetImageById((ResourceId)(i + Sexy::IMAGE_GAUNTDOOR1_ID));
		int aWidth = mDoorInfo[i].mImage->mWidth;
		if (aWidth < -0)
		{
			aWidth += 3;
		}
		mDoorInfo[i].mRect.mHeight = mDoorInfo[i].mImage->mHeight;
		mDoorInfo[i].mRect.mWidth = aWidth >> 2;
		mDoorInfo[i].mRect.mY = aDoors[i].mY;
		mDoorInfo[i].mRect.mX = aDoors[i].mX;
	}

	mLocked = false;
	mGauntletBlinkCount = 0;
	SyncModeButtons();
}

PracticeScreen::~PracticeScreen() {}

void PracticeScreen::Update()
{
	Widget::Update();

	if (mGauntletBlinkCount != 0)
	{
		mGauntletBlinkCount--;
	}

	if ((mUpdateCnt & 3) == 0)
	{
		MarkDirty();
	}
}

void PracticeScreen::Resize(int x, int y, int theWidth, int theHeight)
{
	Widget::Resize(x, y, theWidth, theHeight);

	mBackButton->Resize(x + 177 * 2 + 320, y + 444 * 2 + 60, mBackButton->mWidth, mBackButton->mHeight);
	mGauntPlayButton->Resize(x + 277 * 2 + 320, y + 444 * 2 + 60, mGauntPlayButton->mWidth, mGauntPlayButton->mHeight);
	mNextButton->Resize(x + 390 * 2 + 320, y + 444 * 2 + 60, mNextButton->mWidth, mNextButton->mHeight);
	mMainMenuButton->Resize(x + 10 * 2 + 320, y + 422 * 2 + 60, mMainMenuButton->mWidth, mMainMenuButton->mHeight);
	mSurvivalButton->Resize(x + 506 * 2 + 320, y + 413 * 2 + 60, mSurvivalButton->mWidth, mSurvivalButton->mHeight);
	mPracticeButton->Resize(x + 505 * 2 + 320, y + 446 * 2 + 60, mPracticeButton->mWidth, mPracticeButton->mHeight);
}

void PracticeScreen::AddedToManager(WidgetManager *theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	LoadBoard();

	theWidgetManager->AddWidget(mBackButton);
	theWidgetManager->AddWidget(mNextButton);
	theWidgetManager->AddWidget(mGauntPlayButton);
	theWidgetManager->AddWidget(mMainMenuButton);
	theWidgetManager->AddWidget(mSurvivalButton);
	theWidgetManager->AddWidget(mPracticeButton);
}

void PracticeScreen::RemovedFromManager(WidgetManager *theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	theWidgetManager->RemoveWidget(mBackButton);
	theWidgetManager->RemoveWidget(mNextButton);
	theWidgetManager->RemoveWidget(mGauntPlayButton);
	theWidgetManager->RemoveWidget(mMainMenuButton);
	theWidgetManager->RemoveWidget(mSurvivalButton);
	theWidgetManager->RemoveWidget(mPracticeButton);

	GetCircleShootApp()->mProfile->mLastPracticeBoard = mCurrentBoard;
	GetCircleShootApp()->mProfile->mPracticeEndless = mIsEndless;
	GetCircleShootApp()->mProfile->mPracticeDifficulty = mDifficulty;

	GetCircleShootApp()->SaveProfile();
}

void PracticeScreen::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mCurrentBoard++;
		LoadBoard();
		break;
	case 1:
		mCurrentBoard--;
		LoadBoard();
		break;
	case 2:
		if (mCurrentDoor >= 0)
		{
			GetCircleShootApp()->StartPracticeGame(mStage, 7 * mCurrentDoor, mIsEndless);
		}
		break;
	case 3:
		GetCircleShootApp()->ShowMainMenu();
		break;
	case 4:
		GetCircleShootApp()->DoOptionsDialog();
		break;
	case 5:
		mIsEndless = false;
		SyncModeButtons();
		GetHighscores();
		break;
	case 6:
		mIsEndless = true;
		SyncModeButtons();
		GetHighscores();
		break;
	default:
		break;
	}
}

void PracticeScreen::Draw(Graphics *g)
{
	Widget::Draw(g);

	int aScroll = (mUpdateCnt >> 2) % Sexy::IMAGE_GAUNTSKY->mWidth;
	g->DrawImage(Sexy::IMAGE_GAUNTSKY, aScroll - Sexy::IMAGE_GAUNTSKY->mWidth, 60);
	g->DrawImage(Sexy::IMAGE_GAUNTSKY, aScroll, 60);
	g->DrawImage(Sexy::IMAGE_GAUNTSKY, aScroll + Sexy::IMAGE_GAUNTSKY->mWidth, 60);

	g->DrawImage(mThumbnail, 170 * 2 + 320, 220 * 2 + 60);
	g->DrawImage(Sexy::IMAGE_GAUNTSCREEN, 320, 60);
	g->DrawImage(Sexy::IMAGE_GAUNTTITLE, 320, 60);
	g->DrawImage(Sexy::IMAGE_GAUNTHIGHSCORE, 430 * 2 + 360, 60);

	for (int i = 0; i < 4; i++)
	{
		int aFrame = 1;

		if (i == mCurrentDoor)
		{
			aFrame = 3;
		}
		if (i == mHighlightDoor)
		{
			aFrame = 2;
		}
		if (i == mMaxStage)
		{
			if (((mGauntletBlinkCount >> 4) & 1) != 0)
			{
				aFrame = 2;
			}
		}
		else if (i > mMaxStage)
		{
			aFrame = 0;
		}

		g->DrawImageCel(mDoorInfo[i].mImage, mDoorInfo[i].mRect.mX, mDoorInfo[i].mRect.mY, aFrame);
	}

	g->SetColor(Color(0xFFFFFF));
	g->SetFont(Sexy::FONT_MAIN10OUTLINE);

	std::string aText(mDisplayName);
	aText[0] = toupper(aText[0]);
	g->DrawString(aText,
				  170 * 2 + 320 + (mThumbnail->mWidth - Sexy::FONT_MAIN10OUTLINE->StringWidth(aText)) / 2,
				  427 * 2 + 60);

	g->SetColor(Color(0xFFFF00));
	aText = mIsEndless ? "Survival" : "Practice";
	g->DrawString(aText,
				  150 * 2 + 320 + (mThumbnail->mWidth - Sexy::FONT_MAIN10OUTLINE->StringWidth(aText)),
				  260 * 2 + 60);

	if (mMaxStage > 2)
	{
		int aPosX = 560 * 2 + 320 - Sexy::IMAGE_SUNGLOW->mWidth / 2;
		int aPosY = 220 * 2 + 60 - (Sexy::IMAGE_SUNGLOW->mHeight + Sexy::IMAGE_GAUNTSUNGEM->mHeight) / 2;

		g->SetColorizeImages(true);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);

		int aGlow = mUpdateCnt % 500;
		if (aGlow > 250)
		{
			aGlow = 500 - aGlow;
		}

		g->SetColor(Color(255, 255, 0, 75 * aGlow / 250 + 75));

		if (gSexyAppBase->Is3DAccelerated())
		{
			float aRotation = (mUpdateCnt * SEXY_PI / 180.0f) * 0.3f;
			g->DrawImageRotated(Sexy::IMAGE_SUNGLOW, aPosX, aPosY, aRotation);
		}
		else
		{
			g->DrawImage(Sexy::IMAGE_SUNGLOW, aPosX, aPosY);
		}

		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
		g->SetColorizeImages(false);
	}

	switch (mMaxStage)
	{
	case 1:
		g->DrawImage(Sexy::IMAGE_GAUNTEAGLEGEM, 560 * 2 + 320 - Sexy::IMAGE_GAUNTEAGLEGEM->mWidth / 2, 220 * 2 + 60 - Sexy::IMAGE_GAUNTEAGLEGEM->mHeight);
		break;
	case 2:
		g->DrawImage(Sexy::IMAGE_GAUNTJAGUARGEM, 560 * 2 + 320 - Sexy::IMAGE_GAUNTJAGUARGEM->mWidth / 2, 220 * 2 + 60 - Sexy::IMAGE_GAUNTJAGUARGEM->mHeight);
		break;
	case 3:
		g->DrawImage(Sexy::IMAGE_GAUNTSUNGEM, 560 * 2 + 320 - Sexy::IMAGE_GAUNTSUNGEM->mWidth / 2, 220 * 2 + 60 - Sexy::IMAGE_GAUNTSUNGEM->mHeight);
		break;
	default:
		break;
	}

	if (mScoreSet != NULL)
	{
		Graphics sg(*g);
		sg.SetColor(Color(0x5B010A));
		sg.SetFont(Sexy::FONT_MAIN10OUTLINE);

		HighScoreSet::iterator anItr = mScoreSet->begin();
		for (int i = 0; i != 3 && anItr != mScoreSet->end(); anItr++, i++)
		{
			g->DrawString(
				Sexy::StrFormat("%d", anItr->mScore),
				575 * 2 + 320,
				70 * 2 + 60 + Sexy::FONT_MAIN10OUTLINE->GetLineSpacing() * i);
		}

		sg.ClipRect(320, 60, 567, 480);

		anItr = mScoreSet->begin();
		for (int i = 0; i != 3 && anItr != mScoreSet->end(); anItr++, i++)
		{
			g->DrawString(
				anItr->mName,
				445 * 2 + 320,
				70 * 2 + 60 + Sexy::FONT_MAIN10OUTLINE->GetLineSpacing() * i);
		}
	}

	g->SetColor(Color(0x5B010A));
	g->SetFont(Sexy::FONT_MAIN8OUTLINE);

	const char *aDescription;
	if (mLocked)
	{
		aDescription = "YOU MUST FIRST UNLOCK THIS LEVEL IN ADVENTURE MODE.";
	}
	else if (mCurrentBoard != 0)
	{
		aDescription = "HOW LONG CAN YOU SURVIVE AN ENDLESS STREAM OF ZUMAIC INSANITY?";
		if (!this->mIsEndless)
			aDescription = "PRACTICE A SINGLE BOARD UNTIL MASTERED.  CAN YOU BECOME A SUN GOD?";
	}
	else
	{
		aDescription = "PLAY A RANDOM LEVEL THAT YOU HAVE UNLOCKED IN ADVENTURE MODE.";
		if (!this->mIsEndless)
			aDescription = "PLAY THROUGH ALL THE LEVELS YOU HAVE UNLOCKED IN ADVENTURE MODE IN RANDOM ORDER!";
	}

	Widget::WriteWordWrapped(g, Rect(20, 60, 150, 100), aDescription, 14, -1);
	g->DrawImage(IMAGE_MM_GAMEBORDER, 0, 0);
}


void PracticeScreen::KeyChar(char theChar)
{
	Widget::KeyChar(theChar);
#ifndef RELEASEFINAL
	char c = toupper(theChar);
	if (c == 'U') //Recover this key
	{
		if (gUnlocked)
		{
			gUnlocked = false;
		}
		else
		{
			gUnlocked = true;
		}
	}
#endif
}

void PracticeScreen::MouseLeave()
{
	Widget::MouseLeave();
	gSexyAppBase->SetCursor(0);
}

void PracticeScreen::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);

	int aDoor = GetDoorAt(x, y);
	if (aDoor != mHighlightDoor)
	{
		gSexyAppBase->SetCursor(aDoor >= 0 ? CURSOR_HAND : CURSOR_POINTER);
		mHighlightDoor = aDoor;
		MarkDirty();
	}
}

void PracticeScreen::MouseDown(int x, int y, int theButton)
{
	Widget::MouseDown(x, y, theButton);

	int aDoor = GetDoorAt(x, y);
	if (aDoor >= 0)
	{
		if (aDoor == mCurrentDoor)
		{
			ButtonDepress(2);
		}
		else
		{
			mCurrentDoor = aDoor;
			mDifficulty = aDoor;
			MarkDirty();
		}
	}

	mLastClickCnt = mUpdateCnt;
}

void PracticeScreen::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
}

void PracticeScreen::SyncModeButtons() {
	CircleButton* aButton1 = mSurvivalButton;
	CircleButton* aButton2 = mPracticeButton;

	if (!mIsEndless) {
		aButton1 = mPracticeButton;
		aButton2 = mSurvivalButton;
	}

	aButton1->mInverted = true;
	aButton1->SetDisabled(true);

	aButton2->mInverted = false;
	aButton2->SetDisabled(false);
}

MemoryImage *PracticeScreen::CreateGradientImage(std::string const &theName, int theAlpha)
{
	MemoryImage *anImage = new MemoryImage(gSexyAppBase);
	anImage->Create(640, 480);

	Graphics g(anImage);
	int aHalfHeight = anImage->mHeight / 2;
	for (int i = 0; i < anImage->mHeight; i++)
	{
		int v5 = aHalfHeight - i;
		if (v5 < 0)
			v5 = -v5;

		int aRed = 255 - 255 * v5 / aHalfHeight;
		if (aRed >= 0)
		{
			if (255 - 255 * v5 / aHalfHeight >= 256)
			{
				aRed = 255;
			}
		}
		else
		{
			aRed = 0;
		}

		g.SetColor(Color(aRed, 0, 0, theAlpha));
		g.FillRect(0, i, anImage->mWidth, 1);
	}

	g.SetFont(Sexy::FONT_HUGE);
	g.SetColor(Color(0xFFFFFF));
	g.DrawString(
		theName,
		(anImage->mWidth - Sexy::FONT_HUGE->StringWidth(theName)) / 2,
		anImage->mHeight / 2 + 25);

	return anImage;
}

int PracticeScreen::GetDoorAt(int x, int y)
{
	for (int i = 0; i < 4; i++)
	{
		if (i > mMaxStage)
			break;

		if (mDoorInfo[i].mRect.Contains(x, y))
		{
			Sexy::MemoryImage *anImage = mDoorInfo[i].mImage;

			int aX = x - mDoorInfo[i].mRect.mX;
			int aY = y - mDoorInfo[i].mRect.mY;
			int aWidth = anImage->GetWidth();

			if ((anImage->GetBits()[aX + aY * aWidth] & 0xFF000000) != 0)
			{
				return i;
			}
		}
	}

	return -1;
}

MemoryImage *PracticeScreen::GetThumbnail(std::string const &theName)
{
	// TODO:
	// there's also a base folder argument passed into arg 2 of GetImage in some versions
	// the SexyApp version we're using doesn't support that and we don't support custom save folders yet.
	MemoryImage *anImage = gSexyAppBase->GetImage("levels/cached_thumbnails/" + theName);
	if (anImage != NULL)
	{
		return anImage;
	}

	anImage = gSexyAppBase->GetImage("levels/perm_thumbnails/" + theName);

	return anImage;
}

void PracticeScreen::GetHighscores()
{
}

void PracticeScreen::LoadBoard()
{
	delete mThumbnail;
	mThumbnail = NULL;

	UserProfile *aProfile = GetCircleShootApp()->mProfile;
	if (aProfile->mNeedGauntletBlink)
	{
		aProfile->mNeedGauntletBlink = false;
		mGauntletBlinkCount = 160;
	}
	else
	{
		mGauntletBlinkCount = 0;
	}

	LevelParser *aLevelParser = GetCircleShootApp()->mLevelParser;
	int aBoards = aLevelParser->mBoardProgression.size();
	int aMaxBoards = aBoards + 1;
	if (GetCircleShootApp()->mProfile->mMaxStage < 12)
	{
		aMaxBoards = aBoards;
	}

	if (aMaxBoards <= mCurrentBoard)
	{
		mCurrentBoard = 0;
	}

	if (mCurrentBoard < 0)
	{
		mCurrentBoard = aMaxBoards - 1;
	}

	aLevelParser->GetPracticeBoard(mCurrentBoard, mStage, mDisplayName);

	UserProfile::BoardMap::iterator anItr = aProfile->mMaxBoardLevel.find(mStage);
	bool aUnlocked;

	if (anItr == aProfile->mMaxBoardLevel.end())
	{
		mMaxStage = -1;
		aUnlocked = false;
	}
	else
	{
		mMaxStage = anItr->second / 7;
		if (mMaxStage > 3)

			mMaxStage = 3;

		aUnlocked = true;
	}

	mCurrentDoor = mDifficulty >= 0 ? mDifficulty : 0;
	if (mCurrentDoor > mMaxStage)
	{
		mCurrentDoor = mMaxStage;
	}

	mHighlightDoor = -1;

	if (gUnlocked)
	{
		mMaxStage = 3;
		aUnlocked = true;
	}

	mGauntPlayButton->SetVisible(aUnlocked);

	mThumbnail = GetThumbnail(mStage);
	if (mThumbnail == NULL)
	{
		mThumbnail = new MemoryImage(gSexyAppBase);
		mThumbnail->Create(320 * 2, 240 * 2);
		Graphics g(mThumbnail);
		g.SetColor(Color(0));
		g.FillRect(0, 0, 640, 480);
	}

	mLocked = !aUnlocked;

	Graphics g(mThumbnail);
	if (mLocked)
	{
		g.DrawImage(mGradientImageLocked, 0, 0);
	}
	else if (mCurrentBoard == 0)
	{
		g.DrawImage(mGradientImageQuestionMark, 0, 0);
	}

	GetHighscores();
	MarkDirty();
}
#include "Zuma_Prefix.pch"

#include <SexyAppFramework/ButtonListener.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/DDImage.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/ResourceManager.h>
#include <SexyAppFramework/Widget.h>
#include <SexyAppFramework/WidgetManager.h>

#include "CircleButton.h"
#include "CircleCommon.h"
#include "CircleShootApp.h"
#include "LevelParser.h"
#include "HighScoreMgr.h"
#include "ProfileMgr.h"
#include "AdventureScreen.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AdventureScreen::AdventureScreen()
{
	mMainMenuButton = MakeButton(0, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_ADVMAINMENUBUTTON, 3);
	mPlayButton = MakeButton(1, this, "", CircleButton::CB_ClickSound, Sexy::IMAGE_ADVPLAYBUTTON, 3);

	Point aDoorPoints[12] = {
		Point(226 * 2 + 320, 300 * 2 + 60),
		Point(296 * 2 + 320, 270 * 2 + 60),
		Point(366 * 2 + 320, 300 * 2 + 60),
		Point(320, 164 * 2 + 60),
		Point(69 * 2 + 320, 159 * 2 + 60),
		Point(120 * 2 + 320, 155 * 2 + 60),
		Point(452 * 2 + 320, 154 * 2 + 60),
		Point(509 * 2 + 320, 154 * 2 + 60),
		Point(568 * 2 + 320, 154 * 2 + 60),
		Point(220 * 2 + 320, 35 * 2 + 60),
		Point(291 * 2 + 320, 35 * 2 + 60),
		Point(369 * 2 + 320, 35 * 2 + 60)};

	Point aDoorPointsBounds[12] = {
		Point(256 * 2 + 320, 361),
		Point(327 * 2 + 320, 364),
		Point(398 * 2 + 320, 360),
		Point(56 * 2 + 320, 218),
		Point(116 * 2 + 320, 201),
		Point(167 * 2 + 320, 188),
		Point(462 * 2 + 320, 188),
		Point(521 * 2 + 320, 192),
		Point(584 * 2 + 320, 196),
		Point(248 * 2 + 320, 86),
		Point(324 * 2 + 320, 86),
		Point(408 * 2 + 320, 85)};

	mTemples = 0;
	mStagger = 0;
	mMaxStage = GetCircleShootApp()->mProfile->mMaxStage;
	if (mMaxStage < 0)
		mMaxStage = 0;
	if (mMaxStage > 11)
		mMaxStage = 11;

	for (int i = 0; i < 12; i++)
	{
		Image *anImage = GetImageById(ResourceId(i + Sexy::IMAGE_ADVDOOR1A_ID));
		int aWidth = anImage->mWidth;
		int aHeight = anImage->mHeight;

		mDoorInfo[i].mImage = (MemoryImage *)anImage;
		mDoorInfo[i].mRect = Rect(aDoorPoints[i].mX, aDoorPoints[i].mY, aWidth / 4, aHeight);
		mDoorInfo[i].mUnknown = Point(aDoorPointsBounds[i].mX, aDoorPointsBounds[i].mY);
	}

	for (int i = 0; i < 4; i++)
		mTempleXOffsets[i] = 0;

	mCurrentDoor = mMaxStage;
	mUpdateCnt = 0;
	mHighlightDoor = -1;
	mStartNextTempleOnReveal = false;
	mScoreSet = new HighScoreSet();
	*mScoreSet = GetCircleShootApp()->mHighScoreMgr->GetHighScores("adventure");
}

AdventureScreen::~AdventureScreen()
{
	delete mMainMenuButton;
	delete mPlayButton;

	delete mScoreSet;

	gSexyAppBase->mResourceManager->DeleteExtraImageBuffers("AdventureScreen");
}

void AdventureScreen::Update()
{
	Widget::Update();

	if (mTemples)
	{
		mStagger++;

		if (mTemples == 4)
		{
			if (mStagger == 1)
			{
				gSexyAppBase->PlaySample(SOUND_EARTHQUAKE);
			}

			if (mStagger >= 0 && (mUpdateCnt % 4) == 0)
			{
				int a = 10 - mStagger / 30;
				int b = mStagger / 30 - 10;

				for (int i = 0; i < 4; i++)
				{
					int c = mTempleXOffsets[i];
					int d = c + Sexy::Rand() % 3 - 1;
					if (a < d)
					{
						d = a;
					}
					mTempleXOffsets[i] = d;
					if (b > d)
					{
						mTempleXOffsets[i] = b;
					}
				}
			}

			if (mStagger > 300)
			{
				for (int i = 0; i < 4; i++)
				{
					mTempleXOffsets[i] = 0;
				}
				mTemples = 0;
			}
		}
		else if (mStagger > 100)
		{
			mTemples = 0;
		}

		if (mStartNextTempleOnReveal && mTemples == 0)
		{
			mStartNextTempleOnReveal = false;
			GetCircleShootApp()->DoNextTempleDialog();
		}

		MarkDirty();
	}

	if (mUpdateCnt % 4 == 0)
	{
		MarkDirty();
	}
}

void AdventureScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);

	if (mWidgetManager && mWidgetManager->mMouseIn)
	{
		mHighlightDoor = GetDoorAt(mWidgetManager->mLastMouseX - theX, mWidgetManager->mLastMouseY - theY);
	}

	mMainMenuButton->Resize(theX * 2 + 320, theY + 438 * 2 + 60, mMainMenuButton->mWidth, mMainMenuButton->mHeight);
	mPlayButton->Resize(theX + 542 * 2 + 320, theY + 441 * 2 + 60, mPlayButton->mWidth, mPlayButton->mHeight);
}

void AdventureScreen::AddedToManager(WidgetManager *theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	theWidgetManager->AddWidget(mMainMenuButton);
	theWidgetManager->AddWidget(mPlayButton);
}

void AdventureScreen::RemovedFromManager(WidgetManager *theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	theWidgetManager->RemoveWidget(mMainMenuButton);
	theWidgetManager->RemoveWidget(mPlayButton);
}

void AdventureScreen::ButtonDepress(int theId)
{
	if (mStartNextTempleOnReveal)
		return;

	if (theId == 0)
	{
		GetCircleShootApp()->ShowMainMenu();
	}
	else if (theId == 1 && mCurrentDoor >= 0)
	{
		GetCircleShootApp()->StartAdventureGame(mCurrentDoor);
	}
}

void AdventureScreen::Draw(Graphics *g)
{
	Widget::Draw(g);

	int aSkyScroll = (mUpdateCnt / 4) % IMAGE_ADVSKY->mWidth;
	g->DrawImage(Sexy::IMAGE_ADVSKY, aSkyScroll - IMAGE_ADVSKY->mWidth, 60);
	g->DrawImage(Sexy::IMAGE_ADVSKY, aSkyScroll, 60);
	g->DrawImage(Sexy::IMAGE_ADVSKY, aSkyScroll + IMAGE_ADVSKY->mWidth, 60);

	g->DrawImage(Sexy::IMAGE_ADVBACK, 320, 60);
	g->DrawImage(Sexy::IMAGE_ADVTITLE, 320, 60);
	g->DrawImage(Sexy::IMAGE_ADVHIGHSCORE, 456 * 2 + 320, 0 + 60);

	DrawTemple4(g);
	g->Translate(mTempleXOffsets[2], 0);
	DrawTemple3(g);
	g->Translate(-mTempleXOffsets[2], 0);
	g->Translate(mTempleXOffsets[1], 0);
	DrawTemple2(g);
	g->Translate(-mTempleXOffsets[1], 0);
	g->Translate(mTempleXOffsets[0], 0);
	DrawTemple1(g);
	g->DrawImage(Sexy::IMAGE_GAMEBORDER, 0, 0);
	const char *aTempleNames[4] = {
		"TEMPLE OF ZUKULKAN",
		"QUETZAL QUATL",
		"POPO POYOLLI",
		"SECRET SHRINE OF ZUMA"};

	const char *aStageNames[12] = {
		"DO YOU DARE ENTER THE ANCIENT RUINS?",
		"THE 2ND LEVEL OF THE TEMPLE AWAITS.",
		"WILL YOU DESCEND TO THE THIRD LEVEL?",
		"YOU HAVE UNCOVERED A HIDDEN DOOR!",
		"THE GOLDEN SECRETS OF ZUMA AWAIT YOU.",
		"DO YOU DARE ENTER THE LAST CHAMBER?",
		"ANOTHER LOST TEMPLE...",
		"CAN YOU FULFILL THE PROPHECY?",
		"THE LAST STAGE (?)",
		"A RUINED TEMPLE RISES FROM THE EARTH.",
		"THE FINAL SECRET OF ZUMA BECKONS...",
		"YOU ENTER THE RUINS ONE LAST TIME..."};

	if (mCurrentDoor >= 0)
	{
		g->DrawImage(Sexy::IMAGE_ADVSTAGE, 289 * 2 + 320, 382 * 2 + 60);

		g->SetColor(Color(0xFBF1B0));
		g->SetFont(Sexy::FONT_DIALOG);
		g->DrawString(Sexy::StrFormat("%d", mCurrentDoor + 1), 344 * 2 + 320, 396 * 2 + 60);

		g->SetFont(Sexy::FONT_BROWNTITLE);
		g->SetColor(Color(0xFFFFFF));
		int aTempleTextWidth = Sexy::FONT_BROWNTITLE->StringWidth(aTempleNames[mCurrentDoor / 3]);
		g->DrawString(aTempleNames[mCurrentDoor / 3], 326 * 2 + 320 - aTempleTextWidth / 2, 422 * 2 + 60);

		g->SetFont(Sexy::FONT_MAIN10OUTLINE2);
		std::string aStageName = aStageNames[mCurrentDoor];
		int aStageTextWidth = Sexy::FONT_MAIN10OUTLINE2->StringWidth(aStageName);

		g->SetColor(Color(0xFFEA01));
		g->DrawString(aStageName, 326 * 2 + 320 - aStageTextWidth / 2, 456 * 2 + 60);
	}

	g->Translate(-mTempleXOffsets[0], 0);

	if (mScoreSet != NULL)
	{
		Graphics aClipG(*g);
		aClipG.SetColor(Color(0x28534C));
		aClipG.SetFont(Sexy::FONT_MAIN8OUTLINE);

		HighScoreSet::iterator aHighScoreItr = mScoreSet->begin();
		for (int aScores = 0;
			 aHighScoreItr != mScoreSet->end() && aScores != 3;
			 aScores++, aHighScoreItr++)
		{
			int aY = aScores * Sexy::FONT_MAIN8OUTLINE->GetLineSpacing() + 70;
			aClipG.DrawString(Sexy::StrFormat("%d", aHighScoreItr->mScore), 575 * 2 + 320, aY * 2 + 60);
		}

		aClipG.ClipRect(320, 60, 567, 480);

		aHighScoreItr = mScoreSet->begin();
		for (int aScores = 0;
			 aHighScoreItr != mScoreSet->end() && aScores != 3;
			 aScores++, aHighScoreItr++)
		{
			int aY = aScores * Sexy::FONT_MAIN8OUTLINE->GetLineSpacing() + 70;
			aClipG.DrawString(aHighScoreItr->mName, 470 * 2 + 320, aY * 2 + 60);
		}
	}

	Rect aStageDescRect(20 * 2 + 320, 60 * 2 + 60, 150, 100);
	g->SetFont(Sexy::FONT_MAIN8OUTLINE);
	g->SetColor(Color(0x28534C));
	std::string aStageDesc = "Uncover the secret temples of Zuma!";

	if (mHighlightDoor != -1 && mHighlightDoor != mCurrentDoor)
	{
		aStageDesc = "Click on a doorway to select a different stage.";
	}

	WriteWordWrapped(g, aStageDescRect, aStageDesc, 14, -1);
}

void AdventureScreen::KeyChar(char theChar) //Huh what cheats would go here
{
#ifndef RELEASEFINAL //How about these
	char c = toupper(theChar);
	if (c == '+') //Increase current door
	{
		if (mCurrentDoor != 11) //Anti-crash
			mCurrentDoor++;
	}
	else if (c == '-') //Decrease current door
	{
		mCurrentDoor--;
	}
	else if (c == 'm') //Increase max stage
	{
		mMaxStage++;
	}
	else if (c == 't') //Force temple open
	{
		mStartNextTempleOnReveal ^= true;
	}
	else if (c == 'l') //Increase temple
	{
		if (mTemples != 4)
			mTemples++;
	}
	else if (c == 's') //Decrease temple
	{
		mTemples--;
	}
	/*else if (c == 'h') //Add cheated highscore
	{
		//mScoreSet //todo
	}*/
	else if (c == 'r') //Reset highscores
	{
		mScoreSet = NULL;
	}
	else if (c == 'u')
	{
		RevealTemple(75, mMaxStage / 3 + 1);
		SetStartNextTempleOnRevel(true);
	}
#endif
}

void AdventureScreen::MouseLeave()
{
	Widget::MouseLeave();
	gSexyAppBase->SetCursor(Sexy::CURSOR_POINTER);
}

void AdventureScreen::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);

	if (mStartNextTempleOnReveal)
		return;

	int aDoorAt = GetDoorAt(x, y);
	if (aDoorAt != mHighlightDoor)
	{
		gSexyAppBase->SetCursor(aDoorAt >= 0 ? Sexy::CURSOR_HAND : Sexy::CURSOR_POINTER);
		mHighlightDoor = aDoorAt;
		MarkDirty();
	}
}

void AdventureScreen::MouseDown(int x, int y, int theButton)
{
	Widget::MouseDown(x, y, theButton);

	if (mStartNextTempleOnReveal)
		return;

	int aDoor = GetDoorAt(x, y);
	if (aDoor >= 0)
	{
		if (aDoor == mCurrentDoor)
		{
			ButtonDepress(1);
		}
		else
		{
			mCurrentDoor = aDoor;
			MarkDirty();
		}
	}

	mLastClickCnt = mUpdateCnt;
}

void AdventureScreen::MouseUp(int x, int y)
{
	Widget::MouseUp(x, y);
}

void AdventureScreen::RevealTemple(int theStagger, int theTemple)
{
	mTemples = theTemple;
	mStagger = -theStagger;

	for (int i = 0; i < 4; i++)
	{
		mTempleXOffsets[i] = 0;
	}
}

void AdventureScreen::SetStartNextTempleOnRevel(bool start)
{
	mStartNextTempleOnReveal = start;
	MouseMove(mWidgetManager->mLastMouseX - mX, mWidgetManager->mLastMouseY - mY);
	MarkDirty();
}

void AdventureScreen::DrawDoors(Graphics *g, int first, int last)
{
	for (int i = first; i < last; i++)
	{
		int aCell = 0;
		if (i == mCurrentDoor)
		{
			aCell = 3;
		}
		else if (i == mHighlightDoor)
		{
			aCell = 2;
		}
		else if (i <= mMaxStage)
		{
			aCell = 1;
		}

		g->DrawImageCel(mDoorInfo[i].mImage, mDoorInfo[i].mRect.mX, mDoorInfo[i].mRect.mY, aCell);
	}
}

void AdventureScreen::DrawTemple1(Graphics *g)
{
	g->DrawImage(Sexy::IMAGE_ADVTEMPLE1, 320, 188 * 2 + 60);
	DrawDoors(g, 0, 3);
}

void AdventureScreen::DrawTemple2(Graphics *g)
{
	if (mTemples == 2)
	{
		int aColor = 255 * mStagger / 100;
		if (aColor < 0)
			aColor = 0;

		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 255 - aColor));
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE2V, 320, 112 * 2 + 60);
		g->SetColor(Color(255, 255, 255, aColor));
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE2, 320, 112 * 2 + 60);
		DrawDoors(g, 3, 6);
		g->SetColorizeImages(false);
	}
	else if (mMaxStage < 3)
	{
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE2V, 320, 112 * 2 + 60);
	}
	else
	{
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE2, 320, 112 * 2 + 60);
		DrawDoors(g, 3, 6);
	}
}

void AdventureScreen::DrawTemple3(Graphics *g)
{
	if (mTemples == 3)
	{
		int aColor = 255 * mStagger / 100;
		if (aColor < 0)
			aColor = 0;

		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255, 255, 255 - aColor));
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE3V, 395 * 2 + 320, 125 * 2 + 60);
		g->SetColor(Color(255, 255, 255, aColor));
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE3, 395 * 2 + 320, 125 * 2 + 60);
		DrawDoors(g, 6, 9);
		g->SetColorizeImages(false);
	}
	else if (mMaxStage < 6)
	{
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE3V, 395 * 2 + 320, 125 * 2 + 60);
	}
	else
	{
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE3, 395 * 2 + 320, 125 * 2 + 60);
		DrawDoors(g, 6, 9);
	}
}

void AdventureScreen::DrawTemple4(Graphics *g)
{
	if (mTemples == 4)
	{
		if (mStagger >= 0)
		{
			int aY = 300 - mStagger;
			int aY2 = aY * aY / 300;
			g->Translate(mTempleXOffsets[3], aY2);
			g->DrawImage(Sexy::IMAGE_ADVTEMPLE4, 174 * 2 + 320, 1 * 2 + 60);
			DrawDoors(g, 9, 12);
			g->Translate(-mTempleXOffsets[3], -aY2 * 2 + 60);
		}

		g->DrawImage(Sexy::IMAGE_ADVMIDDLE, 320, 72 * 2 + 60);
	}
	else if (mMaxStage >= 9)
	{
		g->DrawImage(Sexy::IMAGE_ADVTEMPLE4, 174 * 2 + 320, 1 * 2 + 60);
		DrawDoors(g, 9, 12);
		g->DrawImage(Sexy::IMAGE_ADVMIDDLE, 320, 72 * 2 + 60);
	}
}

int AdventureScreen::GetDoorAt(int x, int y)
{
	for (int i = 0; i < 12; i++)
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
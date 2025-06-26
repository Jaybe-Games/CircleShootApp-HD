#include "Zuma_Prefix.pch"

#include <SexyAppFramework/DDInterface.h>
#include <SexyAppFramework/D3DInterface.h>
#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/SexyMatrix.h>

#include "Bullet.h"
#include "Gun.h"
#include "Res.h"
#include "DataSync.h"

#include <math.h>

using namespace Sexy;

Sexy::MemoryImage *gShooterTop = 0;

Gun::Gun()
{
    mCenterX = 1920 / 2;
    mCenterY = 1080 / 2;
    mWidth = Sexy::IMAGE_SHOOTER_BOTTOM->GetWidth();
    mHeight = Sexy::IMAGE_SHOOTER_BOTTOM->GetHeight();

    if (!gShooterTop)
        gShooterTop = Sexy::CutoutImageFromAlpha(
            reinterpret_cast<MemoryImage *>(Sexy::IMAGE_SHOOTER_BOTTOM),
            reinterpret_cast<MemoryImage *>(Sexy::IMAGE_SHOOTER_TOP),
            63, 25);

    mAngle = 0.0f;
    mRecoilCount = 0;
    mBlinkCount = 0;
    mWink = 0;
    mFireVel = 6.0f;
    mState = GunState_Normal;
    mBullet = NULL;
    mNextBullet = NULL;
    mCachedGunImage = NULL;
    mCachedGunAngle = 0.0f;
    mShowNextBall = true;
}

Gun::~Gun()
{
}

void Gun::SyncState(DataSync &theSync)
{
    DataReader *aReader = theSync.mReader;
    DataWriter *aWriter = theSync.mWriter;

    if (aReader)
    {
        EmptyBullets();

        bool aHasBullet = aReader->ReadBool();
        if (aHasBullet)
        {
            mBullet = new Bullet();
            mBullet->SyncState(theSync);
        }

        bool aHasNextBullet = aReader->ReadBool();
        if (aHasNextBullet)
        {
            mNextBullet = new Bullet();
            mNextBullet->SyncState(theSync);
        }
    }
    else
    {
        aWriter->WriteBool(mBullet != NULL);
        if (mBullet)
            mBullet->SyncState(theSync);

        aWriter->WriteBool(mNextBullet != NULL);
        if (mNextBullet)
            mNextBullet->SyncState(theSync);
    }

    theSync.SyncFloat(mAngle);
    theSync.SyncShort(mCenterX);
    theSync.SyncShort(mCenterY);
    theSync.SyncShort(mRecoilX1);
    theSync.SyncShort(mRecoilY1);
    theSync.SyncShort(mRecoilX2);
    theSync.SyncShort(mRecoilY2);
    theSync.SyncShort(mRecoilCount);
    theSync.SyncFloat(mStatePercent);
    theSync.SyncFloat(mFireVel);
    theSync.SyncShort(mBlinkCount);
    theSync.SyncBool(mWink);
    theSync.SyncFloat(mCachedGunAngle);

    if (aReader)
    {
        theSync.SyncBytes(&mState, 4);
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mState = (GunState)ByteSwap((unsigned int)GunState);
#endif
    }
    else
    {
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mState = (GunState)ByteSwap((unsigned int)GunState);
#endif
        theSync.SyncBytes(&mState, 4);
    }

    if (aReader)
    {
        CreateCachedGunImage();
    }
}

bool Gun::NeedsReload()
{
    return mNextBullet == NULL || mBullet == NULL;
}

// void Gun::DeleteBullet()
// {
// }

void Gun::Reload(int theType, bool delay, PowerType thePower)
{
    Bullet *aBullet = new Bullet();
    aBullet->SetType(theType);
    aBullet->SetPowerType(thePower, false);

    mStatePercent = 0.0f;
    if (mBullet)
        delete mBullet;

    mBullet = mNextBullet;
    mNextBullet = aBullet;
    mState = GunState_Reloading;

    if (!delay)
    {
        mState = GunState_Normal;
        mStatePercent = 1.0f;
    }

    CalcAngle();
}

void Gun::SetAngle(float theAngle)
{
    mAngle = theAngle;
    CalcAngle();
}

void Gun::Draw(Graphics *g)
{
    int aCornerX = mCenterX - 108;
    int aCornerY = mCenterY - 108;

    g->DrawImageRotated(Sexy::IMAGE_SHOOTER_BOTTOM, aCornerX, aCornerY, mAngle, 108, 108);

    int aOff;
    float scaleFactor = 216.0f / 108.0f;

    switch (mState)
    {
    case GunState_Normal:
        aOff = static_cast<int>(51 * scaleFactor); // When holding a ball
        break;
    case GunState_Firing:
        aOff = static_cast<int>((mStatePercent * 30.0f) + (1.0f - mStatePercent) * 51.0f * scaleFactor);
        break;
    case GunState_Reloading:
        aOff = static_cast<int>((mStatePercent * 51.0f) + (1.0f - mStatePercent) * 30.0f * scaleFactor);
        break;
    }

    int aTongueSomething = static_cast<int>(17 * scaleFactor); // default value I guess

    if (mBullet != NULL)
    {
        aTongueSomething = static_cast<int>(54 * scaleFactor) - aOff; // oh don't ask me
    }
    else
    {
        aOff = static_cast<int>(37 * scaleFactor); // This is without any ball in mouth
    }

    g->DrawImageRotated(Sexy::IMAGE_SHOOTER_TONGUE, aCornerX + static_cast<int>(38 * scaleFactor), aCornerY + aOff, mAngle, static_cast<int>(16 * scaleFactor), aTongueSomething);



    if (mBullet != NULL)
        mBullet->Draw(g);

    if (mShowNextBall)
    {
        if (mNextBullet != NULL && mState != GunState_Reloading)
        {
            int aColorWidth = Sexy::IMAGE_NEXT_BALL->mWidth / 6;
            int aColorHeight = Sexy::IMAGE_NEXT_BALL->mHeight;
            Rect aColorRect(mNextBullet->GetType() * aColorWidth, 0, aColorWidth, aColorHeight);

            g->DrawImageRotatedF(Sexy::IMAGE_NEXT_BALL, aCornerX + static_cast<int>(47 * scaleFactor), aCornerY + static_cast<int>(22 * scaleFactor), mAngle, static_cast<int>(7 * scaleFactor), static_cast<int>(32 * scaleFactor), &aColorRect);
        }
    }

    g->DrawImageRotated(gShooterTop, aCornerX + static_cast<int>(31 * scaleFactor), aCornerY + static_cast<int>(12 * scaleFactor), mAngle, static_cast<int>(23 * scaleFactor), static_cast<int>(42 * scaleFactor));

    if (mBlinkCount == 0)
        return;

    int blink = 0;

    if (mBlinkCount > 4 && mBlinkCount < 20)
    {
        blink = 1;
    }
    else if (mBlinkCount > 24)
    {
        return;
    }

    int aWidth = Sexy::IMAGE_EYE_BLINK->mWidth;
    int aHeight = Sexy::IMAGE_EYE_BLINK->mHeight / 2;
    if (mWink)
        mWidth /= 2;

    Rect anEyeRect(0, blink * aHeight, aWidth, aHeight);
    g->DrawImageRotatedF(Sexy::IMAGE_EYE_BLINK, aCornerX + static_cast<int>(25 * scaleFactor), aCornerY + static_cast<int>(42 * scaleFactor), mAngle, static_cast<int>(29 * scaleFactor), static_cast<int>(12 * scaleFactor), &anEyeRect);
}

void Gun::DrawShadow(Graphics *g)
{
    // empty
}

bool Gun::StartFire(bool recoil)
{
    if (mState != GunState_Normal || mBullet == NULL)
        return false;

    mStatePercent = 0.0f;
    mState = GunState_Firing;
    mCenterX = mRecoilX1;
    mCenterY = mRecoilY1;

    Bullet *aBullet = mBullet;
    float rad = mAngle - (SEXY_PI / 2);
    float vx = cosf(rad);
    float vy = -sinf(rad);

    aBullet->SetVelocity(vx * mFireVel, vy * mFireVel);
    aBullet->SetPos(
        -40.0f * vx + aBullet->GetX(),
        -40.0f * vy + aBullet->GetY());

    mRecoilX1 = mCenterX;
    mRecoilY1 = mCenterY;
    mRecoilX2 = mCenterX + vx * -6.0f;
    mRecoilY2 = mCenterY + vy * -6.0f;

    if (recoil)
        mRecoilCount = 25;

    mWink = false;
    mBlinkCount = 25;

    CalcAngle();

    return true;
}

Bullet *Gun::GetFiredBullet()
{
    if (mState == GunState_Firing && mStatePercent >= 1)
    {
        Bullet *aBullet = mBullet;
        mBullet = NULL;
        mState = GunState_Normal;

        return aBullet;
    }

    return NULL;
}

void Gun::SetPos(int theX, int theY)
{
    mCenterX = theX;
    mCenterY = theY;
    mRecoilX1 = theX;
    mRecoilY1 = theY;

    CalcAngle();
}

void Gun::SetSize(int theWidth, int theHeight)
{
    // educated guess, not in executable
    mWidth = theWidth;
    mHeight = theHeight;
}

void Gun::Update()
{
    if (mRecoilCount > 0)
    {
        mRecoilCount--;
        if (mRecoilCount <= 20)
        {
            if (mRecoilCount == 1)
            {
                mCenterX = mRecoilX1;
                mCenterY = mRecoilY1;
            }
            else
            {
                if (mRecoilCount > 14)
                {
                    mCenterX = (mRecoilCount - 16) * (mRecoilX1 - mRecoilX2) / 5 + mRecoilX2;
                    mCenterY = (mRecoilCount - 16) * (mRecoilY1 - mRecoilY2) / 5 + mRecoilY2;
                }
                else
                {
                    mCenterX = mRecoilX1 + mRecoilCount * (mRecoilX2 - mRecoilX1) / 15;
                    mCenterY = mRecoilY1 + mRecoilCount * (mRecoilY2 - mRecoilY1) / 15;
                }
            }
        }
    }

    if (mBlinkCount > 0)
        mBlinkCount--;

    if (mState == GunState_Firing)
    {
        mStatePercent += 0.15f;

        if (mStatePercent > 0.6f)
            mBullet->Update();
    }
    else
    {
        mStatePercent += 0.07f;
    }

    if (mStatePercent > 1.0f)
    {
        mStatePercent = 1.0f;
        if (mState == GunState_Reloading)
            mState = GunState_Normal;
    }

    CalcAngle();
}

void Gun::SwapBullets(bool playSound)
{
    if (mState != GunState_Normal)
        return;

    if (mBullet == NULL)
        return;

    if (mNextBullet == NULL)
        return;

    if (mBullet->GetType() == mNextBullet->GetType())
        return;

    if (playSound)
    {
        gSexyAppBase->PlaySample(Sexy::SOUND_BUTTON2);
    }

    Bullet *aBullet = mBullet;
    mBullet = mNextBullet;
    mNextBullet = aBullet;

    CalcAngle();
}

void Gun::EmptyBullets()
{
    mState = GunState_Normal;

    if (mNextBullet != NULL)
    {
        delete mNextBullet;
        mNextBullet = NULL;
    }

    if (mBullet != NULL)
    {
        delete mBullet;
        mBullet = NULL;
    }
}

void Gun::CreateCachedGunImage()
{
    if (mCachedGunImage == NULL)
    {
        int *aSide = (mWidth < mHeight) ? &mHeight : &mWidth;

        int aSize = *aSide * 1.25;

        mCachedGunImage = new MemoryImage(gSexyAppBase);
        mCachedGunImage->Create(aSize, aSize);
    }

    mCachedGunImage->Clear();

    int aCenterX = mCenterX;
    int aCenterY = mCenterY;
    int aBlinkCount = mBlinkCount;
    float anAngle = mAngle;
    Bullet *aNextBullet = mNextBullet;
    Bullet *aBullet = mBullet;

    mNextBullet = NULL;
    mBullet = NULL;
    mBlinkCount = 0;
    mCenterX = mCachedGunImage->mWidth / 2;
    mCenterY = mCachedGunImage->mHeight / 2;
    mRecoilX1 = mCachedGunImage->mWidth / 2;
    mRecoilY1 = mCachedGunImage->mHeight / 2;
    CalcAngle();

    if (gSexyAppBase->Is3DAccelerated())
    {
        mAngle = 0.0f;
        CalcAngle();
    }

    mCachedGunAngle = mAngle;

    Graphics g(mCachedGunImage);
    Draw(&g);

    mCenterX = aCenterX;
    mCenterY = aCenterY;
    mRecoilX1 = aCenterX;
    mRecoilY1 = aCenterY;
    CalcAngle();

    mAngle = anAngle;
    CalcAngle();

    mBlinkCount = aBlinkCount;
    mNextBullet = aNextBullet;
    mBullet = aBullet;
}

void Gun::DrawCachedGunImage(Graphics *g, float theZoom)
{
    int aWidth = mCachedGunImage->mWidth;
    int aCenterX = aWidth / 2;
    int aHeight = mCachedGunImage->mHeight;
    int aCenterY = aHeight / 2;

    Rect a4(0, 0, aWidth, aHeight);

    if (gSexyAppBase->Is3DAccelerated())
    {
        SexyTransform2D aTrans;
        aTrans.Translate(-aCenterX, -aCenterY);
        aTrans.Scale(theZoom, theZoom);
        // aTrans.RotateRad(mAngle);
        aTrans.Rotate(-mAngle);
        aTrans.Translate(mCenterX + ((theZoom - 1.0f) * -50.0f),
                         mCenterY + ((theZoom - 1.0f) * 100.0f));

        D3DInterface *aD3D = gSexyAppBase->mDDInterface->mD3DInterface;
        aD3D->BltTransformed(mCachedGunImage, &g->mClipRect, Color(0, 0, 0, 96), Graphics::DRAWMODE_NORMAL, a4, aTrans, true);

        aTrans.LoadIdentity();
        aTrans.Translate(-aCenterX, -aCenterY);
        aTrans.Scale(theZoom, theZoom);
        // aTrans.RotateRad(mAngle);
        aTrans.Rotate(-mAngle);
        aTrans.Translate(mCenterX, mCenterY);
        aD3D->BltTransformed(mCachedGunImage, &g->mClipRect, Color::White, Graphics::DRAWMODE_NORMAL, a4, aTrans, true);
    }
    else
    {
        Rect a3(mCenterX - (aCenterX * theZoom),
                mCenterY - (aCenterY * theZoom),
                (2 * aCenterX) * theZoom,
                (2 * aCenterY) * theZoom);

        bool isFastStretch = g->GetFastStretch();
        g->SetFastStretch(true);
        g->DrawImage(mCachedGunImage, a3, a4);
        g->SetFastStretch(isFastStretch);
    }
}

void Gun::SetBulletType(int theType)
{
    if (mBullet != NULL)
        mBullet->SetType(theType);
}

void Gun::SetNextBulletType(int theType)
{
    if (mNextBullet != NULL)
        mNextBullet->SetType(theType);
}

void Gun::DoBlink(bool wink)
{
    mWink = wink;
    mBlinkCount = 25;
}

static void RotateXY(float &x, float &y, float cx, float cy, float rad)
{
    float ox = x - cx;
    float oy = y - cy;

    x = cx + ox * cosf(rad) + oy * sinf(rad);
    y = cy + oy * cosf(rad) - ox * sinf(rad);
}

void Gun::CalcAngle()
{
    if (!mBullet)
        return;

    float start = mCenterY - 20;
    float end = mCenterY + 25 * 2;
    float aPointX = (mCenterX + 1);
    float aPointY = 0.0f;

    if (mState == GunState_Normal)
    {
        aPointY = end;
    }
    else if (mState == GunState_Reloading)
    {
        aPointY = start + (end - start) * mStatePercent;
    }
    else if (mStatePercent <= 0.6f)
    {
        aPointY = end + (((mCenterY + 10) - end) * mStatePercent) / 0.6f;
    }
    else
    {
        return;
    }

    RotateXY(aPointX, aPointY, mCenterX, mCenterY - 5, mAngle);
    mBullet->SetPos(aPointX, aPointY);
    mBullet->SetRotation(mAngle);
}
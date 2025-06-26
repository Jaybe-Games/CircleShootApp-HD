#include "Zuma_Prefix.pch"

#include "CurveData.h"
#include "Board.h"
#include "Bullet.h"
#include "CurveMgr.h"
#include "WayPoint.h"
#include "DataSync.h"

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int gVersion = 3;
float INV_SUBPIXEL_MULT = 0.01f;

// Definiere die neue Auflösung
const float NEW_WIDTH = 1280.0f;
const float NEW_HEIGHT = 960.0f;
const float OLD_WIDTH = 640.0f;
const float OLD_HEIGHT = 480.0f;

// Berechne die Skalierungsfaktoren
const float SCALE_X = NEW_WIDTH / OLD_WIDTH;
const float SCALE_Y = NEW_HEIGHT / OLD_HEIGHT;

// Definiere den Offset
const float OFFSET_X = 320.0f;
const float OFFSET_Y = 60.0f;

CurveData::CurveData()
{
    Clear();
}

bool CurveData::Save(const std::string& theFilePath, bool smallVersion)
{
    // TODO: doesn't seem to be present in any .exe?

    return false;
}

bool CurveData::Load(const std::string& theFilePath)
{
    Clear();

    DataReader aReader;
    if (!aReader.OpenFile(theFilePath))
    {
        mErrorString = "Failed to open file";
        return false;
    }

    char aBuf[5];
    aBuf[3] = 0;
    aBuf[4] = 0;

    aReader.ReadBytes(aBuf, 4);
    if (strcmp(aBuf, "CURV") != 0)
    {
        mErrorString = "Invalid file header";
        return false;
    }

    mVersion = aReader.ReadLong();
    if (mVersion < 1 || mVersion > gVersion)
    {
        mErrorString = "Invalid file version";
        return false;
    }

    bool smallVersion;
    bool hasTunnels;

    if (mVersion <= 2)
    {
        smallVersion = false;
        hasTunnels = true;
    }
    else
    {
        smallVersion = aReader.ReadBool();
        hasTunnels = aReader.ReadBool();
    }

    if (smallVersion)
    {
        mEditType = 0;
    }
    else
    {
        mEditType = aReader.ReadLong();
        ulong aBufferSize = aReader.ReadLong();

        if (aBufferSize > 1000000)
        {
            mErrorString = "File is corrupt";
            return false;
        }

        uchar* aByteArray = new uchar[aBufferSize];
        aReader.ReadBytes(aByteArray, aBufferSize);
        mEditData.WriteBytes(aByteArray, aBufferSize);
        delete[] aByteArray;
    }

    ulong aSize = aReader.ReadLong();
    if (aSize > 0)
    {
        if (mVersion > 1)
        {
            mPointList.push_back(PathPoint());
            PathPoint& aStartPoint = mPointList.back();

            aStartPoint.x = aReader.ReadFloat() * SCALE_X + OFFSET_X; // Angepasst für neuen Offset
            aStartPoint.y = aReader.ReadFloat() * SCALE_Y + OFFSET_Y; // Angepasst für neuen Offset

            if (hasTunnels)
            {
                aStartPoint.mInTunnel = aReader.ReadBool();
                aStartPoint.mPriority = aReader.ReadByte();
            }

            float ox = aStartPoint.x;
            float oy = aStartPoint.y;

            for (ulong i = 0; i < aSize - 1; i++)
            {
                mPointList.push_back(PathPoint());
                PathPoint& aPoint = mPointList.back();

                signed char dx = aReader.ReadByte();
                signed char dy = aReader.ReadByte();

                aPoint.x = (dx * INV_SUBPIXEL_MULT * SCALE_X) + ox; // Angepasst für neuen Offset
                aPoint.y = (dy * INV_SUBPIXEL_MULT * SCALE_Y) + oy; // Angepasst für neuen Offset

                if (hasTunnels)
                {
                    aPoint.mInTunnel = aReader.ReadBool();
                    aPoint.mPriority = aReader.ReadByte();
                }

                ox = aPoint.x;
                oy = aPoint.y;
            }
        }
        else
        {
            for (ulong i = 0; i < aSize; i++)
            {
                mPointList.push_back(PathPoint());
                PathPoint& aPoint = mPointList.back();

                aPoint.x = aReader.ReadFloat() * SCALE_X + OFFSET_X; // Angepasst für neuen Offset
                aPoint.y = aReader.ReadFloat() * SCALE_Y + OFFSET_Y; // Angepasst für neuen Offset
                aPoint.mInTunnel = aReader.ReadBool();
                aPoint.mPriority = aReader.ReadByte();
            }
        }
    }

    return true;
}

void CurveData::Clear()
{
    mPointList.clear();
    mEditType = 0;
    mEditData.Clear();
}


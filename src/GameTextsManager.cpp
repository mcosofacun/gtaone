#include "stdafx.h"
#include "GameTextsManager.h"
#include "cvars.h"

//////////////////////////////////////////////////////////////////////////

class FXTReader
{
public:
/*
    https://gta.mendelsohn.de/Reference/Fil_FXT.html

    Decryption
    The messages are encrypted using a polyalphabetic substitution cipher that appears like a monoalphabetic cypher after byte 8, which made it easy to crack.

    Subtract an offset from the first 8 bytes in the file. The respective offsets are, in hexadecimal notation, 
        63, c6, 8c, 18, 30, 60, c0, 80. (Of course you can get these numbers by taking 63 and shifting it one bit to left for each consecutive number.
        You can see why this sequence ends after 8 bytes; it would�ve been harder to crack, had the bit pattern been rotated instead of shifted...)

    Subtract 1 from all bytes. (This includes the bytes from the previous step!)

    If a byte equals 195, delete it and add 64 to the next byte. This relocates extended characters to the windows ANSI character set, 
    which is necessary for three of the four languages provided.

    Each string ends with a 0 byte and contains no CR or LF codes. The next string is stored immediately after it. 
    The last string in the file seems to always be "[]�.
*/

    FXTReader(std::istream& inputStream)
        : mInputStream(inputStream)
    {
    }

    bool get_next_key_value(std::string& outputKey, std::string& outputValue)
    {
        if (!get_next_key(outputKey) || outputKey.empty())
            return false;

        if (!get_next_value(outputValue))
            return false;

        return true;
    }

private:
    bool get_next_key(std::string& outputKey)
    {
        unsigned char currChar;
        if (!get_next_char(currChar))
            return false;

        if (currChar != '[')
            return false;

        outputKey.clear();
        for (;;)
        {
            if (!get_next_char(currChar))
                return false;

            if (currChar == ']')
                break;

            outputKey.push_back(currChar);
        }

        return true;
    }

    bool get_next_value(std::string& outputValue)
    {
        outputValue.clear();
        for (unsigned char currChar;;)
        {
            if (!get_next_char(currChar))
                return false;

            if (currChar == 0)
                break;

            outputValue.push_back(currChar);
        }
        return true;
    }

    bool get_next_char(unsigned char& character)
    {
        const int EncryptedBytesCount = 8;

        if (!mInputStream.read((char*) &character, 1))
            return false;

        // detect encryption method
        if (mBytesCounter == 0)
        {
            if (character == 0xBF)
            {
                mEcnKey = 0x63;
                mEncOffset = -1;
            }
            else if (character == 0xA6)
            {
                mEcnKey = 0x67;
                mEncOffset = 28;
            }
            else
            {
                cxx_assert(false);
                return false;
            }
        }

        if (mBytesCounter < EncryptedBytesCount)
        {
            character = character - mEcnKey;
            mEcnKey <<= 1; 
        }

        ++mBytesCounter;
		if ((character + mEncOffset) == 195) 
        {
            if (!mInputStream.read((char*) &character, 1))
            {
                cxx_assert(false);
                return false;
            }
            ++mBytesCounter;
            character += 64;
		};
        character = (character + mEncOffset);
        return true;
    }

private:
    std::istream& mInputStream;
    int mBytesCounter = 0;
    int mEncOffset = 0;
    unsigned char mEcnKey = 0;
};

//////////////////////////////////////////////////////////////////////////

bool GameTextsManager::Initialize()
{
    mErrorString = "***Text missed***";

    // init texts
    if (!gCvarGameLanguage.mValue.empty())
    {
        gSystem.LogMessage(eLogMessage_Debug, "Set game language: '%s'", gCvarGameLanguage.mValue.c_str());
    }

    std::string textsFilename = GetTextsLanguageFileName(gCvarGameLanguage.mValue);
    gSystem.LogMessage(eLogMessage_Debug, "Loading game texts from '%s'", textsFilename.c_str());

    if (!LoadTexts(textsFilename))
    {
        gSystem.LogMessage(eLogMessage_Warning, "Fail to load game texts for current language");
    }

    return true;
}

void GameTextsManager::Deinit()
{
    mStrings.clear();
}

const std::string& GameTextsManager::GetText(const std::string& textID) const
{
    auto find_iterator = mStrings.find(textID);
    if (find_iterator == mStrings.end())
        return mErrorString;

    return find_iterator->second;
}

bool GameTextsManager::LoadTexts(const std::string& fileName)
{
    std::ifstream fileStream;
    if (!gSystem.mFiles.OpenBinaryFile(fileName, fileStream))
    {
        gSystem.LogMessage(eLogMessage_Warning, "Cannot open texts file '%s'", fileName.c_str());
        return false;
    }

    mStrings.clear();

    FXTReader fxtreader(fileStream);
    
    std::string key;
    std::string value;

    for (;;)
    {
        if (!fxtreader.get_next_key_value(key, value))
            break;

        mStrings[key] = value;
    }
    
    return true;
}

std::string GameTextsManager::GetTextsLanguageFileName(const std::string& languageID) const
{
    if ((gCvarGameVersion.mValue == eGtaGameVersion_Demo) || (gCvarGameVersion.mValue == eGtaGameVersion_Full))
    {
        if (cxx_stricmp(languageID.c_str(), "en") == 0)
        {
            return "ENGLISH.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "fr") == 0)
        {
            return "FRENCH.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "de") == 0)
        {
            return "GERMAN.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "it") == 0)
        {
            return "ITALIAN.FXT";
        }

        return "ENGLISH.FXT";
    }

    if (gCvarGameVersion.mValue == eGtaGameVersion_MissionPack1_London69)
    {
        if (cxx_stricmp(languageID.c_str(), "en") == 0)
        {
            return "ENGUK.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "fr") == 0)
        {
            return "FREUK.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "de") == 0)
        {
            return "GERUK.FXT";
        }

        if (cxx_stricmp(languageID.c_str(), "it") == 0)
        {
            return "ITAUK.FXT";
        }

        return "ENGUK.FXT";
    }

    if (gCvarGameVersion.mValue == eGtaGameVersion_MissionPack2_London61)
    {
        return "enguke.fxt";
    }
   
    return "ENGLISH.FXT";
}
#pragma once

// Class is responsible for reading and storing game messages
class GameTextsManager final: public cxx::noncopyable
{
public:
    bool Initialize();
    void Deinit();

    // Find game text by text identifier
    // @returns default error message on nothing found
    const std::string& GetText(const std::string& textID) const;

private:
    // Loads game texts from source file
    bool LoadTexts(const std::string& fileName);

    std::string GetTextsLanguageFileName(const std::string& languageID) const;

private:
    std::map<std::string, std::string> mStrings;
    std::string mErrorString;
};
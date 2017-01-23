#include "platform/platform.hpp"
#include <windows.h>

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include "coding/file_writer.hpp"

#include "std/windows.hpp"
#include "std/bind.hpp"

#include <direct.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "socket.hpp"

static bool GetUserWritableDir(string & outDir)
{
  char pathBuf[MAX_PATH] = {0};
  if (SUCCEEDED(::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, pathBuf)))
  {
    outDir = pathBuf;
    ::CreateDirectoryA(outDir.c_str(), NULL);
    outDir += "\\MapsWithMe\\";
    ::CreateDirectoryA(outDir.c_str(), NULL);
    return true;
  }
  return false;
}

/// @return Full path to the executable file
static bool GetPathToBinary(string & outPath)
{
  // get path to executable
  char pathBuf[MAX_PATH] = {0};
  if (0 < ::GetModuleFileNameA(NULL, pathBuf, MAX_PATH))
  {
    outPath = pathBuf;
    return true;
  }
  return false;
}

Platform::Platform()
{
  string path;
  CHECK(GetPathToBinary(path), ("Can't get path to binary"));

  // resources path:
  // 1. try to use data folder in the same path as executable
  // 2. if not found, try to use ..\..\..\data (for development only)
  path.erase(path.find_last_of('\\'));
  if (IsFileExistsByFullPath(path + "\\data\\"))
    m_resourcesDir = path + "\\data\\";
  else
  {
#ifndef OMIM_PRODUCTION
    path.erase(path.find_last_of('\\'));
    path.erase(path.find_last_of('\\'));
    if (IsFileExistsByFullPath(path + "\\data\\"))
      m_resourcesDir = path + "\\data\\";
#else
    CHECK(false, ("Can't find resources directory"));
#endif
  }
  m_resourcesDir = "C:\\Users\\g.smorkalov\\Documents\\Projects\\omim\\data\\";

  // writable path:
  // 1. the same as resources if we have write access to this folder
  // 2. otherwise, use system-specific folder
  try
  {
    FileWriter tmpfile(m_resourcesDir + "mapswithmetmptestfile");
    tmpfile.Write("Hi from Alex!", 13);
    m_writableDir = m_resourcesDir;
  }
  catch (RootException const &)
  {
    CHECK(GetUserWritableDir(m_writableDir), ("Can't get writable directory"));
  }
  FileWriter::DeleteFileX(m_resourcesDir + "mapswithmetmptestfile");

  m_settingsDir = m_writableDir;
  char pathBuf[MAX_PATH] = {0};
  GetTempPathA(MAX_PATH, pathBuf);
  m_tmpDir = pathBuf;

  LOG(LDEBUG, ("Resources Directory:", m_resourcesDir));
  LOG(LDEBUG, ("Writable Directory:", m_writableDir));
  LOG(LDEBUG, ("Tmp Directory:", m_tmpDir));
  LOG(LDEBUG, ("Settings Directory:", m_settingsDir));
}

bool Platform::IsFileExistsByFullPath(string const & filePath)
{
  return ::GetFileAttributesA(filePath.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// static
Platform::EError Platform::RmDir(string const & dirName)
{
  if (_rmdir(dirName.c_str()) != 0)
    return ErrnoToError();
  return ERR_OK;
}

#ifdef FILE_TYPE_UNKNOWN
#undef FILE_TYPE_UNKNOWN
#endif

// static
Platform::EError Platform::GetFileType(string const & path, EFileType & type)
{
  struct _stat32 stats;
  if (_stat32(path.c_str(), &stats) != 0)
    return ErrnoToError();
  if (stats.st_mode & _S_IFREG)
    type = FILE_TYPE_REGULAR;
  else if (stats.st_mode & _S_IFDIR)
    type = FILE_TYPE_DIRECTORY;
  else
    type = FILE_TYPE_UNKNOWN;
  return ERR_OK;
}

string Platform::UniqueClientId() const
{
  return "@TODO";
}

void Platform::RunOnGuiThread(TFunctor const & fn)
{
  /// @todo
  fn();
}

void Platform::RunAsync(TFunctor const & fn, Priority p)
{
  /// @todo
  fn();
}

Platform::EConnectionType Platform::ConnectionStatus()
{
  // @TODO Add implementation
  return EConnectionType::CONNECTION_NONE;
}

Platform::TStorageStatus Platform::GetWritableStorageStatus(uint64_t neededSize) const
{
  ULARGE_INTEGER freeSpace;
  if (0 == ::GetDiskFreeSpaceExA(m_writableDir.c_str(), &freeSpace, NULL, NULL))
  {
    LOG(LWARNING, ("GetDiskFreeSpaceEx failed with error", GetLastError()));
    return STORAGE_DISCONNECTED;
  }

  if (freeSpace.u.LowPart + (static_cast<uint64_t>(freeSpace.u.HighPart) << 32) < neededSize)
    return NOT_ENOUGH_SPACE;

  return STORAGE_OK;
}

bool Platform::IsDirectoryEmpty(string const & directory)
{
    return PathIsDirectoryEmptyA(directory.c_str());
}

bool Platform::GetFileSizeByFullPath(string const & filePath, uint64_t & size)
{
  HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    MY_SCOPE_GUARD(autoClose, bind(&CloseHandle, hFile));
    LARGE_INTEGER fileSize;
    if (0 != GetFileSizeEx(hFile, &fileSize))
    {
      size = fileSize.QuadPart;
      return true;
    }
  }
  return false;
}

void Platform::GetSystemFontNames(FilesList & res) const
{
  char const * fontsWhitelist[] = {
    "Roboto-Medium.ttf",
    "Roboto-Regular.ttf",
    "DroidSansFallback.ttf",
    "DroidSansFallbackFull.ttf",
    "DroidSans.ttf",
    "DroidSansArabic.ttf",
    "DroidSansSemc.ttf",
    "DroidSansSemcCJK.ttf",
    "DroidNaskh-Regular.ttf",
    "Lohit-Bengali.ttf",
    "Lohit-Devanagari.ttf",
    "Lohit-Tamil.ttf",
    "PakType Naqsh.ttf",
    "wqy-microhei.ttc",
    "Jomolhari.ttf",
    "Jomolhari-alpha3c-0605331.ttf",
    "Padauk.ttf",
    "KhmerOS.ttf",
    "Umpush.ttf",
    "DroidSansThai.ttf",
    "DroidSansArmenian.ttf",
    "DroidSansEthiopic-Regular.ttf",
    "DroidSansGeorgian.ttf",
    "DroidSansHebrew-Regular.ttf",
    "DroidSansHebrew.ttf",
    "DroidSansJapanese.ttf",
    "LTe50872.ttf",
    "LTe50259.ttf",
    "DevanagariOTS.ttf",
    "FreeSans.ttf",
    "DejaVuSans.ttf",
    "arial.ttf",
    "AbyssinicaSIL-R.ttf",
  };

  char const * systemFontsPath[] = {
    "/system/fonts/",
  };

  const uint64_t fontSizeBlacklist[] = {
    183560,   // Samsung Duos DroidSans
    7140172,  // Serif font without Emoji
    14416824  // Serif font with Emoji
  };

  uint64_t fileSize = 0;

  for (size_t i = 0; i < ARRAY_SIZE(fontsWhitelist); ++i)
  {
    for (size_t j = 0; j < ARRAY_SIZE(systemFontsPath); ++j)
    {
      string const path = string(systemFontsPath[j]) + fontsWhitelist[i];
      if (IsFileExistsByFullPath(path))
      {
        if (GetFileSizeByName(path, fileSize))
        {
          uint64_t const * end = fontSizeBlacklist + ARRAY_SIZE(fontSizeBlacklist);
          if (find(fontSizeBlacklist, end, fileSize) == end)
          {
            res.push_back(path);
            LOG(LINFO, ("Found usable system font", path, "with file size", fileSize));
          }
        }
      }
    }
  }
}

uint64_t Platform::GetWritableStorageSpace() const
{
  ULARGE_INTEGER space;
  GetDiskFreeSpaceExA(m_writableDir.c_str(), &space, NULL, NULL);
  return space.QuadPart;
}

namespace platform
{
unique_ptr<Socket> CreateSocket()
{
  return unique_ptr<Socket>();
}
}

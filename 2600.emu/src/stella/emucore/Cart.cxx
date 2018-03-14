//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Settings.hxx"
#include "System.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "CartDebug.hxx"
#endif

#include "Cart.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Settings& settings)
  : mySettings(settings),
    myStartBank(0),
    myBankChanged(true),
    myCodeAccessBase(nullptr),
    myBankLocked(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::setAbout(const string& about, const string& type,
                         const string& id)
{
  myAbout = about;
  myDetectedType = type;
  myMultiCartID = id;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::saveROM(ofstream& out) const
{
  uInt32 size = 0;

  const uInt8* image = getImage(size);
  if(image == nullptr || size == 0)
  {
    cerr << "save not supported" << endl;
    return false;
  }

  out.write(reinterpret_cast<const char*>(image), size);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::bankChanged()
{
  bool changed = myBankChanged;
  myBankChanged = false;
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::triggerReadFromWritePort(uInt16 address)
{
#ifdef DEBUGGER_SUPPORT
  if(!mySystem->autodetectMode())
    Debugger::debugger().cartDebug().triggerReadFromWritePort(address);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::createCodeAccessBase(uInt32 size)
{
#ifdef DEBUGGER_SUPPORT
  myCodeAccessBase = make_unique<uInt8[]>(size);
  memset(myCodeAccessBase.get(), CartDebug::ROW, size);
#else
  myCodeAccessBase = nullptr;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::initializeRAM(uInt8* arr, uInt32 size, uInt8 val) const
{
  if(randomInitialRAM())
    for(uInt32 i = 0; i < size; ++i)
      arr[i] = mySystem->randGenerator().next();
  else
    memset(arr, val, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomInitialRAM() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.ramrandom" : "plr.ramrandom");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::randomizeStartBank()
{
  if(randomStartBank())
    myStartBank = mySystem->randGenerator().next() % bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomStartBank() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.bankrandom" : "plr.bankrandom");
}

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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Settings.hxx"
#include "System.hxx"
#include "MD5.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "CartDebug.hxx"
#endif

#include "Cart.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Settings& settings, const string& md5)
  : mySettings(settings)
{
  auto to_uInt32 = [](const string& s, uInt32 pos) {
    return uInt32(std::stoul(s.substr(pos, 8), nullptr, 16));
  };

  uInt32 seed = to_uInt32(md5, 0)  ^ to_uInt32(md5, 8) ^
                to_uInt32(md5, 16) ^ to_uInt32(md5, 24);
  Random rand(seed);
  for(uInt32 i = 0; i < 256; ++i)
    myRWPRandomValues[i] = rand.next();

  myRAMAccesses.reserve(5);
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
  size_t size = 0;

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
uInt8 Cartridge::peekRAM(uInt8& dest, uInt16 address)
{
  uInt8 value = myRWPRandomValues[address & 0xFF];

  // Reading from the write port triggers an unwanted write
  // But this only happens when in normal emulation mode
#ifdef DEBUGGER_SUPPORT
  if(!bankLocked() && !mySystem->autodetectMode())
  {
    // Record access here; final determination will happen in ::pokeRAM()
    myRAMAccesses.push_back(address);
    dest = value;
  }
#else
  if(!mySystem->autodetectMode())
    dest = value;
#endif
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::pokeRAM(uInt8& dest, uInt16 address, uInt8 value)
{
#ifdef DEBUGGER_SUPPORT
  for(auto i = myRAMAccesses.begin(); i != myRAMAccesses.end(); ++i)
  {
    if(*i == address)
    {
      myRAMAccesses.erase(i);
      break;
    }
  }
#endif
  dest = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::createCodeAccessBase(size_t size)
{
#ifdef DEBUGGER_SUPPORT
  myCodeAccessBase = make_unique<uInt8[]>(size);
  std::fill_n(myCodeAccessBase.get(), size, CartDebug::ROW);
#else
  myCodeAccessBase = nullptr;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::initializeRAM(uInt8* arr, size_t size, uInt8 val) const
{
  if(randomInitialRAM())
    for(size_t i = 0; i < size; ++i)
      arr[i] = mySystem->randGenerator().next();
  else
    std::fill_n(arr, size, val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::initializeStartBank(uInt16 defaultBank)
{
  int propsBank = myStartBankFromPropsFunc();

  if(randomStartBank())
    return myStartBank = mySystem->randGenerator().next() % bankCount();
  else if(propsBank >= 0)
    return myStartBank = BSPF::clamp(propsBank, 0, bankCount() - 1);
  else
    return myStartBank = BSPF::clamp(int(defaultBank), 0, bankCount() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomInitialRAM() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.ramrandom" : "plr.ramrandom");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomStartBank() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.bankrandom" : "plr.bankrandom");
}

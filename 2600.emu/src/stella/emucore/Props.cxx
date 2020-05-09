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

#include <sstream>

#include "bspf.hxx"
#include "Props.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties()
{
  setDefaults();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties(const Properties& properties)
{
  copy(properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(PropType key, const string& value)
{
  size_t pos = static_cast<size_t>(key);
  if(pos < myProperties.size())
  {
    myProperties[pos] = value;
    if(BSPF::equalsIgnoreCase(myProperties[pos], "AUTO-DETECT"))
      myProperties[pos] = "AUTO";

    switch(key)
    {
      case PropType::Cart_Sound:
      case PropType::Cart_Type:
      case PropType::Console_LeftDiff:
      case PropType::Console_RightDiff:
      case PropType::Console_TVType:
      case PropType::Console_SwapPorts:
      case PropType::Controller_Left:
      case PropType::Controller_Right:
      case PropType::Controller_SwapPaddles:
      case PropType::Controller_MouseAxis:
      case PropType::Display_Format:
      case PropType::Display_Phosphor:
      {
        BSPF::toUpperCase(myProperties[pos]);
        break;
      }

      case PropType::Display_PPBlend:
      {
        int blend = BSPF::stringToInt(myProperties[pos]);
        if(blend < 0 || blend > 100)
          myProperties[pos] = ourDefaultProperties[pos];
        break;
      }

      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
istream& operator>>(istream& is, Properties& p)
{
  p.setDefaults();

  // Loop reading properties
  string key, value;
  for(;;)
  {
    // Get the key associated with this property
    key = p.readQuotedString(is);

    // Make sure the stream is still okay
    if(!is)
      return is;

    // A null key signifies the end of the property list
    if(key == "")
      break;

    // Get the value associated with this property
    value = p.readQuotedString(is);

    // Make sure the stream is still okay
    if(!is)
      return is;

    // Set the property
    PropType type = Properties::getPropType(key);
    p.set(type, value);
  }

  return is;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const Properties& p)
{
  // Write out each of the key and value pairs
  bool changed = false;
  for(size_t i = 0; i < static_cast<size_t>(PropType::NumTypes); ++i)
  {
    // Try to save some space by only saving the items that differ from default
    if(p.myProperties[i] != Properties::ourDefaultProperties[i])
    {
      p.writeQuotedString(os, Properties::ourPropertyNames[i]);
      os.put(' ');
      p.writeQuotedString(os, p.myProperties[i]);
      os.put('\n');
      changed = true;
    }
  }

  if(changed)
  {
    // Put a trailing null string so we know when to stop reading
    p.writeQuotedString(os, "");
    os.put('\n');
    os.put('\n');
  }

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Properties::readQuotedString(istream& in)
{
  // Read characters until we see a quote
  char c;
  while(in.get(c))
    if(c == '"')
      break;

  // Read characters until we see the close quote
  string s;
  while(in.get(c))
  {
    if((c == '\\') && (in.peek() == '"'))
      in.get(c);
    else if((c == '\\') && (in.peek() == '\\'))
      in.get(c);
    else if(c == '"')
      break;
    else if(c == '\r')
      continue;

    s += c;
  }

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::writeQuotedString(ostream& out, const string& s)
{
  out.put('"');
  for(uInt32 i = 0; i < s.length(); ++i)
  {
    if(s[i] == '\\')
    {
      out.put('\\');
      out.put('\\');
    }
    else if(s[i] == '\"')
    {
      out.put('\\');
      out.put('"');
    }
    else
      out.put(s[i]);
  }
  out.put('"');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Properties::operator==(const Properties& properties) const
{
  for(size_t i = 0; i < myProperties.size(); ++i)
    if(myProperties[i] != properties.myProperties[i])
      return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Properties::operator!=(const Properties& properties) const
{
  return !(*this == properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& Properties::operator=(const Properties& properties)
{
  // Do the assignment only if this isn't a self assignment
  if(this != &properties)
  {
    // Now, make myself a copy of the given object
    copy(properties);
  }

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::setDefault(PropType key, const string& value)
{
  ourDefaultProperties[static_cast<size_t>(key)] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::copy(const Properties& properties)
{
  // Now, copy each property from properties
  for(size_t i = 0; i < myProperties.size(); ++i)
    myProperties[i] = properties.myProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::print() const
{
  cout << get(PropType::Cart_MD5)               << "|"
       << get(PropType::Cart_Name)              << "|"
       << get(PropType::Cart_Manufacturer)      << "|"
       << get(PropType::Cart_ModelNo)           << "|"
       << get(PropType::Cart_Note)              << "|"
       << get(PropType::Cart_Rarity)            << "|"
       << get(PropType::Cart_Sound)             << "|"
       << get(PropType::Cart_StartBank)         << "|"
       << get(PropType::Cart_Type)              << "|"
       << get(PropType::Console_LeftDiff)       << "|"
       << get(PropType::Console_RightDiff)      << "|"
       << get(PropType::Console_TVType)         << "|"
       << get(PropType::Console_SwapPorts)      << "|"
       << get(PropType::Controller_Left)        << "|"
       << get(PropType::Controller_Right)       << "|"
       << get(PropType::Controller_SwapPaddles) << "|"
       << get(PropType::Controller_MouseAxis)   << "|"
       << get(PropType::Display_Format)         << "|"
       << get(PropType::Display_VCenter)        << "|"
       << get(PropType::Display_Phosphor)       << "|"
       << get(PropType::Display_PPBlend)
       << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::setDefaults()
{
  for(size_t i = 0; i < myProperties.size(); ++i)
    myProperties[i] = ourDefaultProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropType Properties::getPropType(const string& name)
{
  for(size_t i = 0; i < NUM_PROPS; ++i)
    if(ourPropertyNames[i] == name)
      return static_cast<PropType>(i);

  // Otherwise, indicate that the item wasn't found
  return PropType::NumTypes;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::printHeader()
{
  cout << "Cart_MD5|"
       << "Cart_Name|"
       << "Cart_Manufacturer|"
       << "Cart_ModelNo|"
       << "Cart_Note|"
       << "Cart_Rarity|"
       << "Cart_Sound|"
       << "Cart_StartBank|"
       << "Cart_Type|"
       << "Console_LeftDiff|"
       << "Console_RightDiff|"
       << "Console_TVType|"
       << "Console_SwapPorts|"
       << "Controller_Left|"
       << "Controller_Right|"
       << "Controller_SwapPaddles|"
       << "Controller_MouseAxis|"
       << "Display_Format|"
       << "Display_VCenter|"
       << "Display_Phosphor|"
       << "Display_PPBlend"
       << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::array<string, Properties::NUM_PROPS> Properties::ourDefaultProperties =
{
  "",       // Cart.MD5
  "",       // Cart.Manufacturer
  "",       // Cart.ModelNo
  "",       // Cart.Name
  "",       // Cart.Note
  "",       // Cart.Rarity
  "MONO",   // Cart.Sound
  "AUTO",   // Cart.StartBank
  "AUTO",   // Cart.Type
  "B",      // Console.LeftDiff
  "B",      // Console.RightDiff
  "COLOR",  // Console.TVType
  "NO",     // Console.SwapPorts
  "AUTO",   // Controller.Left
  "AUTO",   // Controller.Right
  "NO",     // Controller.SwapPaddles
  "AUTO",   // Controller.MouseAxis
  "AUTO",   // Display.Format
  "0",      // Display.VCenter
  "NO",     // Display.Phosphor
  "0"       // Display.PPBlend
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::array<string, Properties::NUM_PROPS> Properties::ourPropertyNames =
{
  "Cart.MD5",
  "Cart.Manufacturer",
  "Cart.ModelNo",
  "Cart.Name",
  "Cart.Note",
  "Cart.Rarity",
  "Cart.Sound",
  "Cart.StartBank",
  "Cart.Type",
  "Console.LeftDiff",
  "Console.RightDiff",
  "Console.TVType",
  "Console.SwapPorts",
  "Controller.Left",
  "Controller.Right",
  "Controller.SwapPaddles",
  "Controller.MouseAxis",
  "Display.Format",
  "Display.VCenter",
  "Display.Phosphor",
  "Display.PPBlend"
};

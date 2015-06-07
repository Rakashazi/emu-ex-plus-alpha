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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventJoyHandler.cxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#include <sstream>
#include <map>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Vec.hxx"
#include "bspf.hxx"

#include "EventHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::StellaJoystick()
  : type(JT_NONE),
    ID(-1),
    name("None"),
    numAxes(0),
    numButtons(0),
    numHats(0),
    axisTable(nullptr),
    btnTable(nullptr),
    hatTable(nullptr),
    axisLastValue(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::~StellaJoystick()
{
  delete[] axisTable;
  delete[] btnTable;
  delete[] hatTable;
  delete[] axisLastValue;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::initialize(int index, const string& desc,
            int axes, int buttons, int hats, int /*balls*/)
{
  ID = index;
  name = desc;

  // Dynamically create the various mapping arrays for this joystick,
  // based on its specific attributes
  numAxes    = axes;
  numButtons = buttons;
  numHats    = hats;
  if(numAxes)
    axisTable = new Event::Type[numAxes][2][kNumModes];
  if(numButtons)
    btnTable = new Event::Type[numButtons][kNumModes];
  if(numHats)
    hatTable = new Event::Type[numHats][4][kNumModes];
  axisLastValue = new int[numAxes];

  // Erase the joystick axis mapping array and last axis value
  for(int a = 0; a < numAxes; ++a)
  {
    axisLastValue[a] = 0;
    for(int m = 0; m < kNumModes; ++m)
      axisTable[a][0][m] = axisTable[a][1][m] = Event::NoType;
  }

  // Erase the joystick button mapping array
  for(int b = 0; b < numButtons; ++b)
    for(int m = 0; m < kNumModes; ++m)
      btnTable[b][m] = Event::NoType;

  // Erase the joystick hat mapping array
  for(int h = 0; h < numHats; ++h)
    for(int m = 0; m < kNumModes; ++m)
      hatTable[h][0][m] = hatTable[h][1][m] =
      hatTable[h][2][m] = hatTable[h][3][m] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::getMap() const
{
  // The mapping structure (for remappable devices) is defined as follows:
  // NAME | AXIS # + values | BUTTON # + values | HAT # + values,
  // where each subsection of values is separated by ':'
  if(type == JT_REGULAR)
  {
    ostringstream joybuf;
    joybuf << name << "|" << numAxes;
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for(int k = 0; k < 2; ++k)
          joybuf << " " << axisTable[a][k][m];
    joybuf << "|" << numButtons;
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        joybuf << " " << btnTable[b][m];
    joybuf << "|" << numHats;
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          joybuf << " " << hatTable[h][k][m];

    return joybuf.str();
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::StellaJoystick::setMap(const string& mapString)
{
  istringstream buf(mapString);
  StringList items;
  string item;
  while(getline(buf, item, '|'))
    items.push_back(item);

  // Error checking
  if(items.size() != 4)
    return false;

  IntArray map;

  // Parse axis/button/hat values
  getValues(items[1], map);
  if((int)map.size() == numAxes * 2 * kNumModes)
  {
    // Fill the axes table with events
    auto event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for(int k = 0; k < 2; ++k)
          axisTable[a][k][m] = (Event::Type) *event++;
  }
  getValues(items[2], map);
  if((int)map.size() == numButtons * kNumModes)
  {
    auto event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        btnTable[b][m] = (Event::Type) *event++;
  }
  getValues(items[3], map);
  if((int)map.size() == numHats * 4 * kNumModes)
  {
    auto event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          hatTable[h][k][m] = (Event::Type) *event++;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseMap(EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
    axisTable[a][0][mode] = axisTable[a][1][mode] = Event::NoType;

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
    hatTable[h][0][mode] = hatTable[h][1][mode] =
    hatTable[h][2][mode] = hatTable[h][3][mode] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseEvent(Event::Type event, EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
  {
    if(axisTable[a][0][mode] == event)  axisTable[a][0][mode] = Event::NoType;
    if(axisTable[a][1][mode] == event)  axisTable[a][1][mode] = Event::NoType;
  }

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    if(btnTable[b][mode] == event)  btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
  {
    if(hatTable[h][0][mode] == event)  hatTable[h][0][mode] = Event::NoType;
    if(hatTable[h][1][mode] == event)  hatTable[h][1][mode] = Event::NoType;
    if(hatTable[h][2][mode] == event)  hatTable[h][2][mode] = Event::NoType;
    if(hatTable[h][3][mode] == event)  hatTable[h][3][mode] = Event::NoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::getValues(const string& list, IntArray& map) const
{
  map.clear();
  istringstream buf(list);

  int value;
  buf >> value;  // we don't need to know the # of items at this point
  while(buf >> value)
    map.push_back(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::about() const
{
  ostringstream buf;
  buf << name;
  if(type == JT_REGULAR)
    buf << " with: " << numAxes << " axes, " << numButtons << " buttons, "
        << numHats << " hats";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::JoystickHandler::JoystickHandler(OSystem& system)
  : myOSystem(system)
{
  // Load previously saved joystick mapping (if any) from settings
  istringstream buf(myOSystem.settings().getString("joymap"));
  string joymap, joyname;

  // First check the event type, and disregard the entire mapping if it's invalid
  getline(buf, joymap, '^');
  if(atoi(joymap.c_str()) == Event::LastType)
  {
    // Otherwise, put each joystick mapping entry into the database
    while(getline(buf, joymap, '^'))
    {
      istringstream namebuf(joymap);
      getline(namebuf, joyname, '|');
      if(joyname.length() != 0)
      {
        StickInfo info(joymap);
        myDatabase.insert(make_pair(joyname, info));
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::JoystickHandler::~JoystickHandler()
{
  for(const auto& i: myDatabase)
    delete i.second.joy;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::printDatabase() const
{
  cerr << "---------------------------------------------------------" << endl
       << "joy database:"  << endl;
  for(const auto& i: myDatabase)
    cerr << i.first << endl << i.second << endl << endl;

  cerr << "---------------------" << endl
       << "joy active:"  << endl;
  for(const auto& i: mySticks)
    cerr << i.first << ": " << *i.second << endl;
  cerr << "---------------------------------------------------------" << endl << endl << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::JoystickHandler::add(StellaJoystick* stick)
{
  // Skip if we couldn't open it for any reason
  if(stick->ID < 0)
    return false;

  // Figure out what type of joystick this is
  bool specialAdaptor = false;

  if(BSPF_containsIgnoreCase(stick->name, "2600-daptor"))
  {
    // 2600-daptorII devices have 3 axes and 12 buttons, and the value of the z-axis
    // determines how those 12 buttons are used (not all buttons are used in all modes)
    if(stick->numAxes == 3)
    {
      // TODO - stubbed out for now, until we find a way to reliably get info
      //        from the Z axis
      stick->name = "2600-daptor II";
    }
    else
      stick->name = "2600-daptor";

    specialAdaptor = true;
  }
  else if(BSPF_containsIgnoreCase(stick->name, "Stelladaptor"))
  {
    stick->name = "Stelladaptor";
    specialAdaptor = true;
  }
  else
  {
    // We need unique names for mappable devices
    // For non-unique names that already have a database entry,
    // we append ' #x', where 'x' increases consecutively
    int count = 0;
    for(const auto& i: myDatabase)
    {
      if(BSPF_startsWithIgnoreCase(i.first, stick->name) && i.second.joy)
      {
        ++count;
        break;
      }
    }
    if(count > 0)
    {
      ostringstream name;
      name << stick->name << " #" << count+1;
      stick->name = name.str();
    }
    stick->type = StellaJoystick::JT_REGULAR;
  }
  // The stick *must* be inserted here, since it may be used below
  mySticks[stick->ID] = stick;

  // Map the stelladaptors we've found according to the specified ports
  if(specialAdaptor)
    mapStelladaptors(myOSystem.settings().getString("saport"));

  // Add stick to database
  auto it = myDatabase.find(stick->name);
  if(it != myDatabase.end())    // already present
  {
    it->second.joy = stick;
    stick->setMap(it->second.mapping);
  }
  else    // adding for the first time
  {
    StickInfo info("", stick);
    myDatabase.insert(make_pair(stick->name, info));
    setStickDefaultMapping(stick->ID, Event::NoType, kEmulationMode);
    setStickDefaultMapping(stick->ID, Event::NoType, kMenuMode);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::JoystickHandler::remove(int id)
{
  // When a joystick is removed, we delete the actual joystick object but
  // remember its mapping, since it will eventually be saved to settings

  // Sticks that are removed must have initially been added
  // So we use the 'active' joystick list to access them
  try
  {
    StellaJoystick* stick = mySticks.at(id);

    auto it = myDatabase.find(stick->name);
    if(it != myDatabase.end() && it->second.joy == stick)
    {
      ostringstream buf;
      buf << "Removed joystick " << mySticks[id]->ID << ":" << endl
          << "  " << mySticks[id]->about() << endl;
      myOSystem.logMessage(buf.str(), 1);

      // Remove joystick, but remember mapping
      it->second.mapping = stick->getMap();
      delete it->second.joy;  it->second.joy = nullptr;
      mySticks.erase(id);

      return true;
    }
  }
  catch(std::out_of_range)
  {
    // fall through to indicate remove failed
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::JoystickHandler::remove(const string& name)
{
  auto it = myDatabase.find(name);
  if(it != myDatabase.end() && it->second.joy == nullptr)
  {
    myDatabase.erase(it);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::mapStelladaptors(const string& saport)
{
  // saport will have two values:
  //   'lr' means treat first valid adaptor as left port, second as right port
  //   'rl' means treat first valid adaptor as right port, second as left port
  // We know there will be only two such devices (at most), since the logic
  // in setupJoysticks take care of that
  int saCount = 0;
  int saOrder[2] = { 1, 2 };
  if(BSPF_equalsIgnoreCase(saport, "rl"))
  {
    saOrder[0] = 2; saOrder[1] = 1;
  }

  for(auto& stick: mySticks)
  {
    if(BSPF_startsWithIgnoreCase(stick.second->name, "Stelladaptor"))
    {
      if(saOrder[saCount] == 1)
      {
        stick.second->name += " (emulates left joystick port)";
        stick.second->type = StellaJoystick::JT_STELLADAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        stick.second->name += " (emulates right joystick port)";
        stick.second->type = StellaJoystick::JT_STELLADAPTOR_RIGHT;
      }
      saCount++;
    }
    else if(BSPF_startsWithIgnoreCase(stick.second->name, "2600-daptor"))
    {
      if(saOrder[saCount] == 1)
      {
        stick.second->name += " (emulates left joystick port)";
        stick.second->type = StellaJoystick::JT_2600DAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        stick.second->name += " (emulates right joystick port)";
        stick.second->type = StellaJoystick::JT_2600DAPTOR_RIGHT;
      }
      saCount++;
    }
  }
  myOSystem.settings().setValue("saport", saport);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  eraseMapping(event, mode);
  for(auto& i: mySticks)
    setStickDefaultMapping(i.first, event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::setStickDefaultMapping(int stick,
        Event::Type event, EventMode mode)
{
  EventHandler& handler = myOSystem.eventHandler();
  bool eraseAll = (event == Event::NoType);

  auto setDefaultAxis = [&](int a_stick, int a_axis, int a_value, Event::Type a_event)
  {
    if(eraseAll || a_event == event)
      handler.addJoyAxisMapping(a_event, mode, a_stick, a_axis, a_value, false);
  };
  auto setDefaultBtn = [&](int b_stick, int b_button, Event::Type b_event)
  {
    if(eraseAll || b_event == event)
      handler.addJoyButtonMapping(b_event, mode, b_stick, b_button, false);
  };
  auto setDefaultHat = [&](int h_stick, int h_hat, int h_dir, Event::Type h_event)
  {
    if(eraseAll || h_event == event)
      handler.addJoyHatMapping(h_event, mode, h_stick, h_hat, h_dir, false);
  };

  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      if(stick == 0)
      {
        // Left joystick left/right directions (assume joystick zero)
        setDefaultAxis( 0, 0, 0, Event::JoystickZeroLeft  );
        setDefaultAxis( 0, 0, 1, Event::JoystickZeroRight );
        // Left joystick up/down directions (assume joystick zero)
        setDefaultAxis( 0, 1, 0, Event::JoystickZeroUp    );
        setDefaultAxis( 0, 1, 1, Event::JoystickZeroDown  );
        // Left joystick (assume joystick zero, button zero)
        setDefaultBtn( 0, 0, Event::JoystickZeroFire );
        // Left joystick left/right directions (assume joystick zero and hat 0)
        setDefaultHat( 0, 0, EVENT_HATLEFT,  Event::JoystickZeroLeft  );
        setDefaultHat( 0, 0, EVENT_HATRIGHT, Event::JoystickZeroRight );
        // Left joystick up/down directions (assume joystick zero and hat 0)
        setDefaultHat( 0, 0, EVENT_HATUP,    Event::JoystickZeroUp    );
        setDefaultHat( 0, 0, EVENT_HATDOWN,  Event::JoystickZeroDown  );
      }
      else if(stick == 1)
      {
        // Right joystick left/right directions (assume joystick one)
        setDefaultAxis( 1, 0, 0, Event::JoystickOneLeft  );
        setDefaultAxis( 1, 0, 1, Event::JoystickOneRight );
        // Right joystick left/right directions (assume joystick one)
        setDefaultAxis( 1, 1, 0, Event::JoystickOneUp    );
        setDefaultAxis( 1, 1, 1, Event::JoystickOneDown  );
        // Right joystick (assume joystick one, button zero)
        setDefaultBtn( 1, 0, Event::JoystickOneFire );
        // Right joystick left/right directions (assume joystick one and hat 0)
        setDefaultHat( 1, 0, EVENT_HATLEFT,  Event::JoystickOneLeft  );
        setDefaultHat( 1, 0, EVENT_HATRIGHT, Event::JoystickOneRight );
        // Right joystick up/down directions (assume joystick one and hat 0)
        setDefaultHat( 1, 0, EVENT_HATUP,    Event::JoystickOneUp    );
        setDefaultHat( 1, 0, EVENT_HATDOWN,  Event::JoystickOneDown  );
      }
      break;

    case kMenuMode:  // Default menu/UI events
      if(stick == 0)
      {
        setDefaultAxis( 0, 0, 0, Event::UILeft  );
        setDefaultAxis( 0, 0, 1, Event::UIRight );
        setDefaultAxis( 0, 1, 0, Event::UIUp    );
        setDefaultAxis( 0, 1, 1, Event::UIDown  );

        // Left joystick (assume joystick zero, button zero)
        setDefaultBtn( 0, 0, Event::UISelect );

        setDefaultHat( 0, 0, EVENT_HATLEFT,  Event::UILeft  );
        setDefaultHat( 0, 0, EVENT_HATRIGHT, Event::UIRight );
        setDefaultHat( 0, 0, EVENT_HATUP,    Event::UIUp    );
        setDefaultHat( 0, 0, EVENT_HATDOWN,  Event::UIDown  );
      }
      break;

    default:
      break;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    for(auto& stick: mySticks)
      stick.second->eraseMap(mode);          // erase all events
  }
  else
  {
    for(auto& stick: mySticks)
      stick.second->eraseEvent(event, mode); // only reset the specific event
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::saveMapping()
{
  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  ostringstream joybuf;
  joybuf << Event::LastType;

  for(const auto& i: myDatabase)
  {
    const string& map = i.second.joy ? i.second.joy->getMap() : i.second.mapping;
    if(map != "")
      joybuf << "^" << map;
  }
  myOSystem.settings().setValue("joymap", joybuf.str());
}

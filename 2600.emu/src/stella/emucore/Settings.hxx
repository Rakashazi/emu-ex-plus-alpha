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
// $Id: Settings.hxx 3131 2015-01-01 03:49:32Z stephena $
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

class OSystem;

#include "Variant.hxx"
#include "bspf.hxx"

/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
  @version $Id: Settings.hxx 3131 2015-01-01 03:49:32Z stephena $
*/
class Settings
{
  friend class OSystem;

  public:
    /**
      Create a new settings abstract class
    */
    Settings(OSystem& osystem);

    /**
      Destructor
    */
    virtual ~Settings();

  public:
    /**
      This method should be called to load the arguments from the commandline.

      @return Name of the ROM to load, otherwise empty string
    */
    string loadCommandLine(int argc, char** argv);

    /**
      This method should be called *after* settings have been read,
      to validate (and change, if necessary) any improper settings.
    */
    void validate();

    /**
      This method should be called to display usage information.
    */
    void usage() const;

    /**
      Get the value assigned to the specified key.

      @param key The key of the setting to lookup
      @return The (variant) value of the setting
    */
    const Variant& value(const string& key) const;

    /**
      Set the value associated with the specified key.

      @param key   The key of the setting
      @param value The (variant) value to assign to the setting
    */
    void setValue(const string& key, const Variant& value);

    /**
      Convenience methods to return specific types.

      @param key The key of the setting to lookup
      @return The specific type value of the setting
    */
    int getInt(const string& key) const     { return value(key).toInt();   }
    float getFloat(const string& key) const { return value(key).toFloat(); }
    bool getBool(const string& key) const   { return value(key).toBool();  }
    const string& getString(const string& key) const { return value(key).toString(); }
    const GUI::Size getSize(const string& key) const { return value(key).toSize();   }

  protected:
    /**
      This method will be called to load the current settings from an rc file.
    */
    virtual void loadConfig();

    /**
      This method will be called to save the current settings to an rc file.
    */
    virtual void saveConfig();

  private:
    // Copy constructor and assignment operator not supported
    Settings(const Settings&);
    Settings& operator = (const Settings&);

    // Trim leading and following whitespace from a string
    static string trim(string& str)
    {
      string::size_type first = str.find_first_not_of(' ');
      return (first == string::npos) ? EmptyString :
              str.substr(first, str.find_last_not_of(' ')-first+1);
    }

  protected:
    // The parent OSystem object
    OSystem& myOSystem;

    // Structure used for storing settings
    struct Setting
    {
      string key;
      Variant value;
      Variant initialValue;
    };
    using SettingsArray = vector<Setting>;

    const SettingsArray& getInternalSettings() const
      { return myInternalSettings; }
    const SettingsArray& getExternalSettings() const
      { return myExternalSettings; }

    /** Get position in specified array of 'key' */
    int getInternalPos(const string& key) const;
    int getExternalPos(const string& key) const;

    /** Add key,value pair to specified array at specified position */
    int setInternal(const string& key, const Variant& value,
                    int pos = -1, bool useAsInitial = false);
    int setExternal(const string& key, const Variant& value,
                    int pos = -1, bool useAsInitial = false);

  private:
    // Holds key,value pairs that are necessary for Stella to
    // function and must be saved on each program exit.
    SettingsArray myInternalSettings;

    // Holds auxiliary key,value pairs that shouldn't be saved on
    // program exit.
    SettingsArray myExternalSettings;
};

#endif

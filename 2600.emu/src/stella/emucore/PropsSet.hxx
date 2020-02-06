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

#ifndef PROPERTIES_SET_HXX
#define PROPERTIES_SET_HXX

#include <map>

class FilesystemNode;
class OSystem;

#include "bspf.hxx"
#include "Props.hxx"

/**
  This class maintains an ordered collection of properties, maintained
  in a C++ map and accessible by ROM md5.  The md5 is used since this is
  the attribute which must be present in each entry in stella.pro
  and least likely to change.  A change in MD5 would mean a change in
  the game rom image (essentially a different game) and this would
  necessitate a new entry in the stella.pro file anyway.

  @author  Stephen Anthony
*/
class PropertiesSet
{
  public:
    /**
      Trivial constructor.
    */
   PropertiesSet() = default;

    /**
      Load properties from the specified file, and create an internal
      searchable list.

      @param filename  Full pathname of input file to use
    */
    void load(const string& filename);

    /**
      Save properties to the specified file.

      @param filename  Full pathname of output file to use

      @return  True on success, false on failure or save not needed
               Failure occurs if file couldn't be opened for writing,
               or if the file doesn't exist and a zero-byte file
               would be created
    */
    bool save(const string& filename) const;

    /**
      Get the property from the set with the given MD5.

      @param md5         The md5 of the property to get
      @param properties  The properties with the given MD5, or the default
                         properties if not found
      @param useDefaults  Use the built-in defaults, ignoring any properties
                          from an external file

      @return  True if the set with the specified md5 was found, else false
    */
    bool getMD5(const string& md5, Properties& properties,
                bool useDefaults = false) const;

    /**
      Get the property from the set with the given MD5, at the same time
      checking if it exists.  If it doesn't, insert a temporary copy into
      the set.

      @param rom         The ROM file used to calculate the MD5
      @param md5         The md5 of the property to get
      @param properties  The properties with the given MD5, or the default
                         properties if not found
    */
    void getMD5WithInsert(const FilesystemNode& rom, const string& md5,
                          Properties& properties);

    /**
      Insert the properties into the set.  If a duplicate is inserted
      the old properties are overwritten with the new ones.

      @param properties  The collection of properties
      @param save        Indicates whether the properties should be saved
                         when the program exits
    */
    void insert(const Properties& properties, bool save = true);

    /**
      Prints the contents of the PropertiesSet as a flat file.
    */
    void print() const;

  private:
    using PropsList = std::map<string, Properties>;

    // The properties read from an external 'stella.pro' file
    PropsList myExternalProps;

    // The properties temporarily inserted by the program, which should
    // be discarded when the program ends
    PropsList myTempProps;

  private:
    // Following constructors and assignment operators not supported
    PropertiesSet(const PropertiesSet&) = delete;
    PropertiesSet(PropertiesSet&&) = delete;
    PropertiesSet& operator=(const PropertiesSet&) = delete;
    PropertiesSet& operator=(PropertiesSet&&) = delete;
};

#endif

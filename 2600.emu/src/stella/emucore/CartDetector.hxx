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

#ifndef CARTRIDGE_DETECTOR_HXX
#define CARTRIDGE_DETECTOR_HXX

class Cartridge;
class Properties;

#include "Bankswitch.hxx"
#include "bspf.hxx"
#include "Settings.hxx"

/**
  Auto-detect cart type based on various attributes (file size, signatures,
  filenames, etc)

  @author  Stephen Anthony
*/
class CartDetector
{
  public:
    /**
      Create a new cartridge object allocated on the heap.  The
      type of cartridge created depends on the properties object.

      @param image    A pointer to the ROM image
      @param size     The size of the ROM image
      @param md5      The md5sum for the given ROM image (can be updated)
      @param dtype    The detected bankswitch type of the ROM image
      @param settings The settings container
      @return   Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge> create(const FilesystemNode& file,
                 const ByteBuffer& image, size_t size, string& md5,
                 const string& dtype, Settings& settings);

    /**
      Try to auto-detect the bankswitching type of the cartridge

      @param image  A pointer to the ROM image
      @param size   The size of the ROM image

      @return The "best guess" for the cartridge type
    */
    static Bankswitch::Type autodetectType(const ByteBuffer& image, size_t size);

  private:
    /**
      Create a cartridge from a multi-cart image pointer; internally this
      takes a slice of the ROM image ues that for the cartridge.

      @param image    A pointer to the complete ROM image
      @param size     The size of the ROM image slice
      @param numroms  The number of ROMs in the multicart
      @param md5      The md5sum for the slice of the ROM image
      @param type     The detected type of the slice of the ROM image
      @param id       The ID for the slice of the ROM image
      @param settings The settings container

      @return  Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge>
      createFromMultiCart(const ByteBuffer& image, size_t& size,
        uInt32 numroms, string& md5, Bankswitch::Type type, string& id,
        Settings& settings);

    /**
      Create a cartridge from the entire image pointer.

      @param image    A pointer to the complete ROM image
      @param size     The size of the ROM image
      @param type     The bankswitch type of the ROM image
      @param md5      The md5sum for the ROM image
      @param settings The settings container

      @return  Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge>
      createFromImage(const ByteBuffer& image, size_t size, Bankswitch::Type type,
                      const string& md5, Settings& settings);

    /**
      Search the image for the specified byte signature

      @param image      A pointer to the ROM image
      @param imagesize  The size of the ROM image
      @param signature  The byte sequence to search for
      @param sigsize    The number of bytes in the signature
      @param minhits    The minimum number of times a signature is to be found

      @return  True if the signature was found at least 'minhits' time, else false
    */
    static bool searchForBytes(const uInt8* image, size_t imagesize,
                               const uInt8* signature, uInt32 sigsize,
                               uInt32 minhits);

    /**
      Returns true if the image is probably a SuperChip (128 bytes RAM)
      Note: should be called only on ROMs with size multiple of 4K
    */
    static bool isProbablySC(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image probably contains ARM code in the first 1K
    */
    static bool isProbablyARM(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 0840 bankswitching cartridge
    */
    static bool isProbably0840(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 3E bankswitching cartridge
    */
    static bool isProbably3E(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 3E+ bankswitching cartridge
    */
    static bool isProbably3EPlus(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 3F bankswitching cartridge
    */
    static bool isProbably3F(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 4A50 bankswitching cartridge
    */
    static bool isProbably4A50(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a 4K SuperChip (128 bytes RAM)
    */
    static bool isProbably4KSC(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a BF/BFSC bankswitching cartridge
    */
    static bool isProbablyBF(const ByteBuffer& image, size_t size, Bankswitch::Type& type);

    /**
      Returns true if the image is probably a BUS bankswitching cartridge
    */
    static bool isProbablyBUS(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a CDF bankswitching cartridge
    */
    static bool isProbablyCDF(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a CTY bankswitching cartridge
    */
    static bool isProbablyCTY(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a CV bankswitching cartridge
    */
    static bool isProbablyCV(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a CV+ bankswitching cartridge
    */
    static bool isProbablyCVPlus(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a DASH bankswitching cartridge
    */
    static bool isProbablyDASH(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a DF/DFSC bankswitching cartridge
    */
    static bool isProbablyDF(const ByteBuffer& image, size_t size, Bankswitch::Type& type);

    /**
      Returns true if the image is probably a DPC+ bankswitching cartridge
    */
    static bool isProbablyDPCplus(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a E0 bankswitching cartridge
    */
    static bool isProbablyE0(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a E7 bankswitching cartridge
    */
    static bool isProbablyE7(const ByteBuffer& image, size_t size);

    /**
    Returns true if the image is probably a E78K bankswitching cartridge
    */
    static bool isProbablyE78K(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably an EF/EFSC bankswitching cartridge
    */
    static bool isProbablyEF(const ByteBuffer& image, size_t size, Bankswitch::Type& type);

    /**
      Returns true if the image is probably an F6 bankswitching cartridge
    */
    //static bool isProbablyF6(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably an FA2 bankswitching cartridge
    */
    static bool isProbablyFA2(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably an FC bankswitching cartridge
    */
    static bool isProbablyFC(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably an FE bankswitching cartridge
    */
    static bool isProbablyFE(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a MDM bankswitching cartridge
    */
    static bool isProbablyMDM(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a SB bankswitching cartridge
    */
    static bool isProbablySB(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a UA bankswitching cartridge
    */
    static bool isProbablyUA(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably a Wickstead Design bankswitching cartridge
    */
    static bool isProbablyWD(const ByteBuffer& image, size_t size);

    /**
      Returns true if the image is probably an X07 bankswitching cartridge
    */
    static bool isProbablyX07(const ByteBuffer& image, size_t size);

  private:
    // Following constructors and assignment operators not supported
    CartDetector() = delete;
    CartDetector(const CartDetector&) = delete;
    CartDetector(CartDetector&&) = delete;
    CartDetector& operator=(const CartDetector&) = delete;
    CartDetector& operator=(CartDetector&&) = delete;
};

#endif

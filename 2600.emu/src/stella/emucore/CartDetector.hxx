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

#ifndef CARTRIDGE_DETECTOR_HXX
#define CARTRIDGE_DETECTOR_HXX

class Cartridge;
class Properties;
class OSystem;

#include "bspf.hxx"
#include "BSType.hxx"

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
      @param system   The osystem associated with the system
      @return   Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge> create(const BytePtr& image, uInt32 size,
                 string& md5, const string& dtype, const OSystem& system);

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
      @param osystem  The osystem associated with the system

      @return  Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge>
      createFromMultiCart(const BytePtr& image, uInt32& size,
        uInt32 numroms, string& md5, BSType type, string& id,
        const OSystem& osystem);

    /**
      Create a cartridge from the entire image pointer.

      @param image    A pointer to the complete ROM image
      @param size     The size of the ROM image
      @param type     The bankswitch type of the ROM image
      @param md5      The md5sum for the ROM image
      @param osystem  The osystem associated with the system

      @return  Pointer to the new cartridge object allocated on the heap
    */
    static unique_ptr<Cartridge>
      createFromImage(const BytePtr& image, uInt32 size, BSType type,
                      const string& md5, const OSystem& osystem);

    /**
      Try to auto-detect the bankswitching type of the cartridge

      @param image  A pointer to the ROM image
      @param size   The size of the ROM image

      @return The "best guess" for the cartridge type
    */
    static BSType autodetectType(const BytePtr& image, uInt32 size);

    /**
      Search the image for the specified byte signature

      @param image      A pointer to the ROM image
      @param imagesize  The size of the ROM image
      @param signature  The byte sequence to search for
      @param sigsize    The number of bytes in the signature
      @param minhits    The minimum number of times a signature is to be found

      @return  True if the signature was found at least 'minhits' time, else false
    */
    static bool searchForBytes(const uInt8* image, uInt32 imagesize,
                               const uInt8* signature, uInt32 sigsize,
                               uInt32 minhits);

    /**
      Returns true if the image is probably a SuperChip (128 bytes RAM)
      Note: should be called only on ROMs with size multiple of 4K
    */
    static bool isProbablySC(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 4K SuperChip (128 bytes RAM)
    */
    static bool isProbably4KSC(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image probably contains ARM code in the first 1K
    */
    static bool isProbablyARM(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 0840 bankswitching cartridge
    */
    static bool isProbably0840(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 3E bankswitching cartridge
    */
    static bool isProbably3E(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 3E+ bankswitching cartridge
    */
    static bool isProbably3EPlus(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 3F bankswitching cartridge
    */
    static bool isProbably3F(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a 4A50 bankswitching cartridge
    */
    static bool isProbably4A50(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a BF/BFSC bankswitching cartridge
    */
    static bool isProbablyBF(const BytePtr& image, uInt32 size, BSType& type);

    /**
      Returns true if the image is probably a BUS bankswitching cartridge
    */
    static bool isProbablyBUS(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a CDF bankswitching cartridge
    */
    static bool isProbablyCDF(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a CTY bankswitching cartridge
    */
    static bool isProbablyCTY(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a CV bankswitching cartridge
    */
    static bool isProbablyCV(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a CV+ bankswitching cartridge
    */
    static bool isProbablyCVPlus(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a DASH bankswitching cartridge
    */
    static bool isProbablyDASH(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a DF/DFSC bankswitching cartridge
    */
    static bool isProbablyDF(const BytePtr& image, uInt32 size, BSType& type);

    /**
      Returns true if the image is probably a DPC+ bankswitching cartridge
    */
    static bool isProbablyDPCplus(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a E0 bankswitching cartridge
    */
    static bool isProbablyE0(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a E7 bankswitching cartridge
    */
    static bool isProbablyE7(const BytePtr& image, uInt32 size);

    /**
    Returns true if the image is probably a E78K bankswitching cartridge
    */
    static bool isProbablyE78K(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably an EF/EFSC bankswitching cartridge
    */
    static bool isProbablyEF(const BytePtr& image, uInt32 size, BSType& type);

    /**
      Returns true if the image is probably an F6 bankswitching cartridge
    */
    //static bool isProbablyF6(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably an FA2 bankswitching cartridge
    */
    static bool isProbablyFA2(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably an FE bankswitching cartridge
    */
    static bool isProbablyFE(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a MDM bankswitching cartridge
    */
    static bool isProbablyMDM(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a SB bankswitching cartridge
    */
    static bool isProbablySB(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably a UA bankswitching cartridge
    */
    static bool isProbablyUA(const BytePtr& image, uInt32 size);

    /**
      Returns true if the image is probably an X07 bankswitching cartridge
    */
    static bool isProbablyX07(const BytePtr& image, uInt32 size);

  private:
    // Following constructors and assignment operators not supported
    CartDetector() = delete;
    CartDetector(const CartDetector&) = delete;
    CartDetector(CartDetector&&) = delete;
    CartDetector& operator=(const CartDetector&) = delete;
    CartDetector& operator=(CartDetector&&) = delete;
};

#endif

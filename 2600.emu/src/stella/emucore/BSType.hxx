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

#ifndef BSTYPE_HXX
#define BSTYPE_HXX

#include "bspf.hxx"

// Currently supported bankswitch schemes
enum class BSType {
  _AUTO,  _0840,    _2IN1,  _4IN1,   _8IN1,   _16IN1, _32IN1,
  _64IN1, _128IN1,  _2K,    _3E,     _3EP,    _3F,    _4A50,
  _4K,    _4KSC,    _AR,    _BF,     _BFSC,   _BUS,   _CDF,
  _CM,    _CTY,     _CV,    _CVP,    _DASH,   _DF,    _DFSC,
  _DPC,   _DPCP,    _E0,    _E7,     _E78K,   _EF,     _EFSC,
  _F0,    _F4,    _F4SC,    _F6,    _F6SC,   _F8,     _F8SC,
  _FA,    _FA2,   _FE,      _MDM,   _SB,     _UA,     _WD,
  _X07,
#ifdef CUSTOM_ARM
  _CUSTOM,
#endif
  NumSchemes
};

// Info about the various bankswitch schemes, useful for displaying
// in GUI dropdown boxes, etc
struct BSDescription {
  const char* const name;
  const char* const desc;
};

static BSDescription BSList[int(BSType::NumSchemes)] = {
  { "AUTO",     "Auto-detect"                   },
  { "0840",     "0840 (8K ECONObank)"           },
  { "2IN1",     "2IN1 Multicart (4-32K)"        },
  { "4IN1",     "4IN1 Multicart (8-32K)"        },
  { "8IN1",     "8IN1 Multicart (16-64K)"       },
  { "16IN1",    "16IN1 Multicart (32-128K)"     },
  { "32IN1",    "32IN1 Multicart (64/128K)"     },
  { "64IN1",    "64IN1 Multicart (128/256K)"    },
  { "128IN1",   "128IN1 Multicart (256/512K)"   },
  { "2K",       "2K (64-2048 bytes Atari)"      },
  { "3E",       "3E (32K Tigervision)"          },
  { "3E+",      "3E+ (TJ modified DASH)"        },
  { "3F",       "3F (512K Tigervision)"         },
  { "4A50",     "4A50 (64K 4A50 + ram)"         },
  { "4K",       "4K (4K Atari)"                 },
  { "4KSC",     "4KSC (CPUWIZ 4K + ram)"        },
  { "AR",       "AR (Supercharger)"             },
  { "BF",       "BF (CPUWIZ 256K)"              },
  { "BFSC",     "BFSC (CPUWIZ 256K + ram)"      },
  { "BUS",      "BUS (Experimental)"            },
  { "CDF",      "CDF (Chris, Darrell, Fred)"    },
  { "CM",       "CM (SpectraVideo CompuMate)"   },
  { "CTY",      "CTY (CDW - Chetiry)"           },
  { "CV",       "CV (Commavid extra ram)"       },
  { "CV+",      "CV+ (Extended Commavid)"       },
  { "DASH",     "DASH (Experimental)"           },
  { "DF",       "DF (CPUWIZ 128K)"              },
  { "DFSC",     "DFSC (CPUWIZ 128K + ram)"      },
  { "DPC",      "DPC (Pitfall II)"              },
  { "DPC+",     "DPC+ (Enhanced DPC)"           },
  { "E0",       "E0 (8K Parker Bros)"           },
  { "E7",       "E7 (16K M-network)"            },
  { "E78K",     "E78K (8K M-network)"           },
  { "EF",       "EF (64K H. Runner)"            },
  { "EFSC",     "EFSC (64K H. Runner + ram)"    },
  { "F0",       "F0 (Dynacom Megaboy)"          },
  { "F4",       "F4 (32K Atari)"                },
  { "F4SC",     "F4SC (32K Atari + ram)"        },
  { "F6",       "F6 (16K Atari)"                },
  { "F6SC",     "F6SC (16K Atari + ram)"        },
  { "F8",       "F8 (8K Atari)"                 },
  { "F8SC",     "F8SC (8K Atari + ram)"         },
  { "FA",       "FA (CBS RAM Plus)"             },
  { "FA2",      "FA2 (CBS RAM Plus 24/28K)"     },
  { "FE",       "FE (8K Decathlon)"             },
  { "MDM",      "MDM (Menu Driven Megacart)"    },
  { "SB",       "SB (128-256K SUPERbank)"       },
  { "UA",       "UA (8K UA Ltd.)"               },
  { "WD",       "WD (Experimental)"             },
  { "X07",      "X07 (64K AtariAge)"            },
#ifdef CUSTOM_ARM
  { "CUSTOM",   "CUSTOM (ARM)"                  }
#endif
};

class Bankswitch
{
  public:
    // Convert BSType enum to string
    static string typeToName(BSType type) { return BSList[int(type)].name; }

    // Convert string to BSType enum
    static BSType nameToType(const string& name)
    {
      for(int i = 0; i < int(BSType::NumSchemes); ++i)
        if(BSPF::equalsIgnoreCase(BSList[i].name, name))
          return BSType(i);

      return BSType::_AUTO;
    }

  private:
    // Following constructors and assignment operators not supported
    Bankswitch() = delete;
    Bankswitch(const Bankswitch&) = delete;
    Bankswitch(Bankswitch&&) = delete;
    Bankswitch& operator=(const Bankswitch&) = delete;
    Bankswitch& operator=(Bankswitch&&) = delete;
};

#endif

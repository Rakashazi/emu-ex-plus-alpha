/*
 * info.c - Info about the VICE project, including the GPL.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdlib.h>

#include "info.h"
#include "infocontrib.h"

#ifdef WINMIPS
const char *info_license_text[] = {
    "                 GNU GENERAL PUBLIC LICENSE\n",
    "                    Version 2, June 1991\n",
    "\n",
    " Copyright (C) 1989, 1991 Free Software Foundation, Inc. \n",
    "    59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n",
    "\n",
    " Everyone is permitted to copy and distribute verbatim copies of\n",
    " this license document, but changing it is not allowed.\n",
    "\n",
    "\n",
    "                         Preamble\n",
    "\n",
    "The licenses for most software are designed to take away your\n",
    "freedom to share and change it. By contrast, the GNU General\n",
    "Public License is intended to guarantee your freedom to share and\n",
    "change free software--to make sure the software is free for all\n",
    "its users.  This General Public License applies to most of the\n",
    "Free Software Foundation's software and to any other program\n",
    "whose authors commit to using it.  (Some other Free Software\n",
    "Foundation software is covered by the GNU Library General Public\n",
    "License instead.)  You can apply it to your programs, too.\n",
    "\n",
    "When we speak of free software, we are referring to freedom, not\n",
    "price.  Our General Public Licenses are designed to make sure\n",
    "that you have the freedom to distribute copies of free software\n",
    "(and charge for this service if you wish), that you receive\n",
    "source code or can get it if you want it, that you can change the\n",
    "software or use pieces of it in new free programs; and that you\n",
    "know you can do these things.\n",
    "\n",
    "To protect your rights, we need to make restrictions that forbid\n",
    "anyone to deny you these rights or to ask you to surrender the\n",
    "rights.  These restrictions translate to certain responsibilities\n",
    "for you if you distribute copies of the software, or if you\n",
    "modify it.\n",
    "\n",
    "For example, if you distribute copies of such a program, whether\n",
    "gratis or for a fee, you must give the recipients all the rights\n",
    "that you have.  You must make sure that they, too, receive or can\n",
    "get the source code.  And you must show them these terms so they\n",
    "know their rights.\n",
    "\n",
    "We protect your rights with two steps: (1) copyright the\n",
    "software, and (2) offer you this license which gives you legal\n",
    "permission to copy, distribute and/or modify the software.\n",
    "\n",
    "Also, for each author's protection and ours, we want to make\n",
    "certain that everyone understands that there is no warranty for\n",
    "this free software.  If the software is modified by someone else\n",
    "and passed on, we want its recipients to know that what they have\n",
    "is not the original, so that any problems introduced by others\n",
    "will not reflect on the original authors' reputations.\n",
    "\n",
    "Finally, any free program is threatened constantly by software\n",
    "patents.  We wish to avoid the danger that redistributors of a\n",
    "free program will individually obtain patent licenses, in effect\n",
    "making the program proprietary.  To prevent this, we have made it\n",
    "clear that any patent must be licensed for everyone's free use or\n",
    "not licensed at all.\n",
    "\n",
    "The precise terms and conditions for copying, distribution and\n",
    "modification follow.\n",
    "\n",
    "\n",
    "GNU GENERAL PUBLIC LICENSE TERMS AND CONDITIONS FOR\n",
    "COPYING, DISTRIBUTION AND MODIFICATION\n",
    "\n",
    "0. This License applies to any program or other work which\n",
    "contains a notice placed by the copyright holder saying it may be\n",
    "distributed under the terms of this General Public License.  The\n",
    "\"Program\", below, refers to any such program or work, and a\n",
    "\"work based on the Program\" means either the Program or any\n",
    "derivative work under copyright law: that is to say, a work\n",
    "containing the Program or a portion of it, either verbatim or\n",
    "with modifications and/or translated into another language.\n",
    "(Hereinafter, translation is included without limitation in the\n",
    "term \"modification\".)  Each licensee is addressed as \"you\".\n",
    "\n",
    "Activities other than copying, distribution and modification are\n",
    "not covered by this License; they are outside its scope.  The act\n",
    "of running the Program is not restricted, and the output from the\n",
    "Program is covered only if its contents constitute a work based\n",
    "on the Program (independent of having been made by running the\n",
    "Program).  Whether that is true depends on what the Program does.\n",
    "\n",
    "  1. You may copy and distribute verbatim copies of the Program's\n",
    "source code as you receive it, in any medium, provided that you\n",
    "conspicuously and appropriately publish on each copy an\n",
    "appropriate copyright notice and disclaimer of warranty; keep\n",
    "intact all the notices that refer to this License and to the\n",
    "absence of any warranty; and give any other recipients of the\n",
    "Program a copy of this License along with the Program.\n",
    "\n",
    "You may charge a fee for the physical act of transferring a copy,\n",
    "and you may at your option offer warranty protection in exchange\n",
    "for a fee.\n",
    "\n",
    "  2. You may modify your copy or copies of the Program or any\n",
    "portion of it, thus forming a work based on the Program, and copy\n",
    "and distribute such modifications or work under the terms of\n",
    "Section 1 above, provided that you also meet all of these\n",
    "conditions:\n",
    "\n",
    "    a) You must cause the modified files to carry prominent\n",
    "    notices stating that you changed the files and the date of\n",
    "    any change.\n",
    "\n",
    "    b) You must cause any work that you distribute or publish,\n",
    "    that in whole or in part contains or is derived from the\n",
    "    Program or any part thereof, to be licensed as a whole at no\n",
    "    charge to all third parties under the terms of this License.\n",
    "\n",
    "    c) If the modified program normally reads commands\n",
    "    interactively when run, you must cause it, when started\n",
    "    running for such interactive use in the most ordinary way, to\n",
    "    print or display an announcement including an appropriate\n",
    "    copyright notice and a notice that there is no warranty (or\n",
    "    else, saying that you provide a warranty) and that users may\n",
    "    redistribute the program under these conditions, and telling\n",
    "    the user how to view a copy of this License.  (Exception: if\n",
    "    the Program itself is interactive but does not normally print\n",
    "    such an announcement, your work based on the Program is not\n",
    "    required to print an announcement.)\n",
    "\n",
    "These requirements apply to the modified work as a whole.  If\n",
    "identifiable sections of that work are not derived from the\n",
    "Program, and can be reasonably considered independent and\n",
    "separate works in themselves, then this License, and its terms,\n",
    "do not apply to those sections when you distribute them as\n",
    "separate works.  But when you distribute the same sections as\n",
    "part of a whole which is a work based on the Program, the\n",
    "distribution of the whole must be on the terms of this License,\n",
    "whose permissions for other licensees extend to the entire whole,\n",
    "and thus to each and every part regardless of who wrote it.\n",
    "\n",
    "Thus, it is not the intent of this section to claim rights or\n",
    "contest your rights to work written entirely by you; rather, the\n",
    "intent is to exercise the right to control the distribution of\n",
    "derivative or collective works based on the Program.\n",
    "\n",
    "In addition, mere aggregation of another work not based on the\n",
    "Program with the Program (or with a work based on the\n",
    "Program) on a volume of a storage or distribution medium\n",
    "does not bring the other work under the scope of this License.\n",
    "\n",
    "  3. You may copy and distribute the Program (or a work based on\n",
    "it, under Section 2) in object code or executable form under the\n",
    "terms of Sections 1 and 2 above provided that you also do one of\n",
    "the following:\n",
    "\n",
    "    a) Accompany it with the complete corresponding\n",
    "    machine-readable source code, which must be distributed under\n",
    "    the terms of Sections 1 and 2 above on a medium customarily\n",
    "    used for software interchange; or,\n",
    "\n",
    "    b) Accompany it with a written offer, valid for at least\n",
    "    three years, to give any third party, for a charge no more\n",
    "    than your cost of physically performing source distribution,\n",
    "    a complete machine-readable copy of the corresponding source\n",
    "    code, to be distributed under the terms of Sections 1 and 2\n",
    "    above on a medium customarily used for software interchange;\n",
    "    or,\n",
    "\n",
    "    c) Accompany it with the information you received as to the\n",
    "    offer to distribute corresponding source code.  (This\n",
    "    alternative is allowed only for noncommercial distribution\n",
    "    and only if you received the program in object code or\n",
    "    executable form with such an offer, in accord with Subsection\n",
    "    b above.)\n",
    "\n",
    "The source code for a work means the preferred form of the work\n",
    "for making modifications to it.  For an executable work, complete\n",
    "source code means all the source code for all modules it\n",
    "contains, plus any associated interface definition files, plus\n",
    "the scripts used to control compilation and installation of the\n",
    "executable.  However, as a special exception, the source code\n",
    "distributed need not include anything that is normally\n",
    "distributed (in either source or binary form) with the major\n",
    "components (compiler, kernel, and so on) of the operating system\n",
    "on which the executable runs, unless that component itself\n",
    "accompanies the executable.\n",
    "\n",
    "If distribution of executable or object code is made by offering\n",
    "access to copy from a designated place, then offering equivalent\n",
    "access to copy the source code from the same place counts as\n",
    "distribution of the source code, even though third parties are\n",
    "not compelled to copy the source along with the object code.\n",
    "\n",
    "  4. You may not copy, modify, sublicense, or distribute the\n",
    "Program except as expressly provided under this License.  Any\n",
    "attempt otherwise to copy, modify, sublicense or distribute the\n",
    "Program is void, and will automatically terminate your rights\n",
    "under this License.  However, parties who have received copies,\n",
    "or rights, from you under this License will not have their\n",
    "licenses terminated so long as such parties remain in full\n",
    "compliance.\n",
    "\n",
    "  5. You are not required to accept this License, since you have\n",
    "not signed it.  However, nothing else grants you permission to\n",
    "modify or distribute the Program or its derivative works.  These\n",
    "actions are prohibited by law if you do not accept this License.\n",
    "Therefore, by modifying or distributing the Program (or any work\n",
    "based on the Program), you indicate your acceptance of this\n",
    "License to do so, and all its terms and conditions for copying,\n",
    "distributing or modifying the Program or works based on it.\n",
    "\n",
    "  6. Each time you redistribute the Program (or any work based on\n",
    "the Program), the recipient automatically receives a license from\n",
    "the original licensor to copy, distribute or modify the Program\n",
    "subject to these terms and conditions.  You may not impose any\n",
    "further restrictions on the recipients' exercise of the rights\n",
    "granted herein.  You are not responsible for enforcing compliance\n",
    "by third parties to this License.\n",
    "\n",
    "  7. If, as a consequence of a court judgment or allegation of\n",
    "patent infringement or for any other reason (not limited to\n",
    "patent issues), conditions are imposed on you (whether by court\n",
    "order, agreement or otherwise) that contradict the conditions of\n",
    "this License, they do not excuse you from the conditions of this\n",
    "License.  If you cannot distribute so as to satisfy\n",
    "simultaneously your obligations under this License and any other\n",
    "pertinent obligations, then as a consequence you may not\n",
    "distribute the Program at all.  For example, if a patent license\n",
    "would not permit royalty-free redistribution of the Program by\n",
    "all those who receive copies directly or indirectly through you,\n",
    "then the only way you could satisfy both it and this License\n",
    "would be to refrain entirely from distribution of the Program.\n",
    "\n",
    "If any portion of this section is held invalid or unenforceable\n",
    "under any particular circumstance, the balance of the section is\n",
    "intended to apply and the section as a whole is intended to apply\n",
    "in other circumstances.\n",
    "\n",
    "It is not the purpose of this section to induce you to infringe\n",
    "any patents or other property right claims or to contest validity\n",
    "of any such claims; this section has the sole purpose of\n",
    "protecting the integrity of the free software distribution\n",
    "system, which is implemented by public license practices.  Many\n",
    "people have made generous contributions to the wide range of\n",
    "software distributed through that system in reliance on\n",
    "consistent application of that system; it is up to the\n",
    "author/donor to decide if he or she is willing to distribute\n",
    "software through any other system and a licensee cannot impose\n",
    "that choice.\n",
    "\n",
    "This section is intended to make thoroughly clear what is\n",
    "believed to be a consequence of the rest of this License.\n",
    "\n",
    "  8. If the distribution and/or use of the Program is restricted\n",
    "in certain countries either by patents or by copyrighted\n",
    "interfaces, the original copyright holder who places the Program\n",
    "under this License may add an explicit geographical distribution\n",
    "limitation excluding those countries, so that distribution is\n",
    "permitted only in or among countries not thus excluded.  In such\n",
    "case, this License incorporates the limitation as if written in\n",
    "the body of this License.\n",
    "\n",
    "  9. The Free Software Foundation may publish revised and/or new\n",
    "versions of the General Public License from time to time.  Such\n",
    "new versions will be similar in spirit to the present version,\n",
    "but may differ in detail to address new problems or concerns.\n",
    "\n",
    "Each version is given a distinguishing version number.  If the\n",
    "Program specifies a version number of this License which applies\n",
    "to it and \"any later version\", you have the option of following\n",
    "the terms and conditions either of that version or of any later\n",
    "version published by the Free Software Foundation.  If the\n",
    "Program does not specify a version number of this License, you\n",
    "may choose any version ever published by the Free Software\n",
    "Foundation.\n",
    "\n",
    "  10. If you wish to incorporate parts of the Program into other\n",
    "free programs whose distribution conditions are different, write\n",
    "to the author to ask for permission.  For software which is\n",
    "copyrighted by the Free Software Foundation, write to the Free\n",
    "Software Foundation; we sometimes make exceptions for this.  Our\n",
    "decision will be guided by the two goals of preserving the free\n",
    "status of all derivatives of our free software and of promoting\n",
    "the sharing and reuse of software generally.\n",
    NULL
};
#else
const char info_license_text[] =
    "                 GNU GENERAL PUBLIC LICENSE\n"
    "                    Version 2, June 1991\n"
    "\n"
    " Copyright (C) 1989, 1991 Free Software Foundation, Inc. \n"
    "    59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
    "\n"
    " Everyone is permitted to copy and distribute verbatim copies of\n"
    " this license document, but changing it is not allowed.\n"
    "\n"
    "\n"
    "                         Preamble\n"
    "\n"
    "The licenses for most software are designed to take away your\n"
    "freedom to share and change it. By contrast, the GNU General\n"
    "Public License is intended to guarantee your freedom to share and\n"
    "change free software--to make sure the software is free for all\n"
    "its users.  This General Public License applies to most of the\n"
    "Free Software Foundation's software and to any other program\n"
    "whose authors commit to using it.  (Some other Free Software\n"
    "Foundation software is covered by the GNU Library General Public\n"
    "License instead.)  You can apply it to your programs, too.\n"
    "\n"
    "When we speak of free software, we are referring to freedom, not\n"
    "price.  Our General Public Licenses are designed to make sure\n"
    "that you have the freedom to distribute copies of free software\n"
    "(and charge for this service if you wish), that you receive\n"
    "source code or can get it if you want it, that you can change the\n"
    "software or use pieces of it in new free programs; and that you\n"
    "know you can do these things.\n"
    "\n"
    "To protect your rights, we need to make restrictions that forbid\n"
    "anyone to deny you these rights or to ask you to surrender the\n"
    "rights.  These restrictions translate to certain responsibilities\n"
    "for you if you distribute copies of the software, or if you\n"
    "modify it.\n"
    "\n"
    "For example, if you distribute copies of such a program, whether\n"
    "gratis or for a fee, you must give the recipients all the rights\n"
    "that you have.  You must make sure that they, too, receive or can\n"
    "get the source code.  And you must show them these terms so they\n"
    "know their rights.\n"
    "\n"
    "We protect your rights with two steps: (1) copyright the\n"
    "software, and (2) offer you this license which gives you legal\n"
    "permission to copy, distribute and/or modify the software.\n"
    "\n"
    "Also, for each author's protection and ours, we want to make\n"
    "certain that everyone understands that there is no warranty for\n"
    "this free software.  If the software is modified by someone else\n"
    "and passed on, we want its recipients to know that what they have\n"
    "is not the original, so that any problems introduced by others\n"
    "will not reflect on the original authors' reputations.\n"
    "\n"
    "Finally, any free program is threatened constantly by software\n"
    "patents.  We wish to avoid the danger that redistributors of a\n"
    "free program will individually obtain patent licenses, in effect\n"
    "making the program proprietary.  To prevent this, we have made it\n"
    "clear that any patent must be licensed for everyone's free use or\n"
    "not licensed at all.\n"
    "\n"
    "The precise terms and conditions for copying, distribution and\n"
    "modification follow.\n"
    "\n"
    "\n"
    "GNU GENERAL PUBLIC LICENSE TERMS AND CONDITIONS FOR\n"
    "COPYING, DISTRIBUTION AND MODIFICATION\n"
    "\n"
    "0. This License applies to any program or other work which\n"
    "contains a notice placed by the copyright holder saying it may be\n"
    "distributed under the terms of this General Public License.  The\n"
    "\"Program\", below, refers to any such program or work, and a\n"
    "\"work based on the Program\" means either the Program or any\n"
    "derivative work under copyright law: that is to say, a work\n"
    "containing the Program or a portion of it, either verbatim or\n"
    "with modifications and/or translated into another language.\n"
    "(Hereinafter, translation is included without limitation in the\n"
    "term \"modification\".)  Each licensee is addressed as \"you\".\n"
    "\n"
    "Activities other than copying, distribution and modification are\n"
    "not covered by this License; they are outside its scope.  The act\n"
    "of running the Program is not restricted, and the output from the\n"
    "Program is covered only if its contents constitute a work based\n"
    "on the Program (independent of having been made by running the\n"
    "Program).  Whether that is true depends on what the Program does.\n"
    "\n"
    "  1. You may copy and distribute verbatim copies of the Program's\n"
    "source code as you receive it, in any medium, provided that you\n"
    "conspicuously and appropriately publish on each copy an\n"
    "appropriate copyright notice and disclaimer of warranty; keep\n"
    "intact all the notices that refer to this License and to the\n"
    "absence of any warranty; and give any other recipients of the\n"
    "Program a copy of this License along with the Program.\n"
    "\n"
    "You may charge a fee for the physical act of transferring a copy,\n"
    "and you may at your option offer warranty protection in exchange\n"
    "for a fee.\n"
    "\n"
    "  2. You may modify your copy or copies of the Program or any\n"
    "portion of it, thus forming a work based on the Program, and copy\n"
    "and distribute such modifications or work under the terms of\n"
    "Section 1 above, provided that you also meet all of these\n"
    "conditions:\n"
    "\n"
    "    a) You must cause the modified files to carry prominent\n"
    "    notices stating that you changed the files and the date of\n"
    "    any change.\n"
    "\n"
    "    b) You must cause any work that you distribute or publish,\n"
    "    that in whole or in part contains or is derived from the\n"
    "    Program or any part thereof, to be licensed as a whole at no\n"
    "    charge to all third parties under the terms of this License.\n"
    "\n"
    "    c) If the modified program normally reads commands\n"
    "    interactively when run, you must cause it, when started\n"
    "    running for such interactive use in the most ordinary way, to\n"
    "    print or display an announcement including an appropriate\n"
    "    copyright notice and a notice that there is no warranty (or\n"
    "    else, saying that you provide a warranty) and that users may\n"
    "    redistribute the program under these conditions, and telling\n"
    "    the user how to view a copy of this License.  (Exception: if\n"
    "    the Program itself is interactive but does not normally print\n"
    "    such an announcement, your work based on the Program is not\n"
    "    required to print an announcement.)\n"
    "\n"
    "These requirements apply to the modified work as a whole.  If\n"
    "identifiable sections of that work are not derived from the\n"
    "Program, and can be reasonably considered independent and\n"
    "separate works in themselves, then this License, and its terms,\n"
    "do not apply to those sections when you distribute them as\n"
    "separate works.  But when you distribute the same sections as\n"
    "part of a whole which is a work based on the Program, the\n"
    "distribution of the whole must be on the terms of this License,\n"
    "whose permissions for other licensees extend to the entire whole,\n"
    "and thus to each and every part regardless of who wrote it.\n"
    "\n"
    "Thus, it is not the intent of this section to claim rights or\n"
    "contest your rights to work written entirely by you; rather, the\n"
    "intent is to exercise the right to control the distribution of\n"
    "derivative or collective works based on the Program.\n"
    "\n"
    "In addition, mere aggregation of another work not based on the\n"
    "Program with the Program (or with a work based on the\n"
    "Program) on a volume of a storage or distribution medium\n"
    "does not bring the other work under the scope of this License.\n"
    "\n"
    "  3. You may copy and distribute the Program (or a work based on\n"
    "it, under Section 2) in object code or executable form under the\n"
    "terms of Sections 1 and 2 above provided that you also do one of\n"
    "the following:\n"
    "\n"
    "    a) Accompany it with the complete corresponding\n"
    "    machine-readable source code, which must be distributed under\n"
    "    the terms of Sections 1 and 2 above on a medium customarily\n"
    "    used for software interchange; or,\n"
    "\n"
    "    b) Accompany it with a written offer, valid for at least\n"
    "    three years, to give any third party, for a charge no more\n"
    "    than your cost of physically performing source distribution,\n"
    "    a complete machine-readable copy of the corresponding source\n"
    "    code, to be distributed under the terms of Sections 1 and 2\n"
    "    above on a medium customarily used for software interchange;\n"
    "    or,\n"
    "\n"
    "    c) Accompany it with the information you received as to the\n"
    "    offer to distribute corresponding source code.  (This\n"
    "    alternative is allowed only for noncommercial distribution\n"
    "    and only if you received the program in object code or\n"
    "    executable form with such an offer, in accord with Subsection\n"
    "    b above.)\n"
    "\n"
    "The source code for a work means the preferred form of the work\n"
    "for making modifications to it.  For an executable work, complete\n"
    "source code means all the source code for all modules it\n"
    "contains, plus any associated interface definition files, plus\n"
    "the scripts used to control compilation and installation of the\n"
    "executable.  However, as a special exception, the source code\n"
    "distributed need not include anything that is normally\n"
    "distributed (in either source or binary form) with the major\n"
    "components (compiler, kernel, and so on) of the operating system\n"
    "on which the executable runs, unless that component itself\n"
    "accompanies the executable.\n"
    "\n"
    "If distribution of executable or object code is made by offering\n"
    "access to copy from a designated place, then offering equivalent\n"
    "access to copy the source code from the same place counts as\n"
    "distribution of the source code, even though third parties are\n"
    "not compelled to copy the source along with the object code.\n"
    "\n"
    "  4. You may not copy, modify, sublicense, or distribute the\n"
    "Program except as expressly provided under this License.  Any\n"
    "attempt otherwise to copy, modify, sublicense or distribute the\n"
    "Program is void, and will automatically terminate your rights\n"
    "under this License.  However, parties who have received copies,\n"
    "or rights, from you under this License will not have their\n"
    "licenses terminated so long as such parties remain in full\n"
    "compliance.\n"
    "\n"
    "  5. You are not required to accept this License, since you have\n"
    "not signed it.  However, nothing else grants you permission to\n"
    "modify or distribute the Program or its derivative works.  These\n"
    "actions are prohibited by law if you do not accept this License.\n"
    "Therefore, by modifying or distributing the Program (or any work\n"
    "based on the Program), you indicate your acceptance of this\n"
    "License to do so, and all its terms and conditions for copying,\n"
    "distributing or modifying the Program or works based on it.\n"
    "\n"
    "  6. Each time you redistribute the Program (or any work based on\n"
    "the Program), the recipient automatically receives a license from\n"
    "the original licensor to copy, distribute or modify the Program\n"
    "subject to these terms and conditions.  You may not impose any\n"
    "further restrictions on the recipients' exercise of the rights\n"
    "granted herein.  You are not responsible for enforcing compliance\n"
    "by third parties to this License.\n"
    "\n"
    "  7. If, as a consequence of a court judgment or allegation of\n"
    "patent infringement or for any other reason (not limited to\n"
    "patent issues), conditions are imposed on you (whether by court\n"
    "order, agreement or otherwise) that contradict the conditions of\n"
    "this License, they do not excuse you from the conditions of this\n"
    "License.  If you cannot distribute so as to satisfy\n"
    "simultaneously your obligations under this License and any other\n"
    "pertinent obligations, then as a consequence you may not\n"
    "distribute the Program at all.  For example, if a patent license\n"
    "would not permit royalty-free redistribution of the Program by\n"
    "all those who receive copies directly or indirectly through you,\n"
    "then the only way you could satisfy both it and this License\n"
    "would be to refrain entirely from distribution of the Program.\n"
    "\n"
    "If any portion of this section is held invalid or unenforceable\n"
    "under any particular circumstance, the balance of the section is\n"
    "intended to apply and the section as a whole is intended to apply\n"
    "in other circumstances.\n"
    "\n"
    "It is not the purpose of this section to induce you to infringe\n"
    "any patents or other property right claims or to contest validity\n"
    "of any such claims; this section has the sole purpose of\n"
    "protecting the integrity of the free software distribution\n"
    "system, which is implemented by public license practices.  Many\n"
    "people have made generous contributions to the wide range of\n"
    "software distributed through that system in reliance on\n"
    "consistent application of that system; it is up to the\n"
    "author/donor to decide if he or she is willing to distribute\n"
    "software through any other system and a licensee cannot impose\n"
    "that choice.\n"
    "\n"
    "This section is intended to make thoroughly clear what is\n"
    "believed to be a consequence of the rest of this License.\n"
    "\n"
    "  8. If the distribution and/or use of the Program is restricted\n"
    "in certain countries either by patents or by copyrighted\n"
    "interfaces, the original copyright holder who places the Program\n"
    "under this License may add an explicit geographical distribution\n"
    "limitation excluding those countries, so that distribution is\n"
    "permitted only in or among countries not thus excluded.  In such\n"
    "case, this License incorporates the limitation as if written in\n"
    "the body of this License.\n"
    "\n"
    "  9. The Free Software Foundation may publish revised and/or new\n"
    "versions of the General Public License from time to time.  Such\n"
    "new versions will be similar in spirit to the present version,\n"
    "but may differ in detail to address new problems or concerns.\n"
    "\n"
    "Each version is given a distinguishing version number.  If the\n"
    "Program specifies a version number of this License which applies\n"
    "to it and \"any later version\", you have the option of following\n"
    "the terms and conditions either of that version or of any later\n"
    "version published by the Free Software Foundation.  If the\n"
    "Program does not specify a version number of this License, you\n"
    "may choose any version ever published by the Free Software\n"
    "Foundation.\n"
    "\n"
    "  10. If you wish to incorporate parts of the Program into other\n"
    "free programs whose distribution conditions are different, write\n"
    "to the author to ask for permission.  For software which is\n"
    "copyrighted by the Free Software Foundation, write to the Free\n"
    "Software Foundation; we sometimes make exceptions for this.  Our\n"
    "decision will be guided by the two goals of preserving the free\n"
    "status of all derivatives of our free software and of promoting\n"
    "the sharing and reuse of software generally.\n";
#endif

/* Since the GNU licence text won't change and the SDL UI can
   sometimes only handle 40 chars it's easier to use a specially
   prepared text where needed.
 */
#ifdef USE_SDLUI
#ifdef WINMIPS
const char *info_license_text40[] = {
    "       GNU GENERAL PUBLIC LICENSE\n",
    "          Version 2, June 1991\n",
    "\n",
    " Copyright (C) 1989, 1991 Free Software\n",
    " Foundation, Inc.  59 Temple Place,\n",
    " Suite 330, Boston, MA  02111-1307  USA\n",
    "\n",
    " Everyone is permitted to copy and\n",
    " distribute verbatim copies of this\n",
    " license document, but changing it is\n",
    " not allowed.\n",
    "\n",
    "\n",
    "                Preamble\n",
    "\n",
    "The licenses for most software are\n",
    "designed to take away your freedom to\n",
    "share and change it. By contrast, the\n",
    "GNU General Public License is intended\n",
    "to guarantee your freedom to share and\n",
    "change free software--to make sure the\n",
    "software is free for all its users.\n",
    "This General Public License applies to\n",
    "most of the Free Software Foundation's\n",
    "software and to any other program whose\n",
    "authors commit to using it.  (Some other\n",
    "Free Software Foundation software is\n",
    "covered by the GNU Library General\n",
    "Public License instead.)  You can apply\n",
    "it to your programs, too.\n",
    "\n",
    "When we speak of free software, we are\n",
    "referring to freedom, not price.  Our\n",
    "General Public Licenses are designed to\n",
    "make sure that you have the freedom to\n",
    "distribute copies of free software (and\n",
    "charge for this service if you wish),\n",
    "that you receive source code or can get\n",
    "it if you want it, that you can change\n",
    "the software or use pieces of it in new\n",
    "free programs; and that you know you can\n",
    "do these things.\n",
    "\n",
    "To protect your rights, we need to make\n",
    "restrictions that forbid anyone to deny\n",
    "you these rights or to ask you to\n",
    "surrender the rights.  These\n",
    "restrictions translate to certain\n",
    "responsibilities for you if you\n",
    "distribute copies of the software, or if\n",
    "you modify it.\n",
    "\n",
    "For example, if you distribute copies of\n",
    "such a program, whether gratis or for a\n",
    "fee, you must give the recipients all\n",
    "the rights that you have.  You must make\n",
    "sure that they, too, receive or can get\n",
    "the source code.  And you must show them\n",
    "these terms so they know their rights.\n",
    "\n",
    "We protect your rights with two steps:\n",
    "(1) copyright the software, and (2)\n",
    "offer you this license which gives you\n",
    "legal permission to copy, distribute\n",
    "and/or modify the software.\n",
    "\n",
    "Also, for each author's protection and\n",
    "ours, we want to make certain that\n",
    "everyone understands that there is no\n",
    "warranty for this free software.  If the\n",
    "software is modified by someone else and\n",
    "passed on, we want its recipients to\n",
    "know that what they have is not the\n",
    "original, so that any problems\n",
    "introduced by others will not reflect on\n",
    "the original authors' reputations.\n",
    "\n",
    "Finally, any free program is threatened\n",
    "constantly by software patents.  We wish\n",
    "to avoid the danger that redistributors\n",
    "of a free program will individually\n",
    "obtain patent licenses, in effect making\n",
    "the program proprietary.  To prevent\n",
    "this, we have made it clear that any\n",
    "patent must be licensed for everyone's\n",
    "free use or not licensed at all.\n",
    "\n",
    "The precise terms and conditions for\n",
    "copying, distribution and modification\n",
    "follow.\n",
    "\n",
    "\n",
    "GNU GENERAL PUBLIC LICENSE TERMS AND\n",
    "CONDITIONS FOR COPYING, DISTRIBUTION AND\n",
    "MODIFICATION\n",
    "\n",
    "0. This License applies to any program\n",
    "or other work which contains a notice\n",
    "placed by the copyright holder saying it\n",
    "may be distributed under the terms of\n",
    "this General Public License.  The\n",
    "\"Program\", below, refers to any such\n",
    "program or work, and a \"work based on\n",
    "the Program\" means either the Program or\n",
    "any derivative work under copyright law:\n",
    "that is to say, a work containing the\n",
    "Program or a portion of it, either\n",
    "verbatim or with modifications and/or\n",
    "translated into another language.\n",
    "(Hereinafter, translation is included\n",
    "without limitation in the term\n",
    "\"modification\".)  Each licensee is\n",
    "addressed as \"you\".\n",
    "\n",
    "Activities other than copying,\n",
    "distribution and modification are not\n",
    "covered by this License; they are\n",
    "outside its scope.  The act of running\n",
    "the Program is not restricted, and the\n",
    "output from the Program is covered only\n",
    "if its contents constitute a work based\n",
    "on the Program (independent of having\n",
    "been made by running the Program).\n",
    "Whether that is true depends on what the\n",
    "Program does.\n",
    "\n",
    "  1. You may copy and distribute\n",
    "verbatim copies of the Program's source\n",
    "code as you receive it, in any medium,\n",
    "provided that you conspicuously and\n",
    "appropriately publish on each copy an\n",
    "appropriate copyright notice and\n",
    "disclaimer of warranty; keep intact all\n",
    "the notices that refer to this License\n",
    "and to the absence of any warranty; and\n",
    "give any other recipients of the Program\n",
    "a copy of this License along with the\n",
    "Program.\n",
    "\n",
    "You may charge a fee for the physical\n",
    "act of transferring a copy, and you may\n",
    "at your option offer warranty protection\n",
    "in exchange for a fee.\n",
    "\n",
    "  2. You may modify your copy or copies\n",
    "of the Program or any portion of it,\n",
    "thus forming a work based on the\n",
    "Program, and copy and distribute such\n",
    "modifications or work under the terms of\n",
    "Section 1 above, provided that you also\n",
    "meet all of these conditions:\n",
    "\n",
    "    a) You must cause the modified files\n",
    "    to carry prominent notices stating\n",
    "    that you changed the files and the\n",
    "    date of any change.\n",
    "\n",
    "    b) You must cause any work that you\n",
    "    distribute or publish, that in whole\n",
    "    or in part contains or is derived\n",
    "    from the Program or any part\n",
    "    thereof, to be licensed as a whole\n",
    "    at no charge to all third parties\n",
    "    under the terms of this License.\n",
    "\n",
    "    c) If the modified program normally\n",
    "    reads commands interactively when\n",
    "    run, you must cause it, when started\n",
    "    running for such interactive use in\n",
    "    the most ordinary way, to print or\n",
    "    display an announcement including an\n",
    "    appropriate copyright notice and a\n",
    "    notice that there is no warranty (or\n",
    "    else, saying that you provide a\n",
    "    warranty) and that users may\n",
    "    redistribute the program under these\n",
    "    conditions, and telling the user how\n",
    "    to view a copy of this License.\n",
    "    (Exception: if the Program itself is\n",
    "    interactive but does not normally\n",
    "    print such an announcement, your\n",
    "    work based on the Program is not\n",
    "    required to print an announcement.)\n",
    "\n",
    "These requirements apply to the modified\n",
    "work as a whole.  If identifiable\n",
    "sections of that work are not derived\n",
    "from the Program, and can be reasonably\n",
    "considered independent and separate\n",
    "works in themselves, then this License,\n",
    "and its terms, do not apply to those\n",
    "sections when you distribute them as\n",
    "separate works.  But when you distribute\n",
    "the same sections as part of a whole\n",
    "which is a work based on the Program,\n",
    "the distribution of the whole must be on\n",
    "the terms of this License, whose\n",
    "permissions for other licensees extend\n",
    "to the entire whole, and thus to each\n",
    "and every part regardless of who wrote\n",
    "it.\n",
    "\n",
    "Thus, it is not the intent of this\n",
    "section to claim rights or contest your\n",
    "rights to work written entirely by you;\n",
    "rather, the intent is to exercise the\n",
    "right to control the distribution of\n",
    "derivative or collective works based on\n",
    "the Program.\n",
    "\n",
    "In addition, mere aggregation of another\n",
    "work not based on the Program with the\n",
    "Program (or with a work based on the\n",
    "Program) on a volume of a storage or\n",
    "distribution medium does not bring the\n",
    "other work under the scope of this\n",
    "License.\n",
    "\n",
    "  3. You may copy and distribute the\n",
    "Program (or a work based on it, under\n",
    "Section 2) in object code or executable\n",
    "form under the terms of Sections 1 and 2\n",
    "above provided that you also do one of\n",
    "the following:\n",
    "\n",
    "    a) Accompany it with the complete\n",
    "    corresponding machine-readable\n",
    "    source code, which must be\n",
    "    distributed under the terms of\n",
    "    Sections 1 and 2 above on a medium\n",
    "    customarily used for software\n",
    "    interchange; or,\n",
    "\n",
    "    b) Accompany it with a written\n",
    "    offer, valid for at least three\n",
    "    years, to give any third party, for\n",
    "    a charge no more than your cost of\n",
    "    physically performing source\n",
    "    distribution, a complete machine-\n",
    "    readable copy of the corresponding\n",
    "    source code, to be distributed under\n",
    "    the terms of Sections 1 and 2 above\n",
    "    on a medium customarily used for\n",
    "    software interchange; or,\n",
    "\n",
    "    c) Accompany it with the information\n",
    "    you received as to the offer to\n",
    "    distribute corresponding source\n",
    "    code.  (This alternative is allowed\n",
    "    only for noncommercial distribution\n",
    "    and only if you received the program\n",
    "    in object code or executable form\n",
    "    with such an offer, in accord with\n",
    "    Subsection b above.)\n",
    "\n",
    "The source code for a work means the\n",
    "preferred form of the work for making\n",
    "modifications to it.  For an executable\n",
    "work, complete source code means all the\n",
    "source code for all modules it contains,\n",
    "plus any associated interface definition\n",
    "files, plus the scripts used to control\n",
    "compilation and installation of the\n",
    "executable.  However, as a special\n",
    "exception, the source code distributed\n",
    "need not include anything that is\n",
    "normally distributed (in either source\n",
    "or binary form) with the major\n",
    "components (compiler, kernel, and so on)\n",
    "of the operating system on which the\n",
    "executable runs, unless that component\n",
    "itself accompanies the executable.\n",
    "\n",
    "If distribution of executable or object\n",
    "code is made by offering access to copy\n",
    "from a designated place, then offering\n",
    "equivalent access to copy the source\n",
    "code from the same place counts as\n",
    "distribution of the source code, even\n",
    "though third parties are not compelled\n",
    "to copy the source along with the object\n",
    "code.\n",
    "\n",
    "  4. You may not copy, modify,\n",
    "sublicense, or distribute the Program\n",
    "except as expressly provided under this\n",
    "License.  Any attempt otherwise to copy,\n",
    "modify, sublicense or distribute the\n",
    "Program is void, and will automatically\n",
    "terminate your rights under this\n",
    "License.  However, parties who have\n",
    "received copies, or rights, from you\n",
    "under this License will not have their\n",
    "licenses terminated so long as such\n",
    "parties remain in full compliance.\n",
    "\n",
    "  5. You are not required to accept this\n",
    "License, since you have not signed it.\n",
    "However, nothing else grants you\n",
    "permission to modify or distribute the\n",
    "Program or its derivative works.  These\n",
    "actions are prohibited by law if you do\n",
    "not accept this License.  Therefore, by\n",
    "modifying or distributing the Program\n",
    "(or any work based on the Program), you\n",
    "indicate your acceptance of this License\n",
    "to do so, and all its terms and\n",
    "conditions for copying, distributing or\n",
    "modifying the Program or works based on\n",
    "it.\n",
    "\n",
    "  6. Each time you redistribute the\n",
    "Program (or any work based on the\n",
    "Program), the recipient automatically\n",
    "receives a license from the original\n",
    "licensor to copy, distribute or modify\n",
    "the Program subject to these terms and\n",
    "conditions.  You may not impose any\n",
    "further restrictions on the recipients'\n",
    "exercise of the rights granted herein.\n",
    "You are not responsible for enforcing\n",
    "compliance by third parties to this\n",
    "License.\n",
    "\n",
    "  7. If, as a consequence of a court\n",
    "judgment or allegation of patent\n",
    "infringement or for any other reason\n",
    "(not limited to patent issues),\n",
    "conditions are imposed on you (whether\n",
    "by court order, agreement or otherwise)\n",
    "that contradict the conditions of this\n",
    "License, they do not excuse you from the\n",
    "conditions of this License.  If you\n",
    "cannot distribute so as to satisfy\n",
    "simultaneously your obligations under\n",
    "this License and any other pertinent\n",
    "obligations, then as a consequence you\n",
    "may not distribute the Program at all.\n",
    "For example, if a patent license would\n",
    "not permit royalty-free redistribution\n",
    "of the Program by all those who receive\n",
    "copies directly or indirectly through\n",
    "you, then the only way you could satisfy\n",
    "both it and this License would be to\n",
    "refrain entirely from distribution of\n",
    "the Program.\n",
    "\n",
    "If any portion of this section is held\n",
    "invalid or unenforceable under any\n",
    "particular circumstance, the balance of\n",
    "the section is intended to apply and the\n",
    "section as a whole is intended to apply\n",
    "in other circumstances.\n",
    "\n",
    "It is not the purpose of this section to\n",
    "induce you to infringe any patents or\n",
    "other property right claims or to\n",
    "contest validity of any such claims;\n",
    "this section has the sole purpose of\n",
    "protecting the integrity of the free\n",
    "software distribution system, which is\n",
    "implemented by public license practices.\n",
    "Many people have made generous\n",
    "contributions to the wide range of\n",
    "software distributed through that system\n",
    "in reliance on consistent application of\n",
    "that system; it is up to the\n",
    "author/donor to decide if he or she is\n",
    "willing to distribute software through\n",
    "any other system and a licensee cannot\n",
    "impose \n",
    "that choice.\n",
    "\n",
    "This section is intended to make\n",
    "thoroughly clear what is believed to be\n",
    "a consequence of the rest of this\n",
    "License.\n",
    "\n",
    "  8. If the distribution and/or use of\n",
    "the Program is restricted in certain\n",
    "countries either by patents or by\n",
    "copyrighted interfaces, the original\n",
    "copyright holder who places the Program\n",
    "under this License may add an explicit\n",
    "geographical distribution limitation\n",
    "excluding those countries, so that\n",
    "distribution is permitted only in or\n",
    "among countries not thus excluded.  In\n",
    "such case, this License incorporates the\n",
    "limitation as if written in the body of\n",
    "this License.\n",
    "\n",
    "  9. The Free Software Foundation may\n",
    "publish revised and/or new versions of\n",
    "the General Public License from time to\n",
    "time.  Such new versions will be similar\n",
    "in spirit to the present version, but\n",
    "may differ in detail to address new\n",
    "problems or concerns.\n",
    "\n",
    "Each version is given a distinguishing\n",
    "version number.  If the Program\n",
    "specifies a version number of this\n",
    "License which applies to it and \"any\n",
    "later version\", you have the option of\n",
    "following the terms and conditions\n",
    "either of that version or of any later\n",
    "version published by the Free Software\n",
    "Foundation.  If the Program does not\n",
    "specify a version number of this\n",
    "License, you may choose any version ever\n",
    "published by the Free Software\n",
    "Foundation.\n",
    "\n",
    "  10. If you wish to incorporate parts\n",
    "of the Program into other free programs\n",
    "whose distribution conditions are\n",
    "different, write to the author to ask\n",
    "for permission.  For software which is\n",
    "copyrighted by the Free Software\n",
    "Foundation, write to the Free Software\n",
    "Foundation; we sometimes make exceptions\n",
    "for this.  Our decision will be guided\n",
    "by the two goals of preserving the free\n",
    "status of all derivatives of our free\n",
    "software and of promoting\n",
    NULL
};
#else
const char info_license_text40[] =
    "       GNU GENERAL PUBLIC LICENSE\n"
    "          Version 2, June 1991\n"
    "\n"
    " Copyright (C) 1989, 1991 Free Software\n"
    " Foundation, Inc.  59 Temple Place,\n"
    " Suite 330, Boston, MA  02111-1307  USA\n"
    "\n"
    " Everyone is permitted to copy and\n"
    " distribute verbatim copies of this\n"
    " license document, but changing it is\n"
    " not allowed.\n"
    "\n"
    "\n"
    "                Preamble\n"
    "\n"
    "The licenses for most software are\n"
    "designed to take away your freedom to\n"
    "share and change it. By contrast, the\n"
    "GNU General Public License is intended\n"
    "to guarantee your freedom to share and\n"
    "change free software--to make sure the\n"
    "software is free for all its users.\n"
    "This General Public License applies to\n"
    "most of the Free Software Foundation's\n"
    "software and to any other program whose\n"
    "authors commit to using it.  (Some other\n"
    "Free Software Foundation software is\n"
    "covered by the GNU Library General\n"
    "Public License instead.)  You can apply\n"
    "it to your programs, too.\n"
    "\n"
    "When we speak of free software, we are\n"
    "referring to freedom, not price.  Our\n"
    "General Public Licenses are designed to\n"
    "make sure that you have the freedom to\n"
    "distribute copies of free software (and\n"
    "charge for this service if you wish),\n"
    "that you receive source code or can get\n"
    "it if you want it, that you can change\n"
    "the software or use pieces of it in new\n"
    "free programs; and that you know you can\n"
    "do these things.\n"
    "\n"
    "To protect your rights, we need to make\n"
    "restrictions that forbid anyone to deny\n"
    "you these rights or to ask you to\n"
    "surrender the rights.  These\n"
    "restrictions translate to certain\n"
    "responsibilities for you if you\n"
    "distribute copies of the software, or if\n"
    "you modify it.\n"
    "\n"
    "For example, if you distribute copies of\n"
    "such a program, whether gratis or for a\n"
    "fee, you must give the recipients all\n"
    "the rights that you have.  You must make\n"
    "sure that they, too, receive or can get\n"
    "the source code.  And you must show them\n"
    "these terms so they know their rights.\n"
    "\n"
    "We protect your rights with two steps:\n"
    "(1) copyright the software, and (2)\n"
    "offer you this license which gives you\n"
    "legal permission to copy, distribute\n"
    "and/or modify the software.\n"
    "\n"
    "Also, for each author's protection and\n"
    "ours, we want to make certain that\n"
    "everyone understands that there is no\n"
    "warranty for this free software.  If the\n"
    "software is modified by someone else and\n"
    "passed on, we want its recipients to\n"
    "know that what they have is not the\n"
    "original, so that any problems\n"
    "introduced by others will not reflect on\n"
    "the original authors' reputations.\n"
    "\n"
    "Finally, any free program is threatened\n"
    "constantly by software patents.  We wish\n"
    "to avoid the danger that redistributors\n"
    "of a free program will individually\n"
    "obtain patent licenses, in effect making\n"
    "the program proprietary.  To prevent\n"
    "this, we have made it clear that any\n"
    "patent must be licensed for everyone's\n"
    "free use or not licensed at all.\n"
    "\n"
    "The precise terms and conditions for\n"
    "copying, distribution and modification\n"
    "follow.\n"
    "\n"
    "\n"
    "GNU GENERAL PUBLIC LICENSE TERMS AND\n"
    "CONDITIONS FOR COPYING, DISTRIBUTION AND\n"
    "MODIFICATION\n"
    "\n"
    "0. This License applies to any program\n"
    "or other work which contains a notice\n"
    "placed by the copyright holder saying it\n"
    "may be distributed under the terms of\n"
    "this General Public License.  The\n"
    "\"Program\", below, refers to any such\n"
    "program or work, and a \"work based on\n"
    "the Program\" means either the Program or\n"
    "any derivative work under copyright law:\n"
    "that is to say, a work containing the\n"
    "Program or a portion of it, either\n"
    "verbatim or with modifications and/or\n"
    "translated into another language.\n"
    "(Hereinafter, translation is included\n"
    "without limitation in the term\n"
    "\"modification\".)  Each licensee is\n"
    "addressed as \"you\".\n"
    "\n"
    "Activities other than copying,\n"
    "distribution and modification are not\n"
    "covered by this License; they are\n"
    "outside its scope.  The act of running\n"
    "the Program is not restricted, and the\n"
    "output from the Program is covered only\n"
    "if its contents constitute a work based\n"
    "on the Program (independent of having\n"
    "been made by running the Program).\n"
    "Whether that is true depends on what the\n"
    "Program does.\n"
    "\n"
    "  1. You may copy and distribute\n"
    "verbatim copies of the Program's source\n"
    "code as you receive it, in any medium,\n"
    "provided that you conspicuously and\n"
    "appropriately publish on each copy an\n"
    "appropriate copyright notice and\n"
    "disclaimer of warranty; keep intact all\n"
    "the notices that refer to this License\n"
    "and to the absence of any warranty; and\n"
    "give any other recipients of the Program\n"
    "a copy of this License along with the\n"
    "Program.\n"
    "\n"
    "You may charge a fee for the physical\n"
    "act of transferring a copy, and you may\n"
    "at your option offer warranty protection\n"
    "in exchange for a fee.\n"
    "\n"
    "  2. You may modify your copy or copies\n"
    "of the Program or any portion of it,\n"
    "thus forming a work based on the\n"
    "Program, and copy and distribute such\n"
    "modifications or work under the terms of\n"
    "Section 1 above, provided that you also\n"
    "meet all of these conditions:\n"
    "\n"
    "    a) You must cause the modified files\n"
    "    to carry prominent notices stating\n"
    "    that you changed the files and the\n"
    "    date of any change.\n"
    "\n"
    "    b) You must cause any work that you\n"
    "    distribute or publish, that in whole\n"
    "    or in part contains or is derived\n"
    "    from the Program or any part\n"
    "    thereof, to be licensed as a whole\n"
    "    at no charge to all third parties\n"
    "    under the terms of this License.\n"
    "\n"
    "    c) If the modified program normally\n"
    "    reads commands interactively when\n"
    "    run, you must cause it, when started\n"
    "    running for such interactive use in\n"
    "    the most ordinary way, to print or\n"
    "    display an announcement including an\n"
    "    appropriate copyright notice and a\n"
    "    notice that there is no warranty (or\n"
    "    else, saying that you provide a\n"
    "    warranty) and that users may\n"
    "    redistribute the program under these\n"
    "    conditions, and telling the user how\n"
    "    to view a copy of this License.\n"
    "    (Exception: if the Program itself is\n"
    "    interactive but does not normally\n"
    "    print such an announcement, your\n"
    "    work based on the Program is not\n"
    "    required to print an announcement.)\n"
    "\n"
    "These requirements apply to the modified\n"
    "work as a whole.  If identifiable\n"
    "sections of that work are not derived\n"
    "from the Program, and can be reasonably\n"
    "considered independent and separate\n"
    "works in themselves, then this License,\n"
    "and its terms, do not apply to those\n"
    "sections when you distribute them as\n"
    "separate works.  But when you distribute\n"
    "the same sections as part of a whole\n"
    "which is a work based on the Program,\n"
    "the distribution of the whole must be on\n"
    "the terms of this License, whose\n"
    "permissions for other licensees extend\n"
    "to the entire whole, and thus to each\n"
    "and every part regardless of who wrote\n"
    "it.\n"
    "\n"
    "Thus, it is not the intent of this\n"
    "section to claim rights or contest your\n"
    "rights to work written entirely by you;\n"
    "rather, the intent is to exercise the\n"
    "right to control the distribution of\n"
    "derivative or collective works based on\n"
    "the Program.\n"
    "\n"
    "In addition, mere aggregation of another\n"
    "work not based on the Program with the\n"
    "Program (or with a work based on the\n"
    "Program) on a volume of a storage or\n"
    "distribution medium does not bring the\n"
    "other work under the scope of this\n"
    "License.\n"
    "\n"
    "  3. You may copy and distribute the\n"
    "Program (or a work based on it, under\n"
    "Section 2) in object code or executable\n"
    "form under the terms of Sections 1 and 2\n"
    "above provided that you also do one of\n"
    "the following:\n"
    "\n"
    "    a) Accompany it with the complete\n"
    "    corresponding machine-readable\n"
    "    source code, which must be\n"
    "    distributed under the terms of\n"
    "    Sections 1 and 2 above on a medium\n"
    "    customarily used for software\n"
    "    interchange; or,\n"
    "\n"
    "    b) Accompany it with a written\n"
    "    offer, valid for at least three\n"
    "    years, to give any third party, for\n"
    "    a charge no more than your cost of\n"
    "    physically performing source\n"
    "    distribution, a complete machine-\n"
    "    readable copy of the corresponding\n"
    "    source code, to be distributed under\n"
    "    the terms of Sections 1 and 2 above\n"
    "    on a medium customarily used for\n"
    "    software interchange; or,\n"
    "\n"
    "    c) Accompany it with the information\n"
    "    you received as to the offer to\n"
    "    distribute corresponding source\n"
    "    code.  (This alternative is allowed\n"
    "    only for noncommercial distribution\n"
    "    and only if you received the program\n"
    "    in object code or executable form\n"
    "    with such an offer, in accord with\n"
    "    Subsection b above.)\n"
    "\n"
    "The source code for a work means the\n"
    "preferred form of the work for making\n"
    "modifications to it.  For an executable\n"
    "work, complete source code means all the\n"
    "source code for all modules it contains,\n"
    "plus any associated interface definition\n"
    "files, plus the scripts used to control\n"
    "compilation and installation of the\n"
    "executable.  However, as a special\n"
    "exception, the source code distributed\n"
    "need not include anything that is\n"
    "normally distributed (in either source\n"
    "or binary form) with the major\n"
    "components (compiler, kernel, and so on)\n"
    "of the operating system on which the\n"
    "executable runs, unless that component\n"
    "itself accompanies the executable.\n"
    "\n"
    "If distribution of executable or object\n"
    "code is made by offering access to copy\n"
    "from a designated place, then offering\n"
    "equivalent access to copy the source\n"
    "code from the same place counts as\n"
    "distribution of the source code, even\n"
    "though third parties are not compelled\n"
    "to copy the source along with the object\n"
    "code.\n"
    "\n"
    "  4. You may not copy, modify,\n"
    "sublicense, or distribute the Program\n"
    "except as expressly provided under this\n"
    "License.  Any attempt otherwise to copy,\n"
    "modify, sublicense or distribute the\n"
    "Program is void, and will automatically\n"
    "terminate your rights under this\n"
    "License.  However, parties who have\n"
    "received copies, or rights, from you\n"
    "under this License will not have their\n"
    "licenses terminated so long as such\n"
    "parties remain in full compliance.\n"
    "\n"
    "  5. You are not required to accept this\n"
    "License, since you have not signed it.\n"
    "However, nothing else grants you\n"
    "permission to modify or distribute the\n"
    "Program or its derivative works.  These\n"
    "actions are prohibited by law if you do\n"
    "not accept this License.  Therefore, by\n"
    "modifying or distributing the Program\n"
    "(or any work based on the Program), you\n"
    "indicate your acceptance of this License\n"
    "to do so, and all its terms and\n"
    "conditions for copying, distributing or\n"
    "modifying the Program or works based on\n"
    "it.\n"
    "\n"
    "  6. Each time you redistribute the\n"
    "Program (or any work based on the\n"
    "Program), the recipient automatically\n"
    "receives a license from the original\n"
    "licensor to copy, distribute or modify\n"
    "the Program subject to these terms and\n"
    "conditions.  You may not impose any\n"
    "further restrictions on the recipients'\n"
    "exercise of the rights granted herein.\n"
    "You are not responsible for enforcing\n"
    "compliance by third parties to this\n"
    "License.\n"
    "\n"
    "  7. If, as a consequence of a court\n"
    "judgment or allegation of patent\n"
    "infringement or for any other reason\n"
    "(not limited to patent issues),\n"
    "conditions are imposed on you (whether\n"
    "by court order, agreement or otherwise)\n"
    "that contradict the conditions of this\n"
    "License, they do not excuse you from the\n"
    "conditions of this License.  If you\n"
    "cannot distribute so as to satisfy\n"
    "simultaneously your obligations under\n"
    "this License and any other pertinent\n"
    "obligations, then as a consequence you\n"
    "may not distribute the Program at all.\n"
    "For example, if a patent license would\n"
    "not permit royalty-free redistribution\n"
    "of the Program by all those who receive\n"
    "copies directly or indirectly through\n"
    "you, then the only way you could satisfy\n"
    "both it and this License would be to\n"
    "refrain entirely from distribution of\n"
    "the Program.\n"
    "\n"
    "If any portion of this section is held\n"
    "invalid or unenforceable under any\n"
    "particular circumstance, the balance of\n"
    "the section is intended to apply and the\n"
    "section as a whole is intended to apply\n"
    "in other circumstances.\n"
    "\n"
    "It is not the purpose of this section to\n"
    "induce you to infringe any patents or\n"
    "other property right claims or to\n"
    "contest validity of any such claims;\n"
    "this section has the sole purpose of\n"
    "protecting the integrity of the free\n"
    "software distribution system, which is\n"
    "implemented by public license practices.\n"
    "Many people have made generous\n"
    "contributions to the wide range of\n"
    "software distributed through that system\n"
    "in reliance on consistent application of\n"
    "that system; it is up to the\n"
    "author/donor to decide if he or she is\n"
    "willing to distribute software through\n"
    "any other system and a licensee cannot\n"
    "impose \n"
    "that choice.\n"
    "\n"
    "This section is intended to make\n"
    "thoroughly clear what is believed to be\n"
    "a consequence of the rest of this\n"
    "License.\n"
    "\n"
    "  8. If the distribution and/or use of\n"
    "the Program is restricted in certain\n"
    "countries either by patents or by\n"
    "copyrighted interfaces, the original\n"
    "copyright holder who places the Program\n"
    "under this License may add an explicit\n"
    "geographical distribution limitation\n"
    "excluding those countries, so that\n"
    "distribution is permitted only in or\n"
    "among countries not thus excluded.  In\n"
    "such case, this License incorporates the\n"
    "limitation as if written in the body of\n"
    "this License.\n"
    "\n"
    "  9. The Free Software Foundation may\n"
    "publish revised and/or new versions of\n"
    "the General Public License from time to\n"
    "time.  Such new versions will be similar\n"
    "in spirit to the present version, but\n"
    "may differ in detail to address new\n"
    "problems or concerns.\n"
    "\n"
    "Each version is given a distinguishing\n"
    "version number.  If the Program\n"
    "specifies a version number of this\n"
    "License which applies to it and \"any\n"
    "later version\", you have the option of\n"
    "following the terms and conditions\n"
    "either of that version or of any later\n"
    "version published by the Free Software\n"
    "Foundation.  If the Program does not\n"
    "specify a version number of this\n"
    "License, you may choose any version ever\n"
    "published by the Free Software\n"
    "Foundation.\n"
    "\n"
    "  10. If you wish to incorporate parts\n"
    "of the Program into other free programs\n"
    "whose distribution conditions are\n"
    "different, write to the author to ask\n"
    "for permission.  For software which is\n"
    "copyrighted by the Free Software\n"
    "Foundation, write to the Free Software\n"
    "Foundation; we sometimes make exceptions\n"
    "for this.  Our decision will be guided\n"
    "by the two goals of preserving the free\n"
    "status of all derivatives of our free\n"
    "software and of promoting\n";
#endif
#endif

const char info_warranty_text[] =
    "NO WARRANTY\n"
    "~~~~~~~~~~~\n"
    "\n"
    "  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE,\n"
    "THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT\n"
    "PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN OTHERWISE\n"
    "STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER\n"
    "PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY\n"
    "OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,\n"
    "BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
    "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n"
    "THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF\n"
    "THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE\n"
    "DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY\n"
    "SERVICING, REPAIR OR CORRECTION.\n"
    "\n"
    "  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR\n"
    "AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY\n"
    "OTHER PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE\n"
    "PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR\n"
    "DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR\n"
    "CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR\n"
    "INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
    "TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR\n"
    "LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE\n"
    "OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),\n"
    "EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF\n"
    "THE POSSIBILITY OF SUCH DAMAGES.\n";

#ifdef USE_SDLUI
const char info_warranty_text40[] =
    "NO WARRANTY\n"
    "~~~~~~~~~~~\n"
    "\n"
    "  11. BECAUSE THE PROGRAM IS LICENSED\n"
    "FREE OF CHARGE, THERE IS NO WARRANTY FOR\n"
    "THE PROGRAM, TO THE EXTENT PERMITTED BY\n"
    "APPLICABLE LAW.  EXCEPT WHEN OTHERWISE\n"
    "STATED IN WRITING THE COPYRIGHT HOLDERS\n"
    "AND/OR OTHER PARTIES PROVIDE THE PROGRAM\n"
    "\"AS IS\" WITHOUT WARRANTY OF ANY KIND,\n"
    "EITHER EXPRESSED OR IMPLIED, INCLUDING,\n"
    "BUT NOT LIMITED TO, THE IMPLIED\n"
    "WARRANTIES OF MERCHANTABILITY AND\n"
    "FITNESS FOR A PARTICULAR PURPOSE.  THE\n"
    "ENTIRE RISK AS TO THE QUALITY AND\n"
    "PERFORMANCE OF THE PROGRAM IS WITH YOU.\n"
    "SHOULD THE PROGRAM PROVE DEFECTIVE, YOU\n"
    "ASSUME THE COST OF ALL NECESSARY\n"
    "SERVICING, REPAIR OR CORRECTION.\n"
    "\n"
    "  12. IN NO EVENT UNLESS REQUIRED BY\n"
    "APPLICABLE LAW OR AGREED TO IN WRITING\n"
    "WILL ANY COPYRIGHT HOLDER, OR ANY OTHER\n"
    "PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE\n"
    "THE PROGRAM AS PERMITTED ABOVE, BE\n"
    "LIABLE TO YOU FOR DAMAGES, INCLUDING ANY\n"
    "GENERAL, SPECIAL, INCIDENTAL OR\n"
    "CONSEQUENTIAL DAMAGES ARISING OUT OF THE\n"
    "USE OR INABILITY TO USE THE PROGRAM\n"
    "(INCLUDING BUT NOT LIMITED TO LOSS OF\n"
    "DATA OR DATA BEING RENDERED INACCURATE\n"
    "OR LOSSES SUSTAINED BY YOU OR THIRD\n"
    "PARTIES OR A FAILURE OF THE PROGRAM TO\n"
    "OPERATE WITH ANY OTHER PROGRAMS), EVEN\n"
    "IF SUCH HOLDER OR OTHER PARTY HAS BEEN\n"
    "ADVISED OF THE POSSIBILITY OF SUCH\n"
    "DAMAGES.\n";
#endif

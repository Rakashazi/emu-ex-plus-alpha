#pragma once

#define EMU_EX_PLATFORM

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Should we enable AROS support. */
/* #undef AMIGA_AROS */

/* Should we enable M68K AmigaOS support. */
/* #undef AMIGA_M68K */

/* Should we enable Amiga Morphos support. */
/* #undef AMIGA_MORPHOS */

/* Should we enable AmigaOS4 support. */
/* #undef AMIGA_OS4 */

/* Should we enable Amigaos support. */
/* #undef AMIGA_SUPPORT */

/* Are we compiling for either BeOS or Haiku? */
/* #undef BEOS_COMPILE */

/* Enable support for BSD style joysticks. */
/* #undef BSD_JOYSTICK */

/* Enable cegcc support. */
/* #undef CEGCC_COMPILE */

/* Enable plain darwin compilation */
/* #undef DARWIN_COMPILE */

/* Use the 65xx cpu history feature. */
/* #undef FEATURE_CPUMEMHISTORY */

/* Enable emulation for digital joysticks. */
/* #undef HAS_DIGITAL_JOYSTICK */

/* Enable Mac IOHIDManager Joystick driver. */
/* #undef HAS_HIDMGR */

/* Enable emulation for USB joysticks. */
/* #undef HAS_USB_JOYSTICK */

/* Define to 1 if you have the `accept' function. */
#define HAVE_ACCEPT 1

/* Define to 1 if you have the <alloca.h> header file. */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
//#define HAVE_ALSA_ASOUNDLIB_H 1

/* Define to 1 if you have the <aros/_timeval.h> header file. */
/* #undef HAVE_AROS__TIMEVAL_H */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT 1

/* Enable AudioUnit support. */
/* #undef HAVE_AUDIO_UNIT */

/* Define to 1 if you have the `bind' function. */
#define HAVE_BIND 1

/* Use backtrace facility of libC or libexecinfo */
/* #undef HAVE_BT_SYMBOLS */

/* Define to 1 if you have the <ByteOrder.h> header file. */
/* #undef HAVE_BYTEORDER_H */

/* Enable Cairo rendering support */
/* #undef HAVE_CAIRO */

/* Support for Catweasel MKIII. */
/* #undef HAVE_CATWEASELMKIII */

/* Support for direct PCI I/O access Catweasel MKIII. */
/* #undef HAVE_CATWEASELMKIII_IO /**/

/* Define to 1 if you have the <commctrl.h> header file. */
/* #undef HAVE_COMMCTRL_H */

/* Define to 1 if you have the `connect' function. */
#define HAVE_CONNECT 1

/* Define to 1 if you have the <CoreServices/CoreServices.h> header file. */
/* #undef HAVE_CORESERVICES_CORESERVICES_H */

/* Define to 1 if you have the <CoreVideo/CVHostTime.h> header file. */
/* #undef HAVE_COREVIDEO_CVHOSTTIME_H */

/* Define to 1 if you have the <cwsid.h> header file. */
/* #undef HAVE_CWSID_H */

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Add native GTK3 UI debugging code. */
/* #undef HAVE_DEBUG_GTK3UI */

/* Define to 1 if you have the declaration of `sys_siglist', and to 0 if you
   don't. */
#define HAVE_DECL_SYS_SIGLIST 1

/* Define to 1 if you have the <devices/ahi.h> header file. */
/* #undef HAVE_DEVICES_AHI_H */

/* Define to 1 if you have the <dev/ppbus/ppbconf.h> header file. */
/* #undef HAVE_DEV_PPBUS_PPBCONF_H */

/* Define to 1 if you have the <dev/ppbus/ppi.h> header file. */
/* #undef HAVE_DEV_PPBUS_PPI_H */

/* Use DirectInput joystick driver */
/* #undef HAVE_DINPUT */

/* dinput.lib or libdinput.a are present */
/* #undef HAVE_DINPUT_LIB */

/* Define to 1 if you have the <direct.h> header file. */
/* #undef HAVE_DIRECT_H */

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dirname' function. */
#define HAVE_DIRNAME 1

/* Define to 1 if you have the <dir.h> header file. */
/* #undef HAVE_DIR_H */

/* dsound.lib or libdsound.a are present */
/* #undef HAVE_DSOUND_LIB */

/* Support for dynamic library loading. */
#define HAVE_DYNLIB_SUPPORT /**/

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
/* #undef HAVE_EXECINFO_H */

/* This version provides FastSID support. */
#define HAVE_FASTSID 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Have FFMPEG av* libs available */
/* #undef HAVE_FFMPEG */

/* Have FFMPEG avresample lib available */
/* #undef HAVE_FFMPEG_AVRESAMPLE */

/* FFMPEG uses subdirs for headers */
/* #undef HAVE_FFMPEG_HEADER_SUBDIRS */

/* Have FFMPEG swresample lib available */
/* #undef HAVE_FFMPEG_SWRESAMPLE */

/* Have FFMPEG swscale lib available */
/* #undef HAVE_FFMPEG_SWSCALE */

/* Define to 1 if you have the <FLAC/stream_decoder.h> header file. */
//#define HAVE_FLAC_STREAM_DECODER_H 1

/* Use fontconfig for custom fonts. */
//#define HAVE_FONTCONFIG /**/

/* Define to 1 if you have the `fork' function. */
//#define HAVE_FORK 1

/* Support for FreeBSD par port device file. */
/* #undef HAVE_FREEBSD_PARPORT_HEADERS */

#if !defined(__ANDROID__) || __ANDROID_API__ >= 24
// fseeko/ftello not supported on older Android versions

/* Define to 1 if you have the `fseeko' function. */
#define HAVE_FSEEKO 1

/* Define to 1 if you have the `ftello' function. */
#define HAVE_FTELLO 1

#endif

/* Enable Fullscreen support. */
/* #undef HAVE_FULLSCREEN */

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `getdtablesize' function. */
#define HAVE_GETDTABLESIZE 1

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define if gethostbyname2 can be used */
/* #undef HAVE_GETHOSTBYNAME2 */

/* Define if getipnodebyname can be used */
/* #undef HAVE_GETIPNODEBYNAME */

/* Define to 1 if you have the `getpwuid' function. */
#define HAVE_GETPWUID 1

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Can we use the GIF or UNGIF library? */
/* #undef HAVE_GIF */

/* GTK3 OpenGL support uses GLEW */
//#define HAVE_GTK3_GLEW /**/

/* Enable OpenGL support in GTK3 UI */
//#define HAVE_GTK3_OPENGL /**/

/* Support for HardSID. */
/* #undef HAVE_HARDSID */

/* Support for PCI/ISA HardSID. */
//#define HAVE_HARDSID_IO /**/

/* Define to 1 if you have the `htonl' function. */
#define HAVE_HTONL 1

/* Define to 1 if you have the `htons' function. */
#define HAVE_HTONS 1

/* Enable arbitrary window scaling */
//#define HAVE_HWSCALE /**/

/* Define to 1 if you have the `i386_set_ioperm' function. */
/* #undef HAVE_I386_SET_IOPERM */

/* Define to 1 if you have the <ieee1284.h> header file. */
/* #undef HAVE_IEEE1284_H */

/* Define to 1 if you have the `inb' function. */
/* #undef HAVE_INB */

/* Define to 1 if you have the `inbv' function. */
/* #undef HAVE_INBV */

/* Define to 1 if you have the `inb_p' function. */
/* #undef HAVE_INB_P */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if the in_addr_t type is present. */
#define HAVE_IN_ADDR_T /**/

/* Define to 1 if you have the `ioperm' function. */
/* #undef HAVE_IOPERM */

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define if ipv6 can be used */
/* #undef HAVE_IPV6 */

/* Can we use the JPEG library? */
/* #undef HAVE_JPEG */

/* Define to 1 if you have the 'amd64' library (-lamd64). */
/* #undef HAVE_LIBAMD64 */

/* Define to 1 if you have the `bsd' library (-lbsd). */
#define HAVE_LIBBSD 1

/* Define to 1 if you have the `bz2' library (-lbz2). */
/* #undef HAVE_LIBBZ2 */

/* Define to 1 if you have the `FLAC' library (-lFLAC). */
//#define HAVE_LIBFLAC 1

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the `iconv' library (-liconv). */
/* #undef HAVE_LIBICONV */

/* Define to 1 if you have the 'ieee1284' library (-lieee1284). */
/* #undef HAVE_LIBIEEE1284 */

/* Define to 1 if you have the `lzma' library (-llzma). */
/* #undef HAVE_LIBLZMA */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `ogg' library (-logg). */
//#define HAVE_LIBOGG 1

/* Define to 1 if you have the `ossaudio' library (-lossaudio). */
/* #undef HAVE_LIBOSSAUDIO */

/* Define to 1 if you have the 'pciutils' library. */
//#define HAVE_LIBPCI /**/

/* Define to 1 if you have the `posix' library (-lposix). */
/* #undef HAVE_LIBPOSIX */

/* Define to 1 if you have the `pthread' library (-lpthread). */
/* #undef HAVE_LIBPTHREAD */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the <libusbhid.h> header file. */
/* #undef HAVE_LIBUSBHID_H */

/* Define to 1 if you have the <libusb.h> header file. */
/* #undef HAVE_LIBUSB_H */

/* Define to 1 if you have the `va' library (-lva). */
/* #undef HAVE_LIBVA */

/* Define to 1 if you have the `vorbis' library (-lvorbis). */
//#define HAVE_LIBVORBIS 1

/* Define to 1 if you have the `vorbisenc' library (-lvorbisenc). */
//#define HAVE_LIBVORBISENC 1

/* Define to 1 if you have the `vorbisfile' library (-lvorbisfile). */
//#define HAVE_LIBVORBISFILE 1

/* Enable lightpen support */
#define HAVE_LIGHTPEN /**/

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <linux/hardsid.h> header file. */
/* #undef HAVE_LINUX_HARDSID_H */

/* Define to 1 if you have the <linux/parport.h> header file. */
//#define HAVE_LINUX_PARPORT_H 1

/* Support for Linux par port device file. */
//#define HAVE_LINUX_PARPORT_HEADERS /**/

/* Define to 1 if you have the <linux/ppdev.h> header file. */
//#define HAVE_LINUX_PPDEV_H 1

/* Define to 1 if you have the <linux/soundcard.h> header file. */
/* #undef HAVE_LINUX_SOUNDCARD_H */

/* Define to 1 if you have the `listen' function. */
#define HAVE_LISTEN 1

/* Define to 1 if you have the `ltoa' function. */
/* #undef HAVE_LTOA */

/* Define to 1 if you have the <machine/cpufunc.h> header file. */
/* #undef HAVE_MACHINE_CPUFUNC_H */

/* Define to 1 if you have the <machine/joystick.h> header file. */
/* #undef HAVE_MACHINE_JOYSTICK_H */

/* Define to 1 if you have the <machine/pio.h> header file. */
/* #undef HAVE_MACHINE_PIO_H */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <machine/sysarch.h> header file. */
/* #undef HAVE_MACHINE_SYSARCH_H */

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
/* #undef HAVE_MEMORY_H */

/* Enable MIDI emulation. */
//#define HAVE_MIDI /**/

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Enable 1351 mouse support */
//#define HAVE_MOUSE /**/

/* Define to 1 if you have the <mpg123.h> header file. */
/* #undef HAVE_MPG123_H */

/* Use nanosleep instead of usleep */
#define HAVE_NANOSLEEP /**/

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Enable netplay support */
//#define HAVE_NETWORK /**/

/* Use the new 8580 filter */
#define HAVE_NEW_8580_FILTER 1

/* Define to 1 if the system has the type `off_t'. */
#define HAVE_OFF_T

/* Include sys/types.h for off_t */
#define HAVE_OFF_T_IN_SYS_TYPES

/* Define to 1 if you have the `outb' function. */
/* #undef HAVE_OUTB */

/* Define to 1 if you have the `outbv' function. */
/* #undef HAVE_OUTBV */

/* Define to 1 if you have the `outb_p' function. */
/* #undef HAVE_OUTB_P */

/* Support for ParSID. */
/* #undef HAVE_PARSID */

/* Support for PCAP library. */
/* #undef HAVE_PCAP */

/* A libpcap version with pcap_inject is available */
/* #undef HAVE_PCAP_INJECT */

/* A libpcap version with pcap_sendpacket is available */
/* #undef HAVE_PCAP_SENDPACKET */

/* Define to 1 if you have the <pci/pci.h> header file. */
//#define HAVE_PCI_PCI_H 1

/* Can we use the PNG library? */
/* #undef HAVE_PNG */

/* Define to 1 if you have the <portaudio.h> header file. */
/* #undef HAVE_PORTAUDIO_H */

/* Support for file device based access to ParSID. */
//#define HAVE_PORTSID /**/

/* Define to 1 if you have the <process.h> header file. */
/* #undef HAVE_PROCESS_H */

/* Define to 1 if you have the <proto/openpci.h> header file. */
/* #undef HAVE_PROTO_OPENPCI_H */

/* Define to 1 if you have the <pulse/simple.h> header file. */
/* #undef HAVE_PULSE_SIMPLE_H */

/* Enable QuickTime support. */
/* #undef HAVE_QUICKTIME */

/* Define to 1 if you have the `random' function. */
#define HAVE_RANDOM 1

/* Support for CS8900A ethernet controller. */
/* #undef HAVE_RAWNET */

/* Enable the readline library */
#define HAVE_READLINE /**/

/* Define to 1 if you have the <readline/readline.h> header file. */
#define HAVE_READLINE_READLINE_H 1

/* Support for OpenCBM (former CBM4Linux). */
//#define HAVE_REALDEVICE /**/

/* Define to 1 if you have the `recv' function. */
#define HAVE_RECV 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* This version provides ReSID support. */
#define HAVE_RESID /**/

/* This version provides ReSID-DTV support. */
#define HAVE_RESID_DTV /**/

/* Define to 1 if you have the `rewinddir' function. */
#define HAVE_REWINDDIR 1

/* Does the 'readline' library support 'rl_readline_name'? */
#define HAVE_RLNAME /**/

/* Enable RS232 device emulation. */
//#define HAVE_RS232DEV /**/

/* Enable RS232 network support */
//#define HAVE_RS232NET /**/

/* Enable SDLmain replacement */
/* #undef HAVE_SDLMAIN */

/* Define to 1 if you have the <SDL_audio.h> header file. */
/* #undef HAVE_SDL_AUDIO_H */

/* Define to 1 if you have the <SDL.h> header file. */
/* #undef HAVE_SDL_H */

/* Define to 1 if you have the <SDL_main.h> header file. */
/* #undef HAVE_SDL_MAIN_H */

/* Define to 1 if you have the `SDL_NumJoysticks' function. */
/* #undef HAVE_SDL_NUMJOYSTICKS */

/* Define to 1 if you have the <SDL/SDL_audio.h> header file. */
/* #undef HAVE_SDL_SDL_AUDIO_H */

/* Define to 1 if you have the <SDL/SDL.h> header file. */
/* #undef HAVE_SDL_SDL_H */

/* Define to 1 if you have the <SDL/SDL_main.h> header file. */
/* #undef HAVE_SDL_SDL_MAIN_H */

/* Define to 1 if you have the `send' function. */
#define HAVE_SEND 1

/* Define to 1 if you have the <shlobj.h> header file. */
/* #undef HAVE_SHLOBJ_H */

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Use more accurate buffer fill reporting */
//#define HAVE_SND_PCM_AVAIL /**/

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <soundcard.h> header file. */
/* #undef HAVE_SOUNDCARD_H */

/* Support for SSI2001 (ISA SID card). */
//#define HAVE_SSI2001 /**/

/* Static linking for lame libs */
/* #undef HAVE_STATIC_LAME */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
/* #undef HAVE_STDLIB_H */

/* Define to 1 if you have the `stpcpy' function. */
#define HAVE_STPCPY 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the 'strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
/* #undef HAVE_STRING_H */

/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1

/* Define to 1 if you have the `strlwr' function. */
/* #undef HAVE_STRLWR */

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strrev' function. */
/* #undef HAVE_STRREV */

/* Define to 1 if you have the `strtok' function. */
#define HAVE_STRTOK 1

/* Define to 1 if you have the `strtok_r' function. */
#define HAVE_STRTOK_R 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the `swab' function. */
#define HAVE_SWAB 1

/* Define to 1 if you have the <sys/audioio.h> header file. */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define to 1 if you have the <sys/dirent.h> header file. */
/* #undef HAVE_SYS_DIRENT_H */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
//#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/io.h> header file. */
/* #undef HAVE_SYS_IO_H */

/* Define to 1 if you have the <sys/joystick.h> header file. */
/* #undef HAVE_SYS_JOYSTICK_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Is the type time_t defined in <time.h>? */
#define HAVE_TIME_T_IN_TIME_H /**/

/* Is the type time_t defined in <sys/types.h>? */
#define HAVE_TIME_T_IN_TYPES_H /**/

/* TUN/TAP support using <linux/if_tun.h> */
/* #undef HAVE_TUNTAP */

/* Define to 1 if you have the `ultoa' function. */
/* #undef HAVE_ULTOA */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <usbhid.h> header file. */
/* #undef HAVE_USBHID_H */

/* Define to 1 if you have the <usb.h> header file. */
/* #undef HAVE_USB_H */

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if the system has the type `u_short'. */
#define HAVE_U_SHORT 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the <vorbis/vorbisfile.h> header file. */
//#define HAVE_VORBIS_VORBISFILE_H 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <winioctl.h> header file. */
/* #undef HAVE_WINIOCTL_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Can we use the ZLIB compression library? */
#define HAVE_ZLIB /**/

/* Enable support for Linux style joysticks. */
//#define LINUX_JOYSTICK /**/

/* Enable Mac OS X specific code. */
/* #undef MACOSX_SUPPORT */

/* Enable Mac Joystick support. */
/* #undef MAC_JOYSTICK */

/* Whether struct sockaddr::__ss_family exists */
/* #undef NEED_PREFIXED_SS_FAMILY */

/* Name of package */
#define PACKAGE "vice"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Enable morphos shared SDL library support. */
/* #undef POWERSDL_AMIGA_INLINE */

/* Where do we want to install the executable? */
#define PREFIX "/usr/local"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Enable amiga shared SDL library support. */
/* #undef SDL_AMIGA_INLINE */

/* FFMPEG libraries are shared */
/* #undef SHARED_FFMPEG */

/* The size of `time_t', as computed by sizeof. */
#define SIZEOF_TIME_T do not use this

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT do not use this

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG do not use this

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT do not use this

/* FFMPEG libraries are static */
/* #undef STATIC_FFMPEG */

/* Define to 1 if you have the ANSI C header files. */
/* #undef STDC_HEADERS */

/* time_t is 32 bit */
/* #undef TIME_T_IS_32BIT */

/* time_t is 64 bit */
#define TIME_T_IS_64BIT /**/

/* Are we compiling for unix? */
#define UNIX_COMPILE

/* Define if this version is unstable. */
/* #undef UNSTABLE */

/* Enable alsa support. */
//#define USE_ALSA /**/

/* Enable CoreAudio support. */
/* #undef USE_COREAUDIO */

/* Enable directx sound support. */
/* #undef USE_DXSOUND */

/* Use embedded data files. */
/* #undef USE_EMBEDDED */

/* Enable FLAC support. */
//#define USE_FLAC /**/

/* Define when using gcc */
#define USE_GCC /**/

/* Enable lamemp3 support. */
/* #undef USE_LAMEMP3 */

/* Enable mpg123 mp3 decoding support. */
//#define USE_MPG123 /**/

/* Use native GTK3 UI. */
//#define USE_NATIVE_GTK3 /**/

/* Enable oss support. */
/* #undef USE_OSS */

/* Enable portaudio sampling support. */
/* #undef USE_PORTAUDIO */

/* Enable pulseaudio support. */
/* #undef USE_PULSE */

/* Enable SDL2 prefix for header inclusion. */
/* #undef USE_SDL2_PREFIX */

/* Enable SDL UI support. */
/* #undef USE_SDLUI */

/* Enable SDL sound support. */
/* #undef USE_SDL_AUDIO */

/* Enable SDL prefix for header inclusion. */
/* #undef USE_SDL_PREFIX */

/* define when using the svn revision in addition to the version */
/* #undef USE_SVN_REVISION */

/* Enable ogg/vorbis support. */
//#define USE_VORBIS /**/

/* Version number of package */
#define VERSION "3.6.1"

/* Where should we lookup for data files? */
//#define VICEDIR "/usr/local/lib64/vice"

/* Support for The Final Ethernet */
/* #undef VICE_USE_LIBNET_1_1 */

/* Are we compiling for win32? */
/* #undef WIN32_COMPILE */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if aros can handle auto resolving of library bases. */
/* #undef WORKING_AROS_AUTO */

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #undef YYTEXT_POINTER */

/* Enable large inode numbers on Mac OS X 10.5.  */
//#ifndef _DARWIN_USE_64_BIT_INODE
//# define _DARWIN_USE_64_BIT_INODE 1
//#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* ss_family is not defined here, use __ss_family instead */
/* #undef ss_family */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

#define VICE_API __attribute__((visibility("default")))

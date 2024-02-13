// Re-map any functions from GLIBC > 2.9
__asm__(".symver __exp_finite,exp@GLIBC_2.4");
__asm__(".symver __pow_finite,pow@GLIBC_2.4");
__asm__(".symver __log_finite,log@GLIBC_2.4");
__asm__(".symver __log10_finite,log10@GLIBC_2.4");
__asm__(".symver __fmod_finite,fmod@GLIBC_2.4");
__asm__(".symver powf,powf@GLIBC_2.4");
__asm__(".symver pow,pow@GLIBC_2.4");
__asm__(".symver log,log@GLIBC_2.4");
__asm__(".symver fcntl,fcntl@GLIBC_2.4");
__asm__(".symver __isoc23_sscanf,sscanf@GLIBC_2.4");
__asm__(".symver __isoc23_fscanf,fscanf@GLIBC_2.4");
__asm__(".symver __isoc23_strtoul,strtoul@GLIBC_2.4");
__asm__(".symver __isoc23_strtol,strtol@GLIBC_2.4");

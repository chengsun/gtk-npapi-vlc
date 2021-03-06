###############################################################################
# Building the Activex plugin
###############################################################################

MOSTLYCLEANFILES = axvlc.inf axvlc_rc.rc
EXTRA_DIST = $(DIST_rsrc) $(DIST_misc)

DIST_rsrc = \
    axvlc_rc.rc.in \
    $(NULL)


DIST_misc = \
    axvlc.inf.in \
    axvlc.def \
    axvlc.dll.manifest \
    axvlc.idl \
    axvlc.tlb \
    inplace.bmp \
    vlc16x16.bmp \
    ../share/pixmaps/win32/fullscreen.bmp \
    ../share/pixmaps/win32/defullscreen.bmp \
    ../share/pixmaps/win32/play.bmp \
    ../share/pixmaps/win32/pause.bmp \
    ../share/pixmaps/win32/volume.bmp \
    ../share/pixmaps/win32/volume-muted.bmp \
    ../share/test.html \
    $(NULL)


lib_LTLIBRARIES = axvlc.la

AM_CPPFLAGS = $(LIBVLC_CFLAGS) -DUNICODE -D_UNICODE -D_MIDL_USE_GUIDDEF_
AM_CXXFLAGS = ${CXXFLAGS} -fno-exceptions

axvlc_la_SOURCES = \
    main.cpp \
    utils.cpp \
    utils.h \
    olecontrol.cpp \
    olecontrol.h \
    oleinplaceactiveobject.cpp \
    oleinplaceactiveobject.h \
    oleinplaceobject.cpp \
    oleinplaceobject.h \
    oleobject.cpp \
    oleobject.h \
    persistpropbag.cpp \
    persistpropbag.h \
    persiststorage.cpp \
    persiststorage.h \
    persiststreaminit.cpp \
    persiststreaminit.h \
    ../common/position.h \
    provideclassinfo.cpp \
    provideclassinfo.h \
    connectioncontainer.cpp \
    connectioncontainer.h \
    objectsafety.cpp \
    objectsafety.h \
    dataobject.cpp \
    dataobject.h \
    viewobject.cpp \
    viewobject.h \
    supporterrorinfo.cpp \
    supporterrorinfo.h \
    vlccontrol.cpp \
    vlccontrol.h \
    vlccontrol2.cpp \
    vlccontrol2.h \
    plugin.cpp \
    plugin.h \
    axvlc_idl.c \
    axvlc_idl.h \
    ../common/vlc_player_options.h \
    ../common/win32_fullscreen.cpp \
    ../common/win32_fullscreen.h \
    ../common/win32_vlcwnd.cpp \
    ../common/win32_vlcwnd.h \
    ../common/vlc_player.cpp \
    ../common/vlc_player.h
    $(NULL)

axvlc_la_DEPENDENCIES = $(srcdir)/axvlc.def $(DATA_axvlc_rc)
axvlc_la_LDFLAGS = -Wl,$(srcdir)/axvlc.def -Wl,$(DATA_axvlc_rc) \
    -no-undefined -avoid-version -module
axvlc_la_LIBADD = $(LIBVLC_LIBS) $(ACTIVEX_LIBS)


DATA_axvlc_rc = $(noinst_axvlc_rc_DATA)
noinst_axvlc_rc_DATA = axvlc_rc.$(OBJEXT)
noinst_axvlc_rcdir = 
axvlc_rc.$(OBJEXT): axvlc_rc.rc inplace.bmp axvlc.tlb
	$(WINDRES) --include-dir $(srcdir) -i $< -o $@


DATA_axvlc_tlb = $(noinst_axvlc_tlb_DATA)
noinst_axvlc_tlb_DATA = axvlc.tlb
noinst_axvlc_tlbdir =

if HAS_MIDL_COMPILER

axvlc.tlb axvlc_idl.c axvlc_idl.h: axvlc.idl
	$(MIDL) -Oicf -error all -env win32 -tlb axvlc.tlb -iid axvlc_idl.c -h axvlc_idl.h axvlc.idl

clean-tlb:
	rm -f axvlc.tlb axvlc_idl.c axvlc_idl.h

else
if HAS_WIDL_COMPILER

WINE_SDK_PATH ?= /usr/include/wine/windows

axvlc.tlb axvlc_idl.c axvlc_idl.h: axvlc.idl
	$(WIDL) -I$(WINE_SDK_PATH) -tuh -T axvlc.tlb -U axvlc_idl.c -H axvlc_idl.h axvlc.idl

clean-tlb:
	rm -f axvlc.tlb axvlc_idl.c axvlc_idl.h

else

clean-tlb:

endif
endif

###############################################################################
# Clean rules
###############################################################################

clean-local: clean-tlb

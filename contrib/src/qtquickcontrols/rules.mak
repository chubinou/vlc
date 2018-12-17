#QtQuickControls 1
QTQC_VERSION_MAJOR := 5.12
QTQC_VERSION := $(QTQC_VERSION_MAJOR).0
QTQC_URL := http://download.qt.io/official_releases/qt/$(QTQC_VERSION_MAJOR)/$(QTQC_VERSION)/submodules/qtquickcontrols-everywhere-src-$(QTQC_VERSION).tar.xz

DEPS_qtquickcontrols = qtquickcontrols2 $(DEPS_qtquickcontrols2)

$(TARBALLS)/qtquickcontrols-$(QTQC_VERSION).tar.xz:
	$(call download,$(QTQC_URL))

.sum-qtquickcontrols: qtquickcontrols-$(QTQC_VERSION).tar.xz

qtquickcontrols: qtquickcontrols-$(QTQC_VERSION).tar.xz .sum-qtquickcontrols
	$(UNPACK)
	mv qtquickcontrols-everywhere-src-$(QTQC_VERSION) qtquickcontrols-$(QTQC_VERSION)
	$(MOVE)

.qtquickcontrols: qtquickcontrols
ifdef HAVE_CROSS_COMPILE
	cd $< && $(PREFIX)/bin/qmake
else
	cd $< && ../qt/bin/qmake
endif
	# Make && Install libraries
	cd $< && $(MAKE)
	cd $< && $(MAKE) -C src sub-controls-install_subtargets
	cp $(PREFIX)/qml/QtQuick/Controls/libqtquickcontrolsplugin.a $(PREFIX)/lib/
	rm -rf $(PREFIX)/qml
	touch $@
